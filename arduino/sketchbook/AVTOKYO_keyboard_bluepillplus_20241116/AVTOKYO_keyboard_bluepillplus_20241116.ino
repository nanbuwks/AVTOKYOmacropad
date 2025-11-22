constexpr int PIN_LED_BUILTIN = PC13;
constexpr int PIN_SWITCH_0 = PB0;
constexpr int PIN_SWITCH_1 = PB1;
constexpr int PIN_SWITCH_2 = PB3;
constexpr int PIN_SWITCH_3 = PB5;
constexpr int PIN_SWITCH_4 = PA4;
constexpr int PIN_SWITCH_5 = PA6;
constexpr int PIN_RE1A = PB13;
constexpr int PIN_RE1B = PB14;
constexpr int PIN_RE2A = PB0;
constexpr int PIN_RE2B = PB1;

// 
#include <RotaryEncoder.h>

RotaryEncoder encoder1(PIN_RE1A, PIN_RE1B, RotaryEncoder::LatchMode::FOUR3);
RotaryEncoder encoder2(PIN_RE2A, PIN_RE2B, RotaryEncoder::LatchMode::FOUR3);


#include <Adafruit_TinyUSB.h>

// HIDレポートIDの定義。機能ごとにIDを割り振り、1つのUSBエンドポイントで複数のHID機能をやりとりする
enum {
  RID_KEYBOARD = 1,
  RID_MOUSE,
  RID_CONSUMER_CONTROL, // メディア操作やボリュームコントロールなど
};

// TinyUSBのテンプレートを利用して、HIDレポートディスクリプタを構築する
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE   (HID_REPORT_ID(RID_MOUSE)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(RID_CONSUMER_CONTROL))
};

Adafruit_USBD_HID usb_hid;


// USB HIDの機能をループごとにひとつずつ処理するため、各ステージに番号を割り振る
typedef enum {
    STAGE_KEYBOARD,
    STAGE_MOUSE,
    STAGE_CONSUMER_1,
    STAGE_CONSUMER_2,
    STAGE_COUNT
} STAGE_t;


void setup() {
    // スイッチの入力ピンの初期化。ロータリーエンコーダーのピンはライブラリ側で処理される
    pinMode(PIN_SWITCH_0, INPUT_PULLUP);
    pinMode(PIN_SWITCH_1, INPUT_PULLUP);
    pinMode(PIN_SWITCH_2, INPUT_PULLUP);
    pinMode(PIN_SWITCH_3, INPUT_PULLUP);
    pinMode(PIN_SWITCH_4, INPUT_PULLUP);
    pinMode(PIN_SWITCH_5, INPUT_PULLUP);

    // USB HIDの設定
    usb_hid.setPollInterval(2);
    usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
    usb_hid.setStringDescriptor("TinyUSB HID Composite");

    usb_hid.begin();
}


void loop() {
  // USB通信の処理
  TinyUSBDevice.task();
  
  if (TinyUSBDevice.mounted()) {
        // USBが接続されているとき

        led_blink(500);

        static STAGE_t stage = 0;

        static long encoder1_position = 0;
        static long encoder2_position = 0;

        // エンコーダーの位置はとりこぼさないように毎回更新する
        encoder1.tick();
        encoder2.tick();

        int dir1 = encoder1.getPosition() - encoder1_position;
        int dir2 = encoder2.getPosition() - encoder2_position;
        bool switch_1 = digitalRead(PIN_SWITCH_0) == LOW;
        bool switch_2 = digitalRead(PIN_SWITCH_1) == LOW;
        bool switch_3 = digitalRead(PIN_SWITCH_2) == LOW;
        bool switch_4 = digitalRead(PIN_SWITCH_3) == LOW;
        bool switch_5 = digitalRead(PIN_SWITCH_4) == LOW;
        bool switch_6 = digitalRead(PIN_SWITCH_5) == LOW;

        if (usb_hid.ready()) {
            // 直前のHIDレポートの送信が完了したら、次のステージを処理する

            switch (stage) {
                // HIDの機能ごとにHIDレポートを組み立てて、送信する

                case STAGE_KEYBOARD:
                    // キーボード
                    {
                        uint8_t modifiers = 0;
                        uint8_t keycode[6] = { 0 };
                        keycode[0] = switch_1 ? 0x04 : 0x00;
                        keycode[1] = switch_2 ? 0x05 : 0x00;
                        keycode[2] = switch_3 ? 0x06 : 0x00;
                        keycode[3] = switch_4 ? 0x07 : 0x00;
                        keycode[4] = switch_5 ? 0x08 : 0x00;

                        usb_hid.keyboardReport(RID_KEYBOARD, modifiers, keycode);
                    }
                    stage = (stage + 1) % STAGE_COUNT;
                    break;

                case STAGE_MOUSE:
                    // マウス
                    {
                        int8_t scroll = 0;
                        uint8_t pan = 0;
                        if (dir1 > 0) {
                            scroll = 4;
                        } else if (dir1 < 0) {
                            scroll = -4;
                        }
                        
                        encoder1_position = encoder1.getPosition();
                        usb_hid.mouseScroll(RID_MOUSE, scroll, pan);
                    }
                    stage = (stage + 1) % STAGE_COUNT;
                    break;

                case STAGE_CONSUMER_1:
                    // コンシューマーコントロール
                    {
                        static unsigned long timer = 0;
                        // static muteswitch_pressed = false;
                        if (millis() - timer > 50) {
                            timer = millis();

                            uint16_t functions = 0;
                            if (switch_6) {
                                functions |= HID_USAGE_CONSUMER_MUTE;
                                // muteswitch_pressed = true;
                            }
                            if (dir2 > 0) {
                                functions |= HID_USAGE_CONSUMER_VOLUME_DECREMENT;
                            } else if (dir2 < 0) {
                                functions |= HID_USAGE_CONSUMER_VOLUME_INCREMENT;
                            }
                            encoder2_position = encoder2.getPosition();

                            usb_hid.sendReport16(RID_CONSUMER_CONTROL, functions);

                            while (digitalRead(PIN_SWITCH_5) == LOW) { delay(1); }
                        }
                        stage = (stage + 1) % STAGE_COUNT;
                        break;
                    }
                
                case STAGE_CONSUMER_2:  // コンシューマーコントロールをすべて解除する
                    usb_hid.sendReport16(RID_CONSUMER_CONTROL, 0x0000);
                    stage = (stage + 1) % STAGE_COUNT;
                    break;

                default:
                    stage = STAGE_KEYBOARD;
            }
        }

    } else {
        // 電源は供給されているがUSBが接続されていないとき（デバッグ用）

        led_blink(1000);
    }
}


void led_blink(unsigned int interval_ms) {
    static unsigned long timer = 0;
    static bool led_on = false;

    if (timer == 0) {
        pinMode(PIN_LED_BUILTIN, OUTPUT);
    }

    if (millis() - timer > interval_ms) {
        timer = millis();
        
        digitalWrite(PIN_LED_BUILTIN, led_on ? LOW : HIGH);
        led_on = !led_on;
    }
}
