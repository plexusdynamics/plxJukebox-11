

#include "TCPServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "settings/AdvancedSettings.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "threads/SingleLock.h"

static const char     bt_service_name[] = "XBMC JSON-RPC";
static const char     bt_service_desc[] = "Interface for XBMC remote control over bluetooth";
static const char     bt_service_prov[] = "XBMC JSON-RPC Provider";
static const uint32_t bt_service_guid[] = {0x65AE4CC0, 0x775D11E0, 0xBE16CE28, 0x4824019B};

#ifdef HAVE_LIBBLUETOOTH
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

 /* The defines BDADDR_ANY and BDADDR_LOCAL are broken so use our own structs */
static const bdaddr_t bt_bdaddr_any   = {{0, 0, 0, 0, 0, 0}};
static const bdaddr_t bt_bdaddr_local = {{0, 0, 0, 0xff, 0xff, 0xff}};

#endif

using namespace JSONRPC;
using namespace ANNOUNCEMENT;
//using namespace std; On VS2010, bind conflicts with std::bind

#define RECEIVEBUFFER 1024

CTCPServer *CTCPServer::ServerInstance = NULL;

bool CTCPServer::StartServer(int port, bool nonlocal)
{
  StopServer(true);

  ServerInstance = new CTCPServer(port, nonlocal);
  if (ServerInstance->Initialize())
  {
    ServerInstance->Create();
    return true;
  }
  else
    return false;
}

void CTCPServer::StopServer(bool bWait)
{
  if (ServerInstance)
  {
    ServerInstance->StopThread(bWait);
    if (bWait)
    {
      delete ServerInstance;
      ServerInstance = NULL;
    }
  }
}

CTCPServer::CTCPServer(int port, bool nonlocal)
{
  m_port = port;
  m_nonlocal = nonlocal;
  m_sdpd = NULL;
}

void CTCPServer::Process()
{
  m_bStop = false;

  while (!m_bStop)
  {
    SOCKET          max_fd = 0;
    fd_set          rfds;
    struct timeval  to     = {1, 0};
    FD_ZERO(&rfds);

    for (std::vector<SOCKET>::iterator it = m_servers.begin(); it != m_servers.end(); it++)
    {
      FD_SET(*it, &rfds);
      if ((intptr_t)*it > (intptr_t)max_fd)
        max_fd = *it;
    }

    for (unsigned int i = 0; i < m_connections.size(); i++)
    {
      FD_SET(m_connections[i].m_socket, &rfds);
      if ((intptr_t)m_connections[i].m_socket > (intptr_t)max_fd)
        max_fd = m_connections[i].m_socket;
    }

    int res = select((intptr_t)max_fd+1, &rfds, NULL, NULL, &to);
    if (res < 0)
    {
      CLog::Log(LOGERROR, "JSONRPC Server: Select failed");
      Sleep(1000);
      Initialize();
    }
    else if (res > 0)
    {
      for (int i = m_connections.size() - 1; i >= 0; i--)
      {
        int socket = m_connections[i].m_socket;
        if (FD_ISSET(socket, &rfds))
        {
          char buffer[RECEIVEBUFFER] = {};
          int  nread = 0;
          nread = recv(socket, (char*)&buffer, RECEIVEBUFFER, 0);
          if (nread > 0)
          {
            m_connections[i].PushBuffer(this, buffer, nread);
          }
          if (nread <= 0)
          {
            CLog::Log(LOGINFO, "JSONRPC Server: Disconnection detected");
            m_connections[i].Disconnect();
            m_connections.erase(m_connections.begin() + i);
          }
        }
      }

      for (std::vector<SOCKET>::iterator it = m_servers.begin(); it != m_servers.end(); it++)
      {
        if (FD_ISSET(*it, &rfds))
        {
          CLog::Log(LOGDEBUG, "JSONRPC Server: New connection detected");
          CTCPClient newconnection;
          newconnection.m_socket = accept(*it, (sockaddr*)&newconnection.m_cliaddr, &newconnection.m_addrlen);

          if (newconnection.m_socket == INVALID_SOCKET)
            CLog::Log(LOGERROR, "JSONRPC Server: Accept of new connection failed");
          else
          {
            CLog::Log(LOGINFO, "JSONRPC Server: New connection added");
            m_connections.push_back(newconnection);
          }
        }
      }
    }
  }

  Deinitialize();
}

