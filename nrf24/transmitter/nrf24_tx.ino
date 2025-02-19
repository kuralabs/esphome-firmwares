#include <ArduinoLowPower.h>
#include <SPI.h>
#include <RF24.h>
#include <FastLED.h>

// NOTE TO FUTURE SELF:
// The Adafruit Feather M0 Express, when reaching deep sleep, needs to
// be put manually in upload/programming mode by pressing twice the reset button
// until the built in led breaths. Then upload will work.


// User interaction pins
#define BUTTON_1 A1
#define BUTTON_2 A2
#define BUTTON_3 A5
#define DEBOUNCE_TIME 200  // ms

// Pixel configuration
#define PIXEL 8

CRGB leds[1];

// NRF24 configuration
#define CE_PIN 6
#define CSN_PIN 5
#define MAX_PAYLOAD 32

const uint8_t PIPE_ADDRESS[] = { 0xDE, 0xAD, 0xC0, 0xDE, 0x01 };
const char ENCRYPTION_KEY[] = "REDACTED PUT 64 CHAR HEX STRING";

RF24 radio(CE_PIN, CSN_PIN);
uint8_t xor_key[MAX_PAYLOAD];


volatile uint8_t button_pressed = 0;


// Convert HEX string to byte array & validate format
bool load_key() {
  size_t key_length = strlen(ENCRYPTION_KEY);

  // Ensure key length is exactly 2 * MAX_PAYLOAD (each byte = 2 hex chars)
  if (key_length != 2 * MAX_PAYLOAD) {
    Serial.println("Error: ENCRYPTION_KEY length is incorrect!");
    return false;
  }

  for (size_t i = 0; i < MAX_PAYLOAD; i++) {
    char hex_byte[3] = {ENCRYPTION_KEY[i * 2], ENCRYPTION_KEY[i * 2 + 1], '\0'};

    // Validate hex characters (0-9, A-F)
    if (!isxdigit(hex_byte[0]) || !isxdigit(hex_byte[1])) {
        Serial.print("Error: Invalid hex character detected: ");
        Serial.println(hex_byte);
        return false;
    }

    xor_key[i] = strtoul(hex_byte, NULL, 16); // Convert hex pair to byte
  }

  return true;
}


void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  while (!Serial);  // Wait for Serial to initialize
  Serial.println("Initializing ...");
  Serial.println("[READY] Serial");

  // Initialize pixel
  FastLED.addLeds<NEOPIXEL, PIXEL>(leds, 1);
  FastLED.clear();
  FastLED.show();
  Serial.println("[READY] Pixel");

  if (!load_key()) {
    while (true) {
      Serial.println("Error: Key loading failed!");
      delay(10000);
    }
  }
  Serial.println("[READY] Encryption Key");

  // Set buttons as INPUT_PULLUP to enable internal pull-ups
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  Serial.println("[READY] I/O");

  // Initialize NRF24 radio
  if (!radio.begin()) {
    while (true) {
      Serial.println("Error: NRF24 Initialization Failed!");
      delay(10000);
    }
  }

  radio.openWritingPipe(PIPE_ADDRESS);  // Set NRF24 address
  radio.setPALevel(RF24_PA_LOW);        // Set power level
  radio.stopListening();                // Set NRF24 to transmit mode
  radio.powerDown();                    // Low power mode, will wake up when needed
  Serial.println("[READY] Radio");

  // Attach interrupts for waking up the microcontroller
  LowPower.attachInterruptWakeup(BUTTON_1, isr_button_1, FALLING);
  LowPower.attachInterruptWakeup(BUTTON_2, isr_button_2, FALLING);
  LowPower.attachInterruptWakeup(BUTTON_3, isr_button_3, FALLING);
  Serial.println("[READY] ISR");

  Serial.println("[DONE]");
}


void loop() {
  // Process button presses
  if (button_pressed > 0) {
    // Cache the button that triggered the wake
    uint8_t handling_button = button_pressed;

    // Notify user of button pressed
    leds[0] = CRGB::Blue;
    FastLED.show();

    // Debounce
    delay(DEBOUNCE_TIME);

    // Reset button identifier after debouncing
    button_pressed = 0;

    // Debug button pressed
    Serial.print("Button pressed: ");
    Serial.println(handling_button);
    Serial.flush();

    // Send message
    if (send(handling_button)) {
      Serial.println("Message sent successfully!");
      leds[0] = CRGB::Green;
    } else {
      Serial.println("Error: Message Sending Failed!");
      leds[0] = CRGB::Red;
    }
    FastLED.show();

    // Notify user button has being handled
    delay(1000);
    FastLED.clear();
    FastLED.show();
  }

  // Put the microcontroller in low power mode
  LowPower.sleep();
}


void encrypt(char *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        buffer[i] ^= xor_key[i % MAX_PAYLOAD];  // Apply XOR with key
    }
}


bool send(uint8_t button) {

  // Prepare memory
  char message[MAX_PAYLOAD];
  memset(message, 0, sizeof(message));  // Zeroize the buffer to avoid dirty memory

  // Format message
  snprintf(message, sizeof(message), "button_%u", button);
  Serial.print("Sending: ");
  Serial.println(message);
  Serial.flush();

  // Encrypt before sending
  encrypt(message, sizeof(message));
  Serial.print("Encrypted (HEX): ");
  for (size_t i = 0; i < sizeof(message); i++) {
    Serial.print("0x");
    if (message[i] < 16) Serial.print("0"); // Ensure two-digit format
    Serial.print(message[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  Serial.flush();

  // Wake up
  radio.powerUp();
  delay(5);  // Ensure NRF24 is fully awake

  // Send the message
  bool result = radio.write(&message, sizeof(message));

  // Put NRF24 back to low power
  radio.powerDown();

  return result;
}


// Interrupt Service Routines (ISR) for button presses
void isr_button_1() {
  button_pressed = 1;
}

void isr_button_2() {
  button_pressed = 2;
}

void isr_button_3() {
  button_pressed = 3;
}
