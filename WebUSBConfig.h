/*
             LUFA Library
     Copyright (C) Dean Camera, 2017.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2017  Dean Camera (dean [at] fourwalledcubicle [dot] com)
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


#ifndef __WEB_USB_CONFIG_H__
#define __WEB_USB_CONFIG_H__

#define WEBUSB_VENDOR_CODE 0x42
#define WEBUSB_LANDING_PAGE_INDEX 0

#define MS_OS_20_VENDOR_CODE 0x45     // Must be different than WEBUSB_VENDOR_CODE

#define MS_OS_20_REGISTRY_KEY L"DeviceInterfaceGUIDs" //  20 characters + null, times 2 = 42 bytes
// python -c "import uuid;print('u\"{' + str(uuid.uuid4()) + '}\\\0\"')"
#define MS_OS_20_DEVICE_GUID_STRING_OF_STRING L"{94e78d93-4cbb-481f-b542-a74740d3a713}\0" // 39 characters + null, time 2 = 80 bytes

#define MS_OS_20_DESCRIPTOR_SET_TOTAL_LENGTH (10 + 8 + 8 + 20 + 8 + 20 + 10 + 42 + 80)
#define MS_OS_20_DESCRIPTOR_SET_TOTAL_LENGTH_WEBUSB (10 + 20)

// #define MS_OS_20_ALTERNATE_ENUMERATION_CODE 1 /**< Set to non-zero to enable Windows to allow device to return alternate USB descriptors. */

#define DEFAULT_CONFIG_INDEX 1

#define WEBUSB_ENABLE_BYTE_ADDRESS 0x45     // Must be between 0 and 512

#endif