bool CTCPServer::PrepareDownload(const char *path, CVariant &details, std::string &protocol)
{
  return false;
}

bool CTCPServer::Download(const char *path, CVariant &result)
{
  return false;
}

int CTCPServer::GetCapabilities()
{
  return Response | Announcing;
}

void CTCPServer::Announce(EAnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  std::string str = AnnouncementToJSON(flag, sender, message, data, g_advancedSettings.m_jsonOutputCompact);

  for (unsigned int i = 0; i < m_connections.size(); i++)
  {
    {
      CSingleLock lock (m_connections[i].m_critSection);
      if ((m_connections[i].GetAnnouncementFlags() & flag) == 0)
        continue;
    }

    unsigned int sent = 0;
    do
    {
      CSingleLock lock (m_connections[i].m_critSection);
      sent += send(m_connections[i].m_socket, str.c_str(), str.size() - sent, sent);
    } while (sent < str.size());
  }
}

bool CTCPServer::Initialize()
{
  Deinitialize();

  bool started = false;

  started |= InitializeBlue();
  started |= InitializeTCP();

  if(started)
  {
    CAnnouncementManager::AddAnnouncer(this);
    CLog::Log(LOGINFO, "JSONRPC Server: Successfully initialized");
    return true;
  }
  return false;
}

