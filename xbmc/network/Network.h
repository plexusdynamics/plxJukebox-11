#ifndef NETWORK_H_
#define NETWORK_H_



#include <vector>
#include "utils/StdString.h"

enum EncMode { ENC_NONE = 0, ENC_WEP = 1, ENC_WPA = 2, ENC_WPA2 = 3 };
enum NetworkAssignment { NETWORK_DASH = 0, NETWORK_DHCP = 1, NETWORK_STATIC = 2, NETWORK_DISABLED = 3 };

class NetworkAccessPoint
{
public:
   NetworkAccessPoint(CStdString& essId, int quality, EncMode encryption)
   {
      m_essId = essId;
      m_quality = quality;
      m_encryptionMode = encryption;
   }

   CStdString getEssId() { return m_essId; }
   int getQuality() { return m_quality; }
   EncMode getEncryptionMode() { return m_encryptionMode; }

private:
   CStdString   m_essId;
   int          m_quality;
   EncMode      m_encryptionMode;
};

class CNetworkInterface
{
public:
   virtual ~CNetworkInterface() {};

   virtual CStdString& GetName(void) = 0;

   virtual bool IsEnabled(void) = 0;
   virtual bool IsConnected(void) = 0;
   virtual bool IsWireless(void) = 0;

   virtual CStdString GetMacAddress(void) = 0;

   virtual CStdString GetCurrentIPAddress() = 0;
   virtual CStdString GetCurrentNetmask() = 0;
   virtual CStdString GetCurrentDefaultGateway(void) = 0;
   virtual CStdString GetCurrentWirelessEssId(void) = 0;

   // Returns the list of access points in the area
   virtual std::vector<NetworkAccessPoint> GetAccessPoints(void) = 0;

   virtual void GetSettings(NetworkAssignment& assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode) = 0;
   virtual void SetSettings(NetworkAssignment& assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode) = 0;
};



class CNetwork
{
public:
  enum EMESSAGE
  {
    SERVICES_UP,
    SERVICES_DOWN
  };

   CNetwork();
   virtual ~CNetwork();

   // Return our hostname
   virtual CStdString GetHostName(void);

   // Return the list of interfaces
   virtual std::vector<CNetworkInterface*>& GetInterfaceList(void) = 0;
   CNetworkInterface* GetInterfaceByName(CStdString& name);

   // Return the first interface which is active
   virtual CNetworkInterface* GetFirstConnectedInterface(void);

   // Return true if there is a interface for the same network as address
   bool HasInterfaceForIP(unsigned long address);

   // Return true if there's at least one defined network interface
   bool IsAvailable(bool wait = false);

   // Return true if there's at least one interface which is connected
   bool IsConnected(void);

   // Return true if the magic packet was send
   bool WakeOnLan(const char *mac);

   // Get/set the nameserver(s)
   virtual std::vector<CStdString> GetNameServers(void) = 0;
   virtual void SetNameServers(std::vector<CStdString> nameServers) = 0;

   // callback from application controlled thread to handle any setup
   void NetworkMessage(EMESSAGE message, int param);

   void StartServices();
   void StopServices(bool bWait);

   static int ParseHex(char *str, unsigned char *addr);
};
#ifdef HAS_LINUX_NETWORK
#include "linux/NetworkLinux.h"
#else
#include "windows/NetworkWin32.h"
#endif
#endif
