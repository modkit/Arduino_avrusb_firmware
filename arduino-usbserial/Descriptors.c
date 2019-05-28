/*
             LUFA Library
     Copyright (C) Dean Camera, 2015.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2015  Dean Camera (dean [at] fourwalledcubicle [dot] com)
  Copyright 2019  Benjamin Riggs (https://github.com/riggs)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  USB Device Descriptors, for library use when in USB device mode. Descriptors are special
 *  computer-readable structures which the host requests upon device enumeration, to determine
 *  the device's capabilities and functions.
 */

#include "Descriptors.h"

uint8_t WebUSB_Enabled = 0;

/** Device descriptor structure. This descriptor, located in FLASH memory, describes the overall
 *  device characteristics, including the supported USB version, control endpoint size and the
 *  number of device configurations. The descriptor is read out by the USB host when the enumeration
 *  process begins.
 */
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor =
{
    .Header                 = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

    .USBSpecification       = VERSION_BCD(2,1,0),

    .Class                  = USB_CSCP_IADDeviceClass,
    .SubClass               = USB_CSCP_IADDeviceSubclass,
    .Protocol               = USB_CSCP_IADDeviceProtocol,

    .Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,

    .VendorID               = 0x2341, // Arduino
    .ProductID              = ARDUINO_MODEL_PID,
    .ReleaseNumber          = VERSION_BCD(0,0,2),

    .ManufacturerStrIndex   = STRING_ID_Manufacturer,
    .ProductStrIndex        = STRING_ID_Product,
    .SerialNumStrIndex      = USE_INTERNAL_SERIAL,

    .NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

const USB_Descriptor_Device_t PROGMEM DeviceDescriptor_WebUSB =
    {
        .Header                 = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

        .USBSpecification       = VERSION_BCD(2,1,0),

        .Class                  = USB_CSCP_NoDeviceClass,
        .SubClass               = USB_CSCP_NoDeviceSubclass,
        .Protocol               = USB_CSCP_NoDeviceProtocol,

        .Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,

        .VendorID				= 0x04B9,
        .ProductID              = 0x0AF1,
        .ReleaseNumber          = VERSION_BCD(0,0,2),

        .ManufacturerStrIndex   = STRING_ID_Manufacturer,
        .ProductStrIndex        = STRING_ID_Product,
        .SerialNumStrIndex      = USE_INTERNAL_SERIAL,

        .NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
    };

/** Binary device Object Store (BOS) descriptor structure. This descriptor, located in memory, describes a
 *  flexible and extensible framework for describing and adding device-level capabilities to the set of USB standard
 *  specifications. The BOS descriptor defines a root descriptor that is similar to the configuration descriptor,
 *  and is the base descriptor for accessing a family of related descriptors. It defines the number of 'sub' Device
 *  Capability Descriptors and the total length of itself and the sub-descriptors.
 */

const USB_Descriptor_BOS_t PROGMEM BOSDescriptor = BOS_DESCRIPTOR(
    (MS_OS_20_PLATFORM_DESCRIPTOR(MS_OS_20_VENDOR_CODE, MS_OS_20_DESCRIPTOR_SET_TOTAL_LENGTH))
    (WEBUSB_PLATFORM_DESCRIPTOR(WEBUSB_VENDOR_CODE, WEBUSB_LANDING_PAGE_INDEX))
);

const USB_Descriptor_BOS_t PROGMEM BOSDescriptor_WebUSB = BOS_DESCRIPTOR(
    (MS_OS_20_PLATFORM_DESCRIPTOR(MS_OS_20_VENDOR_CODE, MS_OS_20_DESCRIPTOR_SET_TOTAL_LENGTH_WEBUSB))
        (WEBUSB_PLATFORM_DESCRIPTOR(WEBUSB_VENDOR_CODE, WEBUSB_LANDING_PAGE_INDEX))
);


/** Configuration descriptor structure. This descriptor, located in FLASH memory, describes the usage
 *  of the device in one of its supported configurations, including information about any device interfaces
 *  and endpoints. The descriptor is read out by the USB host during the enumeration process when selecting
 *  a configuration so that the host may correctly communicate with the USB device.
 */
const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor =
{
    .Config =
        {
            .Header                 = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

            .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
            .TotalInterfaces        = 3,

            .ConfigurationNumber    = DEFAULT_CONFIG_INDEX,
            .ConfigurationStrIndex  = NO_DESCRIPTOR,

            .ConfigAttributes       = (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),

            .MaxPowerConsumption    = USB_CONFIG_POWER_MA(100)
        },
   
    .CDC_Interface_Association =
        {
            .Header					= {.Size = sizeof(USB_Descriptor_Interface_Association_t), .Type = DTYPE_InterfaceAssociation},

            .FirstInterfaceIndex	= INTERFACE_ID_CDC_CCI,
            .TotalInterfaces		= 2,

            .Class                  = CDC_CSCP_CDCClass,
            .SubClass               = CDC_CSCP_ACMSubclass,
            .Protocol               = CDC_CSCP_ATCommandProtocol,

            .IADStrIndex			= NO_DESCRIPTOR
        },

    .CDC_CCI_Interface =
        {
            .Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

            .InterfaceNumber        = INTERFACE_ID_CDC_CCI,
            .AlternateSetting       = 0,

            .TotalEndpoints         = 1,

            .Class                  = CDC_CSCP_CDCClass,
            .SubClass               = CDC_CSCP_ACMSubclass,
            .Protocol               = CDC_CSCP_ATCommandProtocol,

            .InterfaceStrIndex      = NO_DESCRIPTOR
        },

    .CDC_Functional_Header =
        {
            .Header                 = {.Size = sizeof(USB_CDC_Descriptor_FunctionalHeader_t), .Type = DTYPE_CSInterface},
            .Subtype                = CDC_DSUBTYPE_CSInterface_Header,

            .CDCSpecification       = VERSION_BCD(1,1,0),
        },

    .CDC_Functional_ACM =
        {
            .Header                 = {.Size = sizeof(USB_CDC_Descriptor_FunctionalACM_t), .Type = DTYPE_CSInterface},
            .Subtype                = CDC_DSUBTYPE_CSInterface_ACM,

            .Capabilities           = 0x06, /* Supports Network_Connection, Get_Line_Encoding, Set_Line_Encoding, Serial_State */
        },

    .CDC_Functional_Union =
        {
            .Header                 = {.Size = sizeof(USB_CDC_Descriptor_FunctionalUnion_t), .Type = DTYPE_CSInterface},
            .Subtype                = CDC_DSUBTYPE_CSInterface_Union,

            .MasterInterfaceNumber  = INTERFACE_ID_CDC_CCI,
            .SlaveInterfaceNumber   = INTERFACE_ID_CDC_DCI,
        },

    .CDC_NotificationEndpoint =
        {
            .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

            .EndpointAddress        = CDC_NOTIFICATION_EPADDR,
            .Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
            .EndpointSize           = CDC_NOTIFICATION_EPSIZE,
            .PollingIntervalMS      = 0xFF
        },

    .CDC_DCI_Interface =
        {
            .Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

            .InterfaceNumber        = INTERFACE_ID_CDC_DCI,
            .AlternateSetting       = 0,

            .TotalEndpoints         = 2,

            .Class                  = CDC_CSCP_CDCDataClass,
            .SubClass               = CDC_CSCP_NoDataSubclass,
            .Protocol               = CDC_CSCP_NoDataProtocol,

            .InterfaceStrIndex      = NO_DESCRIPTOR
        },

    .CDC_DataOutEndpoint =
        {
            .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

            .EndpointAddress        = CDC_RX_EPADDR,
            .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
            .EndpointSize           = CDC_TXRX_EPSIZE,
            .PollingIntervalMS      = 0x05
        },

    .CDC_DataInEndpoint =
        {
            .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

            .EndpointAddress        = CDC_TX_EPADDR,
            .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
            .EndpointSize           = CDC_TXRX_EPSIZE,
            .PollingIntervalMS      = 0x05
        },

    .WebUSB_CDC_Interface =
        {
            .Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

            .InterfaceNumber        = INTERFACE_ID_WEBUSB,
            .AlternateSetting       = 0,

            .TotalEndpoints         = 0,

            .Class                  = USB_CSCP_VendorSpecificClass,
            .SubClass               = USB_CSCP_NoDeviceSubclass,
            .Protocol               = USB_CSCP_NoDeviceProtocol,

            .InterfaceStrIndex      = NO_DESCRIPTOR
        },
};

const USB_Descriptor_Configuration_WebUSB_t PROGMEM ConfigurationDescriptor_WebUSB =
    {
        .Config =
            {
                .Header                 = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

                .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_WebUSB_t),
                .TotalInterfaces        = 1,

                .ConfigurationNumber    = DEFAULT_CONFIG_INDEX,
                .ConfigurationStrIndex  = NO_DESCRIPTOR,

                .ConfigAttributes       = (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),

                .MaxPowerConsumption    = USB_CONFIG_POWER_MA(100)
            },

        .Interface =
            {
                .Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

                .InterfaceNumber        = 0,
                .AlternateSetting       = 0,

                .TotalEndpoints         = 3,

                .Class                  = USB_CSCP_VendorSpecificClass,
                .SubClass               = USB_CSCP_NoDeviceSubclass,
                .Protocol               = USB_CSCP_NoDeviceProtocol,

                .InterfaceStrIndex      = NO_DESCRIPTOR
            },

        .CDC_NotificationEndpoint =
            {
                .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

                .EndpointAddress        = CDC_NOTIFICATION_EPADDR,
                .Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                .EndpointSize           = CDC_NOTIFICATION_EPSIZE,
                .PollingIntervalMS      = 0xFF
            },

        .CDC_DataOutEndpoint =
            {
                .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

                .EndpointAddress        = CDC_RX_EPADDR,
                .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                .EndpointSize           = CDC_TXRX_EPSIZE,
                .PollingIntervalMS      = 0x05
            },

        .CDC_DataInEndpoint =
            {
                .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

                .EndpointAddress        = CDC_TX_EPADDR,
                .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                .EndpointSize           = CDC_TXRX_EPSIZE,
                .PollingIntervalMS      = 0x05
            },
};