bool CTCPServer::InitializeBlue()
{
  if(!m_nonlocal)
    return false;

#ifdef _WIN32

  SOCKET fd = socket (AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
  if(fd == INVALID_SOCKET)
  {
    CLog::Log(LOGINFO, "JSONRPC Server: Unable to get bluetooth socket");
    return false;
  }
  SOCKADDR_BTH sa  = {};
  sa.addressFamily = AF_BTH;
  sa.port          = BT_PORT_ANY;

  if(bind(fd, (SOCKADDR*)&sa, sizeof(sa)) < 0)
  {
    CLog::Log(LOGINFO, "JSONRPC Server: Unable to bind to bluetooth socket");
    closesocket(fd);
    return false;
  }

  ULONG optval = TRUE;
  if(setsockopt(fd, SOL_RFCOMM, SO_BTH_AUTHENTICATE, (const char*)&optval, sizeof(optval)) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to force authentication for bluetooth socket");
    closesocket(fd);
    return false;
  }

  int len = sizeof(sa);
  if(getsockname(fd, (SOCKADDR*)&sa, &len) < 0)
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to get bluetooth port");

  if (listen(fd, 10) < 0)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to listen to bluetooth port");
    closesocket(fd);
    return false;
  }

  m_servers.push_back(fd);

  CSADDR_INFO addrinfo;
  addrinfo.iProtocol   = BTHPROTO_RFCOMM;
  addrinfo.iSocketType = SOCK_STREAM;
  addrinfo.LocalAddr.lpSockaddr       = (SOCKADDR*)&sa;
  addrinfo.LocalAddr.iSockaddrLength  = sizeof(sa);
  addrinfo.RemoteAddr.lpSockaddr      = (SOCKADDR*)&sa;
  addrinfo.RemoteAddr.iSockaddrLength = sizeof(sa);

  WSAQUERYSET service = {};
  service.dwSize = sizeof(service);
  service.lpszServiceInstanceName = (LPSTR)bt_service_name;
  service.lpServiceClassId        = (LPGUID)&bt_service_guid;
  service.lpszComment             = (LPSTR)bt_service_desc;
  service.dwNameSpace             = NS_BTH;
  service.lpNSProviderId          = NULL; /* RFCOMM? */
  service.lpcsaBuffer             = &addrinfo;
  service.dwNumberOfCsAddrs       = 1;

  if(WSASetService(&service, RNRSERVICE_REGISTER, 0) == SOCKET_ERROR)
    CLog::Log(LOGERROR, "JSONRPC Server: failed to register bluetooth service error %d",  WSAGetLastError());

  return true;
#endif

#ifdef HAVE_LIBBLUETOOTH

  SOCKET fd = socket (AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if(fd == INVALID_SOCKET)
  {
    CLog::Log(LOGINFO, "JSONRPC Server: Unable to get bluetooth socket");
    return false;
  }
  struct sockaddr_rc sa  = {0};
  sa.rc_family  = AF_BLUETOOTH;
  sa.rc_bdaddr  = bt_bdaddr_any;
  sa.rc_channel = 0;

  if(bind(fd, (struct sockaddr*)&sa, sizeof(sa)) < 0)
  {
    CLog::Log(LOGINFO, "JSONRPC Server: Unable to bind to bluetooth socket");
    closesocket(fd);
    return false;
  }

  socklen_t len = sizeof(sa);
  if(getsockname(fd, (struct sockaddr*)&sa, &len) < 0)
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to get bluetooth port");

  if (listen(fd, 10) < 0)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to listen to bluetooth port %d", sa.rc_channel);
    closesocket(fd);
    return false;
  }

  uint8_t rfcomm_channel = sa.rc_channel;

  uuid_t root_uuid, l2cap_uuid, rfcomm_uuid, svc_uuid;
  sdp_list_t *l2cap_list = 0,
             *rfcomm_list = 0,
             *root_list = 0,
             *proto_list = 0,
             *access_proto_list = 0,
             *service_class = 0;

  sdp_data_t *channel = 0;

  sdp_record_t *record = sdp_record_alloc();

  // set the general service ID
  sdp_uuid128_create( &svc_uuid, &bt_service_guid );
  sdp_set_service_id( record, svc_uuid );

  // make the service record publicly browsable
  sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
  root_list = sdp_list_append(0, &root_uuid);
  sdp_set_browse_groups( record, root_list );

  // set l2cap information
  sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
  l2cap_list = sdp_list_append( 0, &l2cap_uuid );
  proto_list = sdp_list_append( 0, l2cap_list );

  // set rfcomm information
  sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
  channel = sdp_data_alloc(SDP_UINT8, &rfcomm_channel);
  rfcomm_list = sdp_list_append( 0, &rfcomm_uuid );
  sdp_list_append( rfcomm_list, channel );
  sdp_list_append( proto_list, rfcomm_list );

  // attach protocol information to service record
  access_proto_list = sdp_list_append( 0, proto_list );
  sdp_set_access_protos( record, access_proto_list );

  // set the name, provider, and description
  sdp_set_info_attr(record, bt_service_name, bt_service_prov, bt_service_desc);

  // set the Service class ID
  service_class = sdp_list_append(0, &svc_uuid);
  sdp_set_service_classes( record, service_class);

  // cleanup
  sdp_data_free( channel );
  sdp_list_free( l2cap_list, 0 );
  sdp_list_free( rfcomm_list, 0 );
  sdp_list_free( root_list, 0 );
  sdp_list_free( access_proto_list, 0 );
  sdp_list_free( service_class, 0 );

  // connect to the local SDP server, register the service record
  sdp_session_t *session = sdp_connect( &bt_bdaddr_any, &bt_bdaddr_local, SDP_RETRY_IF_BUSY );
  if(session == NULL)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to connect to sdpd");
    closesocket(fd);
    sdp_record_free(record);
    return false;
  }

  if(sdp_record_register(session, record, 0) < 0)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to register record with error %d", errno);
    closesocket(fd);
    sdp_close(session);
    sdp_record_free(record);
    return false;
  }

  m_sdpd = session;
  m_servers.push_back(fd);

  return true;
#endif
  return false;
}

