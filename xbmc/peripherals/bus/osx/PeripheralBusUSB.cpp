

#include "PeripheralBusUSB.h"
#include "peripherals/Peripherals.h"
#include "utils/log.h"

using namespace PERIPHERALS;

#ifdef TARGET_DARWIN_OSX
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/serial/IOSerialKeys.h>

typedef struct USBDevicePrivateData {
  CPeripheralBusUSB     *refCon;
  CStdString            deviceName;
  io_object_t           notification;
  PeripheralScanResult  result;
} USBDevicePrivateData;
#endif

CPeripheralBusUSB::CPeripheralBusUSB(CPeripherals *manager) :
    CPeripheralBus(manager, PERIPHERAL_BUS_USB)
{
  m_bNeedsPolling = false;

#ifdef TARGET_DARWIN_OSX
  //set up the matching criteria for the devices we're interested in
  //interested in instances of class IOUSBDevice and its subclasses
  // match any usb device by not creating matching values in the dict
  CFMutableDictionaryRef matching_dict = IOServiceMatching(kIOUSBDeviceClassName);

  m_notify_port = IONotificationPortCreate(kIOMasterPortDefault);
  CFRunLoopSourceRef run_loop_source = IONotificationPortGetRunLoopSource(m_notify_port);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop_source, kCFRunLoopCommonModes);

  //add a notification callback for attach event
  IOReturn result = IOServiceAddMatchingNotification(m_notify_port,
    kIOFirstMatchNotification, matching_dict,
    (IOServiceMatchingCallback)DeviceAttachCallback, this, &m_attach_iterator);
  if (result == kIOReturnSuccess)
  {
    //call the callback to 'arm' the notification
    DeviceAttachCallback(this, m_attach_iterator);
  }
#endif
}

CPeripheralBusUSB::~CPeripheralBusUSB()
{
#ifdef TARGET_DARWIN_OSX
  if (m_notify_port)
  {
    // remove the sleep notification port from the application runloop
    CFRunLoopRemoveSource( CFRunLoopGetCurrent(),
      IONotificationPortGetRunLoopSource(m_notify_port), kCFRunLoopDefaultMode );
    IONotificationPortDestroy(m_notify_port);
    m_notify_port = 0;
  }
  if (m_attach_iterator)
  {
    IOObjectRelease(m_attach_iterator);
    m_attach_iterator = 0;
  }
#endif
}

bool CPeripheralBusUSB::PerformDeviceScan(PeripheralScanResults &results)
{
  results = m_scan_results;
  return true;
}

#ifdef TARGET_DARWIN_OSX
const PeripheralType CPeripheralBusUSB::GetType(int iDeviceClass)
{
  switch (iDeviceClass)
  {
  case kUSBHIDClass:
    return PERIPHERAL_HID;
  case kUSBCommClass:
    return PERIPHERAL_NIC;
  case kUSBMassStorageClass:
    return PERIPHERAL_DISK;
  //case USB_CLASS_PER_INTERFACE:
  case kUSBAudioClass:
  case kUSBPrintingClass:
  //case USB_CLASS_PTP:
  case kUSBHubClass:
  case kUSBDataClass:
  case kUSBVendorSpecificClass:
  default:
    return PERIPHERAL_UNKNOWN;
  }
}

void CPeripheralBusUSB::DeviceDetachCallback(void *refCon, io_service_t service, natural_t messageType, void *messageArgument)
{ 
  if (messageType == kIOMessageServiceIsTerminated)
  {
    IOReturn result;

    USBDevicePrivateData *privateDataRef = (USBDevicePrivateData*)refCon;

    std::vector<PeripheralScanResult>::iterator it = privateDataRef->refCon->m_scan_results.m_results.begin();
    while(it != privateDataRef->refCon->m_scan_results.m_results.end())
    {
      if (privateDataRef->result.m_strLocation == it->m_strLocation)
        privateDataRef->refCon->m_scan_results.m_results.erase(it);
      else
        it++;
    }
    privateDataRef->refCon->ScanForDevices();
    
    CLog::Log(LOGDEBUG, "USB Device Detach:%s, %s\n",
      privateDataRef->deviceName.c_str(), privateDataRef->result.m_strLocation.c_str());
    result = IOObjectRelease(privateDataRef->notification);
    delete privateDataRef;
    //release the service
    result = IOObjectRelease(service);
  }
}

