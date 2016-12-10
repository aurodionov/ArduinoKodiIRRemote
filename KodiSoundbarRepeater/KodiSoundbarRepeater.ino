/*
 * IR Remote Control for XBMC/Kodi
 * 
 * Send the standard arrow keys/enter/backspace and play/FFWD/rewind based
 * on receipt of commands from a TV remote.
 * 
 * Uses the IRRemote library from
 * http://www.righto.com/2009/08/multi-protocol-infrared-remote-library.html
 * (GitHub: https://github.com/z3t0/Arduino-IRremote)
 * and the guide at http://mitchtech.net/arduino-usb-hid-keyboard/ to set up
 * an Arduino to work as a keyboard.
 * 
 * Specifically works with the oCOSMO CE4031's remote, as is.
 *
 *
 * Made in 2015 by Corban Mailloux
 * corb.co
 */

#include <IRremote.h>

uint8_t buf[8] = { 0 };  /* Keyboard report buffer */

int RECV_PIN = 11;
// Send pin = 3

IRrecv irrecv(RECV_PIN);
IRsend irsend; // Send pin 3

decode_results results;

bool repeatable = false;
bool lastWasKeyboard = false;
unsigned long firstPressTime = 0;
int keyRepeatDelay = 300; // MS to wait before repeating keyboard keys
byte lastKeyboard = 0x00;
unsigned long lastSoundbar = 0x000000;

void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
}

void loop() {
  if (irrecv.decode(&results)) {
    // Serial.println(results.value, HEX); // Debug line
    switch (results.value) {
      case 0xFFFFFFFF: // Repeat code
        if (repeatable) {
          if (lastWasKeyboard) {
            unsigned long now = millis();
            if (now - firstPressTime > keyRepeatDelay) {
              // Serial.println("Sending...");
              setKeyboard(lastKeyboard, false); // Don't reset the repeat delay
            } else {
              // Serial.println("Resume.");
              irrecv.resume();
              return;
            }
          } else {
            sendToSoundbar(lastSoundbar);
            irrecv.resume();
            return;
          }
        }
        break;
      case 0xA50: // Source
        // After pressing the Source button, you have to use the arrow keys.
        // To avoid accidentally picking up those presses as Kodi keys,
        // ignore presses for 5 seconds.
        repeatable = false;
        delay(5000);
        break;
      case 0x2F0: // Up
        repeatable = true;
        setKeyboard(0x52);
        break;
      case 0xAF0: // Down
        repeatable = true;
        setKeyboard(0x51);
        break;
      case 0x2D0: // Left
        repeatable = true;
        setKeyboard(0x50);
        break;
      case 0xCD0: // Right
        repeatable = true;
        setKeyboard(0x4F);
        break;
      case 0xD10: // Enter
        repeatable = false;
        setKeyboard(0x28);
        break;
      case 0x690: // Back
        repeatable = false;
        setKeyboard(0x2A); // Backspace
        break;
      case 0x9B0: // Play/Pause
        repeatable = false;
        setKeyboard(0x2C); // Spacebar
        break;
      case 0x1B0: // Rewind
        repeatable = false;
        setKeyboard(0x15); // R
        break;
      case 0x7B0: // Fast Forward
        repeatable = false;
        setKeyboard(0x09); // F
        break;
      case 0xFB0: // Star (Context Menu)
        repeatable = false;
        setKeyboard(0x06); // C
        break;

      // Soundbar Controls
      case 0xB70: // SoundBar - Power
        repeatable = false;
        sendToSoundbar(0xFF02FD);
        break;
      case 0xCF0: // SoundBar - Volume Down
        repeatable = true;
        sendToSoundbar(0xFFA25D);
        break;
      case 0x4F0: // SoundBar - Volume Up
        repeatable = true;
        sendToSoundbar(0xFF827D);
        break;
      case 0x0F0: // SoundBar - Source
        repeatable = false;
        sendToSoundbar(0xFF22DD); // Input
        break;
      case 0x8F0: // SoundBar - Mode
        repeatable = false;
        sendToSoundbar(0xFF44BB); // Bluetooth mode
        break;
      case 0x5F0: // A - Red
        // Nothing
        repeatable = false;
        break;
      case 0xDF0: // B - Green
        // Nothing
        repeatable = false;
        break;
      case 0x3F0: // C - Yellow
        repeatable = false;
        setKeyboard(0x73); // F24, to trigger the resolution script
        break;
      case 0xBF0: // D - Orange
        // Nothing
        repeatable = false;
        break;

      // Non-matching IR
      default:
        repeatable = false;
        break;
    }

    delay(25);
    irrecv.resume(); // Receive the next value
  }
}

void releaseKey() 
{
  buf[0] = 0;
  buf[2] = 0;
  Serial.write(buf, 8); // Release key
}

void setKeyboard(byte value)
{
  setKeyboard(value, true); // Default mode
}

void setKeyboard(byte value, bool isfirstPress)
{
  if (isfirstPress) {
    firstPressTime = millis();
  }
  lastWasKeyboard = true;
  lastKeyboard = value;
  buf[2] = value;
  Serial.write(buf, 8); // Send keypress
  releaseKey();
}

void sendToSoundbar(unsigned long value)
{
  irsend.sendNEC(value, 32);
  lastSoundbar = value;
  delay(50);
  lastWasKeyboard = false; // Reset
  irrecv.enableIRIn();
}