bool CTCPServer::InitializeTCP()
{

  struct sockaddr_in myaddr;
  memset(&myaddr, 0, sizeof(myaddr));

  myaddr.sin_family = AF_INET;
  myaddr.sin_port = htons(m_port);

  if (m_nonlocal)
    myaddr.sin_addr.s_addr = INADDR_ANY;
  else
    inet_pton(AF_INET, "127.0.0.1", &myaddr.sin_addr.s_addr);

  SOCKET fd = socket(PF_INET, SOCK_STREAM, 0);

  if (fd == INVALID_SOCKET)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to create serversocket");
    return false;
  }

  if (bind(fd, (struct sockaddr*)&myaddr, sizeof myaddr) < 0)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to bind serversocket");
    closesocket(fd);
    return false;
  }

  if (listen(fd, 10) < 0)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to set listen");
    closesocket(fd);
    return false;
  }
  m_servers.push_back(fd);
  return true;
}

void CTCPServer::Deinitialize()
{
  for (unsigned int i = 0; i < m_connections.size(); i++)
    m_connections[i].Disconnect();

  m_connections.clear();

  for (unsigned int i = 0; i < m_servers.size(); i++)
    closesocket(m_servers[i]);

  m_servers.clear();

#ifdef HAVE_LIBBLUETOOTH
  if(m_sdpd)
    sdp_close( (sdp_session_t*)m_sdpd );
  m_sdpd = NULL;
#endif

  CAnnouncementManager::RemoveAnnouncer(this);
}

CTCPServer::CTCPClient::CTCPClient()
{
  m_announcementflags = ANNOUNCE_ALL;
  m_socket = INVALID_SOCKET;
  m_beginBrackets = 0;
  m_endBrackets = 0;
  m_beginChar = 0;
  m_endChar = 0;

  m_addrlen = sizeof(m_cliaddr);
}

CTCPServer::CTCPClient::CTCPClient(const CTCPClient& client)
{
  Copy(client);
}

CTCPServer::CTCPClient& CTCPServer::CTCPClient::operator=(const CTCPClient& client)
{
  Copy(client);
  return *this;
}

int CTCPServer::CTCPClient::GetPermissionFlags()
{
  return OPERATION_PERMISSION_ALL;
}

int CTCPServer::CTCPClient::GetAnnouncementFlags()
{
  return m_announcementflags;
}

bool CTCPServer::CTCPClient::SetAnnouncementFlags(int flags)
{
  m_announcementflags = flags;
  return true;
}

void CTCPServer::CTCPClient::PushBuffer(CTCPServer *host, const char *buffer, int length)
{
  for (int i = 0; i < length; i++)
  {
    char c = buffer[i];

    if (m_beginChar == 0 && c == '{')
    {
      m_beginChar = '{';
      m_endChar = '}';
    }
    else if (m_beginChar == 0 && c == '[')
    {
      m_beginChar = '[';
      m_endChar = ']';
    }

    if (m_beginChar != 0)
    {
      m_buffer.push_back(c);
      if (c == m_beginChar)
        m_beginBrackets++;
      else if (c == m_endChar)
        m_endBrackets++;
      if (m_beginBrackets > 0 && m_endBrackets > 0 && m_beginBrackets == m_endBrackets)
      {
        std::string line = CJSONRPC::MethodCall(m_buffer, host, this);
        CSingleLock lock (m_critSection);
        send(m_socket, line.c_str(), line.size(), 0);
        m_beginChar = m_beginBrackets = m_endBrackets = 0;
        m_buffer.clear();
      }
    }
  }
}

void CTCPServer::CTCPClient::Disconnect()
{
  if (m_socket > 0)
  {
    CSingleLock lock (m_critSection);
    shutdown(m_socket, SHUT_RDWR);
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
  }
}

void CTCPServer::CTCPClient::Copy(const CTCPClient& client)
{
  m_socket            = client.m_socket;
  m_cliaddr           = client.m_cliaddr;
  m_addrlen           = client.m_addrlen;
  m_announcementflags = client.m_announcementflags;
  m_beginBrackets     = client.m_beginBrackets;
  m_endBrackets       = client.m_endBrackets;
  m_beginChar         = client.m_beginChar;
  m_endChar           = client.m_endChar;
  m_buffer            = client.m_buffer;
}

