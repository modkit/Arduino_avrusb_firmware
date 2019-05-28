/*
             LUFA Library
     Copyright (C) Dean Camera, 2015.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2015  Dean Camera (dean [at] fourwalledcubicle [dot] com)
  Copyright 2019  Benjamin Riggs (https://github.com/riggs)
  Copyright 2019  Modkit Inc. (open [at] modkit [dot] com)

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
 *  Main source file for the Arduino-usbserial project. This file contains the main tasks of
 *  the project and is responsible for the initial application hardware configuration.
 *
 *  Alex Holden <alex@linuxhacker.org>: Ported Arduino changes forward from
 *  LUFA 100807 USBtoSerial to LUFA 151115 USBtoSerial in order to fix data
 *  corruption at 250000 baud. Activity LEDs are now turned off by a timer compare
 *  ISR because we are no longer polling the timer in the main event loop.
 */

#include "Arduino-usbserial.h"

/** Toggle for WebUSB endpoints enabled from host. */
extern uint8_t WebUSB_Enabled;

/** Circular buffer to hold data from the host before it is sent to the device via the serial port. */
static RingBuffer_t USBtoUSART_Buffer;

/** Underlying data buffer for \ref USBtoUSART_Buffer, where the stored bytes are located. */
static uint8_t      USBtoUSART_Buffer_Data[128];

/** Circular buffer to hold data from the serial port before it is sent to the host. */
static RingBuffer_t USARTtoUSB_Buffer;

/** Underlying data buffer for \ref USARTtoUSB_Buffer, where the stored bytes are located. */
static uint8_t      USARTtoUSB_Buffer_Data[128];

/** Pulse generation counters to keep track of the number of 1/100 second remaining for each pulse type */
static volatile uint8_t TxLEDPulseTimer;
static volatile uint8_t RxLEDPulseTimer;

/** Helper function to reset the device when switching between CDC and WebUSB modes */
void resetDeviceAfterTimeout(int timeout_ms)
  {
    /* disable usb and interrupts */
    USB_Disable();
    cli(); 

    /* wait timeout_ms for usb to properly disconnect and then start the watchdog */
    Delay_MS(timeout_ms);
    wdt_enable(WDTO_250MS);

    /* busy wait until watchdog resets the mcu */
    while(1);
  }


/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
const USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
  {
    .Config =
      {
        .ControlInterfaceNumber         = INTERFACE_ID_CDC_CCI,
        .DataINEndpoint                 =
          {
            .Address                = CDC_TX_EPADDR,
            .Size                   = CDC_TXRX_EPSIZE,
            .Banks                  = 1,
          },
        .DataOUTEndpoint                =
          {
            .Address                = CDC_RX_EPADDR,
            .Size                   = CDC_TXRX_EPSIZE,
            .Banks                  = 1,
          },
        .NotificationEndpoint           =
          {
            .Address                = CDC_NOTIFICATION_EPADDR,
            .Size                   = CDC_NOTIFICATION_EPSIZE,
            .Banks                  = 1,
          },
      },
  };


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
  SetupHardware();

  WebUSB_Enabled = eeprom_read_byte((uint8_t *) WEBUSB_ENABLE_BYTE_ADDRESS) & 1;

  RingBuffer_InitBuffer(&USBtoUSART_Buffer, USBtoUSART_Buffer_Data, sizeof(USBtoUSART_Buffer_Data));
  RingBuffer_InitBuffer(&USARTtoUSB_Buffer, USARTtoUSB_Buffer_Data, sizeof(USARTtoUSB_Buffer_Data));

  GlobalInterruptEnable();

  for (;;)
  {
    /* Only try to read in bytes from the CDC interface if the transmit buffer is not full */
    if (!(RingBuffer_IsFull(&USBtoUSART_Buffer)))
    {
      int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

      /* Store received byte into the USART transmit buffer */
      if (!(ReceivedByte < 0))
        RingBuffer_Insert(&USBtoUSART_Buffer, ReceivedByte);
    }

    uint16_t BufferCount = RingBuffer_GetCount(&USARTtoUSB_Buffer);
    if (BufferCount)
    {
      Endpoint_SelectEndpoint(VirtualSerial_CDC_Interface.Config.DataINEndpoint.Address);

      /* Check if a packet is already enqueued to the host - if so, we shouldn't try to send more data
       * until it completes as there is a chance nothing is listening and a lengthy timeout could occur */
      if (Endpoint_IsINReady())
      {
        /* There is data from the UART waiting to be sent to the host, so switch
         * the TX LED on and restart the pulse timer: */
        LEDs_TurnOnLEDs(LEDMASK_TX);
        TxLEDPulseTimer = TX_RX_LED_PULSE_MS;

        /* Never send more than one bank size less one byte to the host at a time, so that we don't block
         * while a Zero Length Packet (ZLP) to terminate the transfer is sent if the host isn't listening */
        uint8_t BytesToSend = MIN(BufferCount, (CDC_TXRX_EPSIZE - 1));

        /* Read bytes from the USART receive buffer into the USB IN endpoint */
        while (BytesToSend--)
        {
          /* Try to send the next byte of data to the host, abort if there is an error without dequeuing */
          if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface,
                      RingBuffer_Peek(&USARTtoUSB_Buffer)) != ENDPOINT_READYWAIT_NoError)
          {
            break;
          }

          /* Dequeue the already sent byte from the buffer now we have confirmed that no transmission error occurred */
          RingBuffer_Remove(&USARTtoUSB_Buffer);
        }
      }
    }

    /* Load the next byte from the USART transmit buffer into the USART if transmit buffer space is available */
    if (Serial_IsSendReady() && !(RingBuffer_IsEmpty(&USBtoUSART_Buffer))) {
        LEDs_TurnOnLEDs(LEDMASK_RX);
        RxLEDPulseTimer = TX_RX_LED_PULSE_MS;
        Serial_SendByte(RingBuffer_Remove(&USBtoUSART_Buffer));
    }

    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
    USB_USBTask();
  }
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
#if (ARCH == ARCH_AVR8)
  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  /* Disable clock division */
  clock_prescale_set(clock_div_1);

