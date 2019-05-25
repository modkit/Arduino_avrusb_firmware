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
 *  Header file for Descriptors.c.
 */

#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

/* Includes: */
#include <avr/pgmspace.h>

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/Board/LEDs.h>

#include "../WebUSBConfig.h"
#include "../WebUSBDevice.h"
#include "../MS_OS_20_Device.h"

/* Product-specific definitions: */
#define ARDUINO_UNO_PID         0x0001
#define ARDUINO_MEGA2560_PID    0x0010
#define ARDUINO_USBSERIAL_PID   0x003B
#define ARDUINO_MEGAADK_PID     0x003F
#define ARDUINO_MEGA2560R3_PID  0x0042
#define ARDUINO_UNOR3_PID       0x0043
#define ARDUINO_MEGAADKR3_PID   0x0044

/* Macros: */
/** Endpoint address of the CDC device-to-host notification IN endpoint. */
#define CDC_NOTIFICATION_EPADDR        (ENDPOINT_DIR_IN  | 1)

/** Endpoint address of the CDC device-to-host data IN endpoint. */
#define CDC_TX_EPADDR                  (ENDPOINT_DIR_IN  | 3)

/** Endpoint address of the CDC host-to-device data OUT endpoint. */
#define CDC_RX_EPADDR                  (ENDPOINT_DIR_OUT | 2)

/** Size in bytes of the CDC device-to-host notification IN endpoint. */
#define CDC_NOTIFICATION_EPSIZE        8

/** Size in bytes of the CDC data IN and OUT endpoints. */
#define CDC_TXRX_EPSIZE                64

/* Shared state variable */
extern uint8_t WebUSB_Enabled;

/* Type Defines: */
/** Type define for the device configuration descriptor structure. This must be defined in the
 *  application code, as the configuration descriptor contains several sub-descriptors which
 *  vary between devices, and which describe the device's usage to the host.
 */
typedef struct
{
    USB_Descriptor_Configuration_Header_t 	Config;

    // Groups the two CDC Interfaces
    USB_Descriptor_Interface_Association_t	CDC_Interface_Association;
      // CDC Command Interface
    USB_Descriptor_Interface_t              CDC_CCI_Interface;
    USB_CDC_Descriptor_FunctionalHeader_t   CDC_Functional_Header;
    USB_CDC_Descriptor_FunctionalACM_t      CDC_Functional_ACM;
    USB_CDC_Descriptor_FunctionalUnion_t    CDC_Functional_Union;
    USB_Descriptor_Endpoint_t               CDC_NotificationEndpoint;
    // CDC Data Interface
    USB_Descriptor_Interface_t              CDC_DCI_Interface;
    USB_Descriptor_Endpoint_t               CDC_DataOutEndpoint;
    USB_Descriptor_Endpoint_t               CDC_DataInEndpoint;

    USB_Descriptor_Interface_t             	WebUSB_CDC_Interface;
} USB_Descriptor_Configuration_t;

typedef struct
{
    USB_Descriptor_Configuration_Header_t 	Config;
    USB_Descriptor_Interface_t              Interface;
    USB_Descriptor_Endpoint_t               CDC_NotificationEndpoint;
    USB_Descriptor_Endpoint_t               CDC_DataOutEndpoint;
    USB_Descriptor_Endpoint_t               CDC_DataInEndpoint;
} USB_Descriptor_Configuration_WebUSB_t;

/** Enum for the device interface descriptor IDs within the device. Each interface descriptor
 *  should have a unique ID index associated with it, which can be used to refer to the
 *  interface from other descriptors.
 */
enum InterfaceDescriptors_t
{
    INTERFACE_ID_CDC_CCI = 0, /**< CDC CCI interface descriptor ID */
    INTERFACE_ID_CDC_DCI = 1, /**< CDC DCI interface descriptor ID */
    INTERFACE_ID_WEBUSB = 2, /**< WebUSB interface descriptor ID */
};

/** Enum for the device string descriptor IDs within the device. Each string descriptor should
 *  have a unique ID index associated with it, which can be used to refer to the string from
 *  other descriptors.
 */
enum StringDescriptors_t
{
    STRING_ID_Language     = 0, /**< Supported Languages string descriptor ID (must be zero) */
    STRING_ID_Manufacturer = 1, /**< Manufacturer string ID */
    STRING_ID_Product      = 2, /**< Product string ID */
};

/** Type define for the Microsoft OS 2.0 Descriptor for the device. This must be defined in the
 *  application code as the descriptor may contain sub-descriptors which can vary between devices,
 *  and which identify which USB drivers Windows should use.
 */
typedef struct
{
    MS_OS_20_Descriptor_Set_Header_t        Header;
    MS_OS_20_Configuration_Subset_Header    Configuration1;
    MS_OS_20_Function_Subset_Header         CDC_Function;
    MS_OS_20_CompatibleID_Descriptor        CDC_CompatibleID; // USBSER.SYS driver for COM port
    MS_OS_20_Function_Subset_Header         WebUSB_Function;
    MS_OS_20_CompatibleID_Descriptor        WebUSB_CompatibleID; // WINUSB.SYS driver
    MS_OS_20_Registry_Property_Descriptor   WebUSB_RegistryData;
} MS_OS_20_Descriptor_t;

/** Another (simpler) MS OS 2.0 Descriptor when in WebUSB mode.
 */
typedef struct
{
    MS_OS_20_Descriptor_Set_Header_t        Header;
    MS_OS_20_CompatibleID_Descriptor        CompatibleID;
} MS_OS_20_Descriptor_WebUSB_t;

/* Function Prototypes: */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const DescriptorAddress)
                                    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

#endif
