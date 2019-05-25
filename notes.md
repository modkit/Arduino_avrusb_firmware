USB Notes
=========

## Descriptors

###### USB Endpoint
The actual numerical address used for various USB communication. 16U2s only support 4 total endpoints.

###### USB Interface
Generally, a unit of functionality on a USB device. Most classes use multiple endpoints on a single interface.
USB CDC-ACM (Virtual Serial) class devices break this assumption by utilizing two interfaces.

###### Composite Device
A USB device with multiple interfaces serving different functions.

###### Interface Association Descriptor
A special descriptor added to USB to allow Composite Devices where one of the units of functionality uses
multiple interfaces, such as CDC-ACM. This allows multiple interfaces to be logically grouped together.

##### Windows Compatibility

###### MS OS 20 Descriptor Set
A set of custom Descriptors used to control device behavior on Windows, including selecting drivers to load as well as 
setting registry values. These are retrieved via a vendor-specific control request, must like the WebUSB descriptor.

###### BOS Descriptor
Windows looks for a Platform Capability BOS Descriptor which defines the size of the MS OS 20 Descriptor Set, along
with a magic byte to identify the type of request (to differentiate from other vendor requests).

## Windows Access Strategy
Windows composite device driver (USBCCGP) doesn't seem to handle alternate interface definitions.
That driver also cannot change USB configuration values, and neither can WinUSB, the driver used by Chrome 
(per [Multi-config USB devices and Windows](https://techcommunity.microsoft.com/t5/Microsoft-USB-Blog/Multi-config-USB-devices-and-Windows/ba-p/270702)).

Thus, the only viable solution for Windows is to have the device masquerade as two separate USB devices,
each with their own set of descriptors.
Because of registry settings, this also necessitates two distinct sets of VID/PIDs.

The 'two devices' share the same endpoint IDs for functionality, meaning the actual business logic of marshalling data
between USB-serial and UART remains unchanged. The only differences in functionality are the USB descriptors.
State of which 'version' of the descriptors to expose is stored in EEPROM and updated via a new WebUSB control request:

```$js
device.controlTransferOut({
    'requestType': 'vendor',
    'recipient': 'device',
    'request': 0x42,    // Device-specific ID, provided via BOS descriptor. (See WebUSB spec for details)
    'value': 1,         // 1 to enable WebUSB descriptors, 0 for default USB-serial descriptors
    'index': 3          // New WebUSB request code
})
```

When in 'default' USB-serial mode, there is an additional USB interface (#2) with no endpoints, created solely to be
exposed via the WINUSB driver on Windows, which enables Chrome on Windows to see the device in the first place.
When in 'WebUSB' mode, all three endpoints are under a single interface (#0). If, in Chrome, there are 3 interfaces,
the device is in USB-serial mode; if there's a single interface, it's in WebUSB mode.

Flashing Firmware
-----------------

Arduino provides instructions [here](https://www.arduino.cc/en/Hacking/DFUProgramming8U2).

Instead of their firmware, use `arduino-usbserial/Arduino-usbserial.hex`

After flashing, the device needs to be un- & re-plugged to switch back to normal operation.

##### Quick OSX/Linux reference

Install `dfu-programmer`

Put the device in DFU mode by shorting reset & ground pins.
 
Run `$ dfu-programmer atmega16u2 flash arduino-usbserial/Arduino-usbserial.hex`

###### Building Firmware

The makefile needs to know what device to build for: `$ ARDUINO_MODEL_PID=0x0043 make`

Initializing Connection
-----------------------

#### WebUSB

    let device = await navigator.usb.requestDevice({ filters:[] });

To filter by VID/PID: `requestDevice({ filters:[{ vendorId: 0x04B9, productId: 0x0AF1 }] });`

    await device.open();
    if ( device.configuration === null ) {
        await device.selectConfiguration(1);
    }
    
When in USB-serial mode, the device has 3 interfaces, and we need to claim #2. When in WebUSB mode, the device has one
interface, #0, which we need to claim.

    await device.claimInterface(device.configuration.interfaces.length - 1);
    
If device isn't in WebUSB mode, put it in WebUSB mode. Note this causes the connection to stall out & requires that the
user physically disconnect and reconnect the device (due to OS security features).

    if ( device.configuration.interfaces.length === 3 ) {
        device.controlTransferOut({
            'requestType': 'vendor',
            'recipient': 'device',
            'request': 0x42,    // Device-specific ID, provided via BOS descriptor. (See WebUSB spec for details)
            'value': 1,         // 1 to enable WebUSB descriptors, 0 for default USB-serial descriptors
            'index': 3          // New WebUSB request code
        });     // This promise will likely throw an error.
        // Handle reconnecting to a (logically) new device.
    }

#### USB-serial

Despite USB being able to communicate fully with the device, establishing a USB-serial connection still requires
sending USB commands to configure a virtual serial device.

1. Set line config:

        let data_view = new DataView(new ArrayBuffer(7));
        data_view.setUint32(0, 115200, true);   // Baud rate
        data_view.setUint8(4, 0);               // 1 stop bit
        data_view.setUint8(5, 0);               // No parity bits
        data_view.setUint8(6, 8);               // 8 data bits

        await device.controlTransferOut({
            'requestType': 'class',
            'recipient': 'interface',
            'request': 0x20, //Set Line Coding
            'value': 0,
            'index': 0
        }, data_view.buffer);

2. Set control line state:

        await device.controlTransferOut({
            'requestType': 'class',
            'recipient': 'interface',
            'request': 0x22, //Set Control Line State
            'value': 3,
            'index': 0
        });
        
    * 'value' byte, bit 0: DTR (1 is high)
    * 'value' byte, bit 1: Carrier (1 is active)

Echo Test
---------

Flash `serial_echo.ino.hex` onto the Atmega328.

```$js
function receiver({device, interface, size}) {
    return async function() {
   
    const result = await device.transferIn(interface, size);
    if ( result.status !== "ok" ) {
        throw new Error(`Transfer in failed with statuts: ${result.status}`);
    }
    const data_view = result.data;
    return Array.from(new Uint8Array(data_view.buffer))
}}

async function poll(fn) {
    console.log(await fn());
    setTimeout(() => poll(fn), 0);
}

poll(receiver({device, interface:3, size:64}));

await device.transferOut(2, Uint8Array.from([1, 2, 3, 4]).buffer);
```
