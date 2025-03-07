

#include "XBMCOperations.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "Util.h"
#include "utils/Variant.h"
#include "powermanagement/PowerManager.h"

using namespace JSONRPC;

JSON_STATUS CXBMCOperations::GetInfoLabels(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::vector<CStdString> info;

  for (unsigned int i = 0; i < parameterObject["labels"].size(); i++)
  {
   CStdString field = parameterObject["labels"][i].asString();
    field = field.ToLower();

    info.push_back(parameterObject["labels"][i].asString());
  }

  if (info.size() > 0)
  {
    std::vector<CStdString> infoLabels = g_application.getApplicationMessenger().GetInfoLabels(info);
    for (unsigned int i = 0; i < info.size(); i++)
    {
      if (i >= infoLabels.size())
        break;
      result[info[i].c_str()] = infoLabels[i];
    }
  }

  return OK;
}

JSON_STATUS CXBMCOperations::GetInfoBooleans(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::vector<CStdString> info;

  bool CanControlPower = (client->GetPermissionFlags() & ControlPower) > 0;

  for (unsigned int i = 0; i < parameterObject["booleans"].size(); i++)
  {
    CStdString field = parameterObject["booleans"][i].asString();
    field = field.ToLower();

    // Need to override power management of whats in infomanager since jsonrpc
    // have a security layer aswell.
    if (field.Equals("system.canshutdown"))
      result[parameterObject["booleans"][i].asString()] = (g_powerManager.CanPowerdown() && CanControlPower);
    else if (field.Equals("system.canpowerdown"))
      result[parameterObject["booleans"][i].asString()] = (g_powerManager.CanPowerdown() && CanControlPower);
    else if (field.Equals("system.cansuspend"))
      result[parameterObject["booleans"][i].asString()] = (g_powerManager.CanSuspend() && CanControlPower);
    else if (field.Equals("system.canhibernate"))
      result[parameterObject["booleans"][i].asString()] = (g_powerManager.CanHibernate() && CanControlPower);
    else if (field.Equals("system.canreboot"))
      result[parameterObject["booleans"][i].asString()] = (g_powerManager.CanReboot() && CanControlPower);
    else
      info.push_back(parameterObject["booleans"][i].asString());
  }

  if (info.size() > 0)
  {
    std::vector<bool> infoLabels = g_application.getApplicationMessenger().GetInfoBooleans(info);
    for (unsigned int i = 0; i < info.size(); i++)
    {
      if (i >= infoLabels.size())
        break;
      result[info[i].c_str()] = CVariant(infoLabels[i]);
    }
  }

  return OK;
}