/** Language descriptor structure. This descriptor, located in FLASH memory, is returned when the host requests
 *  the string descriptor with index 0 (the first index). It is actually an array of 16-bit integers, which indicate
 *  via the language ID table available at USB.org what languages the device supports for its string descriptors.
 */
const USB_Descriptor_String_t PROGMEM LanguageString = USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);

/** Manufacturer descriptor string. This is a Unicode string containing the manufacturer's details in human readable
 *  form, and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ManufacturerString = USB_STRING_DESCRIPTOR(L"Arduino (www.arduino.cc)");
const USB_Descriptor_String_t PROGMEM ManufacturerString_WebUSB = USB_STRING_DESCRIPTOR(L"Arduino (Modkit Remix)");

/** Product descriptor string. This is a Unicode string containing the product's details in human readable form,
 *  and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
#if (ARDUINO_MODEL_PID == ARDUINO_UNO_PID)
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"Arduino UNO");
const USB_Descriptor_String_t PROGMEM ProductString_WebUSB = USB_STRING_DESCRIPTOR(L"Arduino UNO (Modkit Remix)");
#elif (ARDUINO_MODEL_PID == ARDUINO_MEGA2560_PID)
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"Arduino Mega 2560");
const USB_Descriptor_String_t PROGMEM ProductString_WebUSB = USB_STRING_DESCRIPTOR(L"Arduino Mega 2560 (Modkit Remix)");
#elif (ARDUINO_MODEL_PID == ARDUINO_USBSERIAL_PID)
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"Arduino USB-Serial");
const USB_Descriptor_String_t PROGMEM ProductString_WebUSB = USB_STRING_DESCRIPTOR(L"Arduino USB-Serial (Modkit Remix)");
#elif (ARDUINO_MODEL_PID == ARDUINO_MEGAADK_PID)
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"Arduino Mega ADK");
const USB_Descriptor_String_t PROGMEM ProductString_WebUSB = USB_STRING_DESCRIPTOR(L"Arduino Mega ADK (Modkit Remix)");
#elif (ARDUINO_MODEL_PID == ARDUINO_MEGA2560R3_PID)
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"Arduino Mega 2560 R3");
const USB_Descriptor_String_t PROGMEM ProductString_WebUSB = USB_STRING_DESCRIPTOR(L"Arduino Mega 2560 R3 (Modkit Remix)");
#elif (ARDUINO_MODEL_PID == ARDUINO_UNOR3_PID)
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"Arduino UNO R3");
const USB_Descriptor_String_t PROGMEM ProductString_WebUSB = USB_STRING_DESCRIPTOR(L"Arduino UNO R3 (Modkit Remix)");
#elif (ARDUINO_MODEL_PID == ARDUINO_MEGAADKR3_PID)
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"Arduino Mega ADK R3");
const USB_Descriptor_String_t PROGMEM ProductString_WebUSB = USB_STRING_DESCRIPTOR(L"Arduino Mega ADK R3 (Modkit Remix)");
#elif (ARDUINO_MODEL_PID == ARDUINO_UNOR3_PID+0x200)
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"Genuino UNO R3");
const USB_Descriptor_String_t PROGMEM ProductString_WebUSB = USB_STRING_DESCRIPTOR(L"Genuino UNO R3 (Modkit Remix)");
#elif (ARDUINO_MODEL_PID == ARDUINO_MEGA2560R3_PID+0x200)
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"Genuino Mega 2560 R3");
const USB_Descriptor_String_t PROGMEM ProductString_WebUSB = USB_STRING_DESCRIPTOR(L"Genuino Mega 2560 R3 (Modkit Remix)");
#endif

/** This function is called by the library when in device mode, and must be overridden (see library "USB Descriptors"
 *  documentation) by the application code so that the address and size of a requested descriptor can be given
 *  to the USB library. When the device receives a Get Descriptor request on the control endpoint, this function
 *  is called so that the descriptor details can be passed back and the appropriate descriptor sent back to the
 *  USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const DescriptorAddress)
{
    const uint8_t  DescriptorType   = (wValue >> 8);
    const uint8_t  DescriptorNumber = (wValue & 0xFF);

    const void* Address = NULL;
    uint16_t    Size    = NO_DESCRIPTOR;

    switch (DescriptorType)
    {
        case DTYPE_Device:
            if (WebUSB_Enabled) {
                Address = &DeviceDescriptor_WebUSB;
                LEDs_ToggleLEDs(LEDS_LED1);
            } else {
                Address = &DeviceDescriptor;
                LEDs_ToggleLEDs(LEDS_ALL_LEDS);
            }
            Size    = sizeof(USB_Descriptor_Device_t);
            break;
        case DTYPE_BOS:
            if (WebUSB_Enabled) {
                Address = &BOSDescriptor_WebUSB;
                Size = pgm_read_byte(&BOSDescriptor_WebUSB.TotalLength);
                LEDs_ToggleLEDs(LEDS_LED1);
            } else {
                Address = &BOSDescriptor;
                Size = pgm_read_byte(&BOSDescriptor.TotalLength);
            }
            break;
        case DTYPE_Configuration:
            if (WebUSB_Enabled) {
                Address = &ConfigurationDescriptor_WebUSB;
                Size    = sizeof(USB_Descriptor_Configuration_WebUSB_t);
                LEDs_ToggleLEDs(LEDS_LED1);
            } else {
                Address = &ConfigurationDescriptor;
                Size    = sizeof(USB_Descriptor_Configuration_t);
            }
            break;
        case DTYPE_String:
            switch (DescriptorNumber)
            {
                case STRING_ID_Language:
                    Address = &LanguageString;
                    Size    = pgm_read_byte(&LanguageString.Header.Size);
                    break;
                case STRING_ID_Manufacturer:
                    if (WebUSB_Enabled) {
                        Address = &ManufacturerString_WebUSB;
                        Size    = pgm_read_byte(&ManufacturerString_WebUSB.Header.Size);
                    } else {
                        Address = &ManufacturerString;
                        Size    = pgm_read_byte(&ManufacturerString.Header.Size);
                    }
                    break;
                case STRING_ID_Product:
                    if (WebUSB_Enabled) {
                        Address = &ProductString_WebUSB;
                        Size    = pgm_read_byte(&ProductString_WebUSB.Header.Size);
                    } else {
                        Address = &ProductString;
                        Size    = pgm_read_byte(&ProductString.Header.Size);
                    }
                    break;
            }
            break;
    }

    *DescriptorAddress = Address;
    return Size;
}