#endif

  /* Hardware Initialization */
  LEDs_Init();
  USB_Init();

  /* Set up Timer 0 to give us a compare match interrupt at 1KHz so we can
   * turn off the TX/RX LEDs in the after an appropriate number of ms to
   * make the pulses visible. NB. 16MHz with /64 prescaler = 250KHz. */
  TCCR0B = (1 << CS01 | 1 << CS00);
  TCCR0A = 1 << WGM01;
  TCNT0 = 0;
  OCR0A = 249;
  TIMSK0 = (1 << OCIE0A);

  /* Pull target /RESET line high */
  AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
  AVR_RESET_LINE_DDR  |= AVR_RESET_LINE_MASK;
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
  CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

/** Event handler for the USB_Disconnect event. This indicates the device is no longer connected to the host and the
 *  device should revert to default interfaces.
 */
void EVENT_USB_Device_Disconnect(void)
{
  Endpoint_ClearSETUP();
  Endpoint_ClearStatusStage();
    CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

/** Microsoft OS 2.0 Descriptor. This is used by Windows to select the USB driver for the device.
 *
 *  For WebUSB in Chrome, the correct driver is WinUSB, which is selected via CompatibleID.
 *
 *  Additionally, while Chrome is built using libusb, a magic registry key needs to be set containing a GUID for
 *  the device.
 */
const MS_OS_20_Descriptor_t PROGMEM MS_OS_20_Descriptor =
  {
    .Header =
      {
        .Length = CPU_TO_LE16(10),
        .DescriptorType = CPU_TO_LE16(MS_OS_20_SET_HEADER_DESCRIPTOR),
        .WindowsVersion = MS_OS_20_WINDOWS_VERSION_8_1,
        .TotalLength = CPU_TO_LE16(MS_OS_20_DESCRIPTOR_SET_TOTAL_LENGTH)
      },

    .Configuration1 =
      {
        .Length = CPU_TO_LE16(8),
        .DescriptorType = CPU_TO_LE16(MS_OS_20_SUBSET_HEADER_CONFIGURATION),
        .ConfigurationValue = 1,
        .Reserved = 0,
        .TotalLength = CPU_TO_LE16(8 + 8 + 20)
      },

    .CDC_Function =
      {
        .Length = CPU_TO_LE16(8),
        .DescriptorType = CPU_TO_LE16(MS_OS_20_SUBSET_HEADER_FUNCTION),
        .FirstInterface = INTERFACE_ID_CDC_CCI,
        .Reserved = 0,
        .SubsetLength = CPU_TO_LE16(8 + 20)
      },

    .CDC_CompatibleID =
      {
        .Length = CPU_TO_LE16(20),
        .DescriptorType = CPU_TO_LE16(MS_OS_20_FEATURE_COMPATBLE_ID),
        .CompatibleID = u8"USBSER\x00", // Automatically null-terminated to 8 bytes
        .SubCompatibleID = {0, 0, 0, 0, 0, 0, 0, 0}
      },

    .WebUSB_Function =
      {
        .Length = CPU_TO_LE16(8),
        .DescriptorType = CPU_TO_LE16(MS_OS_20_SUBSET_HEADER_FUNCTION),
        .FirstInterface = INTERFACE_ID_WEBUSB,
        .Reserved = 0,
        .SubsetLength = CPU_TO_LE16(8 + 20 + 10 + 42 + 80)
      },

    .WebUSB_CompatibleID =
      {
        .Length = CPU_TO_LE16(20),
        .DescriptorType = CPU_TO_LE16(MS_OS_20_FEATURE_COMPATBLE_ID),
        .CompatibleID = u8"WINUSB\x00", // Automatically null-terminated to 8 bytes
        .SubCompatibleID = {0, 0, 0, 0, 0, 0, 0, 0}
      },

    .WebUSB_RegistryData =
      {
        .Length = CPU_TO_LE16(10 + 42 + 80),
        .DescriptorType = CPU_TO_LE16(MS_OS_20_FEATURE_REG_PROPERTY),
        .PropertyDataType = CPU_TO_LE16(MS_OS_20_REG_MULTI_SZ),
        .PropertyNameLength = CPU_TO_LE16(sizeof(MS_OS_20_REGISTRY_KEY)),
        .PropertyName = MS_OS_20_REGISTRY_KEY, // 42 bytes
        .PropertyDataLength = CPU_TO_LE16(sizeof(MS_OS_20_DEVICE_GUID_STRING_OF_STRING)),
        .PropertyData = MS_OS_20_DEVICE_GUID_STRING_OF_STRING // 82 bytes
      }
  };

const MS_OS_20_Descriptor_WebUSB_t PROGMEM MS_OS_20_Descriptor_WebUSB =
    {
        .Header =
            {
                .Length = CPU_TO_LE16(10),
                .DescriptorType = CPU_TO_LE16(MS_OS_20_SET_HEADER_DESCRIPTOR),
                .WindowsVersion = MS_OS_20_WINDOWS_VERSION_8_1,
                .TotalLength = CPU_TO_LE16(MS_OS_20_DESCRIPTOR_SET_TOTAL_LENGTH_WEBUSB)
            },

        .CompatibleID =
            {
                .Length = CPU_TO_LE16(20),
                .DescriptorType = CPU_TO_LE16(MS_OS_20_FEATURE_COMPATBLE_ID),
                .CompatibleID = u8"WINUSB\x00", // Automatically null-terminated to 8 bytes
                .SubCompatibleID = {0, 0, 0, 0, 0, 0, 0, 0}
            }
    };

/** URL descriptor string. This is a UTF-8 string containing a URL excluding the prefix. At least one of these must be
 * 	defined and returned when the Landing Page descriptor index is requested.
 */
const WebUSB_URL_Descriptor_t PROGMEM WebUSB_LandingPage = WEBUSB_URL_DESCRIPTOR(1, u8"www.entropicengineering.com");


/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
  switch (USB_ControlRequest.bmRequestType) {
    /* Set Interface is not handled by the library, as its function is application-specific */
    /* Handle Vendor Requests for WebUSB & MS OS 20 Descriptors */
    case (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE):
      /* Free the endpoint for the next Request */
      switch (USB_ControlRequest.bRequest) {
        case WEBUSB_VENDOR_CODE:
          switch (USB_ControlRequest.wIndex) {
            case WebUSB_RTYPE_GetURL:
              switch (USB_ControlRequest.wValue) {
                case WEBUSB_LANDING_PAGE_INDEX:
                                    Endpoint_ClearSETUP();
                  /* Write the descriptor data to the control endpoint */
                  Endpoint_Write_Control_PStream_LE(&WebUSB_LandingPage, WebUSB_LandingPage.Header.Size);
                  /* Release the endpoint after transaction. */
                                    Endpoint_ClearStatusStage();
                  break;
                default:    /* Stall transfer on invalid index. */
                  Endpoint_StallTransaction();
                  break;
              }
              break;
            default:    /* Stall on unknown WebUSB request */
              Endpoint_StallTransaction();
              break;
          }
          break;
        case MS_OS_20_VENDOR_CODE:
          switch (USB_ControlRequest.wIndex) {
            case MS_OS_20_DESCRIPTOR_INDEX:
                            Endpoint_ClearSETUP();
              /* Write the descriptor data to the control endpoint */
              if (WebUSB_Enabled) {
                                Endpoint_Write_Control_PStream_LE(&MS_OS_20_Descriptor_WebUSB, MS_OS_20_Descriptor_WebUSB.Header.TotalLength);
              } else {
                                Endpoint_Write_Control_PStream_LE(&MS_OS_20_Descriptor, MS_OS_20_Descriptor.Header.TotalLength);
              }
              /* Release the endpoint after transaction. */
                            Endpoint_ClearStatusStage();
              break;
            case MS_OS_20_SET_ALT_ENUMERATION:
              switch (USB_ControlRequest.wValue) {
                case (MS_OS_20_ALTERNATE_ENUMERATION_CODE << 8):	// High byte
                                    Endpoint_ClearSETUP();
                  // Do something with alternate interface settings
                                    break;
                                default:	/* Unknown AltEnumCode */
                                    Endpoint_StallTransaction();
              }
            default:    /* Stall on unknown MS OS 2.0 request */
              Endpoint_StallTransaction();
              break;
          }
          break;
        default:    /* Stall on unknown bRequest / Vendor Code */
          Endpoint_StallTransaction();
          break;
      }
      break;
        case (REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_DEVICE):
            /* Free the endpoint for the next Request */
            switch (USB_ControlRequest.bRequest) {
                case WEBUSB_VENDOR_CODE:
                    switch (USB_ControlRequest.wIndex) {
                        case WebUSB_RTYPE_Enable:
                            Endpoint_ClearSETUP();
                            /* Update state, if necessary */
                            if (WebUSB_Enabled != USB_ControlRequest.wValue & 1) {
                                WebUSB_Enabled = USB_ControlRequest.wValue & 1;
                                eeprom_write_byte((uint8_t *) WEBUSB_ENABLE_BYTE_ADDRESS, WebUSB_Enabled);
                                Endpoint_ClearStatusStage();
                                resetDeviceAfterTimeout(2000);
                            } else {
                                Endpoint_ClearStatusStage();
                            }
                        default:    /* Stall on unknown MS OS 2.0 request */
                            Endpoint_StallTransaction();
                            break;
                    }
                    break;
                default:    /* Stall on unknown bRequest / Vendor Code */
                    Endpoint_StallTransaction();
                    break;
            }
            break;
    default:
      CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
      break;
  }
}

