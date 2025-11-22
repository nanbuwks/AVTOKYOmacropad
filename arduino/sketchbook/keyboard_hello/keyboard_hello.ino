#include "Adafruit_TinyUSB.h"

uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_KEYBOARD()
};

Adafruit_USBD_HID usb_hid;

uint8_t hidcode[] = {HID_KEY_H, HID_KEY_E, HID_KEY_L, HID_KEY_L, HID_KEY_O};

void setup() {
  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }

  usb_hid.setBootProtocol(HID_ITF_PROTOCOL_KEYBOARD);
  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setStringDescriptor("TinyUSB Keyboard");

//h  usb_hid.setReportCallback(NULL, hid_report_callback);

  usb_hid.begin();

  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }

  // led pin
}

void process_hid() {
}

void loop() {
#ifdef TINYUSB_NEED_POLLING_TASK
  // Manual call tud_task since it isn't called by Core's background
  TinyUSBDevice.task();
#endif

  // not enumerated()/mounted() yet: nothing to do
  if (!TinyUSBDevice.mounted()) {
    return;
  }
  uint8_t const report_id = 0;
  uint8_t const modifier = 0;
  static bool keyPressedPreviously = false;

  uint8_t count = 0;
  uint8_t keycode[6] = {0};

  // scan normal key and send report
  for (uint8_t i = 0; i < 5; i++) {
    keycode[0] = hidcode[i];
    usb_hid.keyboardReport(report_id, modifier, keycode);
    delay(100);
    usb_hid.keyboardRelease(0);
    delay(100);
  }
  delay(1000);
}