void CPeripheralBusUSB::DeviceAttachCallback(CPeripheralBusUSB* refCon, io_iterator_t iterator)
{
  io_service_t usbDevice;
  while ((usbDevice = IOIteratorNext(iterator)))
  {
    IOReturn  result;

    io_name_t deviceName;
    result = IORegistryEntryGetName(usbDevice, deviceName);
		if (result != KERN_SUCCESS)
      deviceName[0] = '\0';

    SInt32 deviceScore;
    IOCFPlugInInterface **devicePlugin;
    result = IOCreatePlugInInterfaceForService(usbDevice,
      kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &devicePlugin, &deviceScore);
    if (result != kIOReturnSuccess)
    {
      IOObjectRelease(usbDevice);
      continue;
    }

    IOUSBDeviceInterface **deviceInterface;
    // Use the plugin interface to retrieve the device interface.
    result = (*devicePlugin)->QueryInterface(devicePlugin,
      CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID*)&deviceInterface);
    if (result != kIOReturnSuccess)
    {
      IODestroyPlugInInterface(devicePlugin);
      IOObjectRelease(usbDevice);
      continue;
    }

    // get vendor/product ids
    UInt16  vendorId;
    UInt16  productId;
    UInt32  locationId;
    UInt8   bDeviceClass;

    result = (*deviceInterface)->GetDeviceVendor( deviceInterface, &vendorId);
    result = (*deviceInterface)->GetDeviceProduct(deviceInterface, &productId);
    result = (*deviceInterface)->GetLocationID(   deviceInterface, &locationId);
    result = (*deviceInterface)->GetDeviceClass(  deviceInterface, &bDeviceClass);

    io_service_t usbInterface;
    io_iterator_t interface_iterator;
    IOUSBFindInterfaceRequest	request;
    request.bInterfaceClass    = kIOUSBFindInterfaceDontCare;
    request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    request.bAlternateSetting  = kIOUSBFindInterfaceDontCare;
    result = (*deviceInterface)->CreateInterfaceIterator(deviceInterface, &request, &interface_iterator);
    while ((usbInterface = IOIteratorNext(interface_iterator)))
    {
      SInt32 interfaceScore;
      IOCFPlugInInterface **interfacePlugin;
      //create intermediate plugin for interface access
      result = IOCreatePlugInInterfaceForService(usbInterface, 
        kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &interfacePlugin, &interfaceScore);
      if (result != kIOReturnSuccess)
      {
        IOObjectRelease(usbInterface);
        continue;
      }
      IOUSBInterfaceInterface** interfaceInterface;
      result = (*interfacePlugin)->QueryInterface(interfacePlugin,
        CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (void**)&interfaceInterface);
      if (result != kIOReturnSuccess)
      {
        IODestroyPlugInInterface(interfacePlugin);
        IOObjectRelease(usbInterface);
        continue;
      }

      // finally we can get to the bInterfaceClass
      // we should also check for kHIDKeyboardInterfaceProtocol but
      // some IR remotes that emulate an HID keyboard do not report this.
      UInt8 bInterfaceClass;
      result = (*interfaceInterface)->GetInterfaceClass(interfaceInterface, &bInterfaceClass);
      if (bInterfaceClass == kUSBHIDInterfaceClass || bInterfaceClass == kUSBCommunicationDataInterfaceClass)
      {
        char ttlDeviceFilePath[MAXPATHLEN] = {0};
        CFStringRef deviceFilePathAsCFString;
        USBDevicePrivateData *privateDataRef;
        privateDataRef = new USBDevicePrivateData;
        // save the device info to our private data.
        privateDataRef->refCon = refCon;
        privateDataRef->deviceName = deviceName;
        privateDataRef->result.m_iVendorId  = vendorId;
        privateDataRef->result.m_iProductId = productId;

        if (bInterfaceClass == kUSBCommunicationDataInterfaceClass)
        {
          // fetch the bds device path if this is USB serial device.
          // to do this we have to switch from the kIOUSBPlane to
          // kIOServicePlane, then we can search down for the path.
          io_registry_entry_t parent;
          kern_return_t kresult;
          kresult = IORegistryEntryGetParentEntry(usbInterface, kIOServicePlane, &parent);
          if (kresult == KERN_SUCCESS)
          {
            deviceFilePathAsCFString = (CFStringRef)IORegistryEntrySearchCFProperty(parent,
              kIOServicePlane, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, kIORegistryIterateRecursively);
            if (deviceFilePathAsCFString)
            {
              // Convert the path from a CFString to a NULL-terminated C string
              CFStringGetCString((CFStringRef)deviceFilePathAsCFString,
                ttlDeviceFilePath, MAXPATHLEN - 1, kCFStringEncodingASCII);
              CFRelease(deviceFilePathAsCFString);
            }
            IOObjectRelease(parent);
          }
        }
        if (strlen(ttlDeviceFilePath))
          privateDataRef->result.m_strLocation.Format("%s", ttlDeviceFilePath);
        else
          privateDataRef->result.m_strLocation.Format("%d", locationId);

        if (bDeviceClass == kUSBCompositeClass)
          privateDataRef->result.m_type = refCon->GetType(bInterfaceClass);
        else
          privateDataRef->result.m_type = refCon->GetType(bDeviceClass);

        if (!refCon->m_scan_results.ContainsResult(privateDataRef->result))
        {
          // register this usb device for an interest notification callback. 
          result = IOServiceAddInterestNotification(refCon->m_notify_port,
            usbDevice,                      // service
            kIOGeneralInterest,             // interestType
            (IOServiceInterestCallback)DeviceDetachCallback, // callback
            privateDataRef,                 // refCon
            &privateDataRef->notification); // notification
            
          if (result == kIOReturnSuccess)
          {
            refCon->m_scan_results.m_results.push_back(privateDataRef->result);
            CLog::Log(LOGDEBUG, "USB Device Attach:%s, %s\n",
              deviceName, privateDataRef->result.m_strLocation.c_str());
          }
          else
          {
            delete privateDataRef;
          }
        }
        else
        {
          delete privateDataRef;
        }
        // done with this device, only need one notification per device.
        IODestroyPlugInInterface(interfacePlugin);
        IOObjectRelease(usbInterface);
        break;
      }
      IODestroyPlugInInterface(interfacePlugin);
      IOObjectRelease(usbInterface);
    }
    IODestroyPlugInInterface(devicePlugin);
    IOObjectRelease(usbDevice);
  }
  refCon->ScanForDevices();
}
#endif