/** ISR to turn off the TX/RX LEDs after an appropriate delay. */
ISR(TIMER0_COMPA_vect)
{
  if (TxLEDPulseTimer && !(--TxLEDPulseTimer))
    LEDs_TurnOffLEDs(LEDMASK_TX);

  /* Turn off RX LED(s) once the RX pulse period has elapsed */
  if (RxLEDPulseTimer && !(--RxLEDPulseTimer))
    LEDs_TurnOffLEDs(LEDMASK_RX);
}

/** ISR to manage the reception of data from the serial port, placing received bytes into a circular buffer
 *  for later transmission to the host.
 */
ISR(USART1_RX_vect, ISR_BLOCK)
{
  uint8_t ReceivedByte = UDR1;

  /* Drop incoming data if the USB device isn't configured or the ring buffer is already full. */
  if ((USB_DeviceState == DEVICE_STATE_Configured) && !(RingBuffer_IsFull(&USARTtoUSB_Buffer)))
    RingBuffer_Insert(&USARTtoUSB_Buffer, ReceivedByte);
}

/** Event handler for the CDC Class driver Line Encoding Changed event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
  uint8_t ConfigMask = 0;

  switch (CDCInterfaceInfo->State.LineEncoding.ParityType)
  {
    case CDC_PARITY_Odd:
      ConfigMask = ((1 << UPM11) | (1 << UPM10));
      break;
    case CDC_PARITY_Even:
      ConfigMask = (1 << UPM11);
      break;
  }

  if (CDCInterfaceInfo->State.LineEncoding.CharFormat == CDC_LINEENCODING_TwoStopBits)
    ConfigMask |= (1 << USBS1);

  switch (CDCInterfaceInfo->State.LineEncoding.DataBits)
  {
    case 6:
      ConfigMask |= (1 << UCSZ10);
      break;
    case 7:
      ConfigMask |= (1 << UCSZ11);
      break;
    case 8:
      ConfigMask |= ((1 << UCSZ11) | (1 << UCSZ10));
      break;
  }

  /* Keep the TX line held high (idle) while the USART is reconfigured */
  PORTD |= (1 << 3);

  /* Must turn off USART before reconfiguring it, otherwise incorrect operation may occur */
  UCSR1B = 0;
  UCSR1A = 0;
  UCSR1C = 0;

  /* Set the new baud rate before configuring the USART */
  /* Special case 57600 baud for compatibility with the ATmega328 bootloader. */
  UBRR1  = (CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 57600 || CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 300)
      ? SERIAL_UBBRVAL(CDCInterfaceInfo->State.LineEncoding.BaudRateBPS)
      : SERIAL_2X_UBBRVAL(CDCInterfaceInfo->State.LineEncoding.BaudRateBPS);

  /* Reconfigure the USART in double speed mode for a wider baud rate range at the expense of accuracy */
  UCSR1C = ConfigMask;
  UCSR1A = (CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 57600 || CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 300) ? 0 : (1 << U2X1);
  UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));

  /* Release the TX line after the USART has been reconfigured */
  PORTD &= ~(1 << 3);
}

/** Event handler for the CDC Class driver Host-to-Device Line Encoding Changed event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 * Connects the Arduino reset line to the DTR signal.
 */
void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
  bool CurrentDTRState = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR);

  if (CurrentDTRState)
    AVR_RESET_LINE_PORT &= ~AVR_RESET_LINE_MASK;
  else
    AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
}
