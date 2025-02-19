#include "nrf24rx.h"
#include "esphome/core/log.h"


namespace esphome {
namespace nrf24rx {


static const char *const TAG = "nrf24rx";


void IRAM_ATTR NRF24Receiver::handle_irq_(NRF24Receiver *receiver) {
    receiver->message_available_ = true;
}

void NRF24Receiver::setup() {

    SPI.begin();

    this->radio_ = RF24(
        this->ce_pin_->get_pin(),
        this->csn_pin_->get_pin()
    );
    this->radio_.begin();
    this->radio_.setPALevel(RF24_PA_LOW);
    this->radio_.openReadingPipe(PIPE_NUMBER, PIPE_ADDRESS);
    this->radio_.startListening();

    // Attach Interrupt to IRQ Pin
    this->irq_pin_->attach_interrupt(
        NRF24Receiver::handle_irq_,
        this,
        gpio::INTERRUPT_FALLING_EDGE
    );
}

void NRF24Receiver::loop() {
    if (!this->message_available_) {
        return;
    }

    this->message_available_ = false;

    while (this->radio_.available()) {

        ESP_LOGI(TAG, "New message received ...");

        // Get actual message size
        uint8_t length = this->radio_.getPayloadSize();
        if (length == 0 || length > MAX_PAYLOAD) {
            ESP_LOGW(TAG, "Invalid message size: %d", length);
            return;
        }

        // Read ciphertext
        ESP_LOGI(TAG, "Reading message ...");
        char buffer[MAX_PAYLOAD + 1];
        memset(buffer, 0, sizeof(buffer));

        this->radio_.read(&buffer, length);
        ESP_LOGI(TAG, "Successfully read %d bytes message", length);

        // Decrypt message
        if (!this->encryption_key_.empty()) {
            ESP_LOGI(TAG, "Decrypting message ...");
            this->decrypt(buffer, length);
        }

        // Convert to string safely
        buffer[MAX_PAYLOAD] = '\0';
        std::string message(buffer);

        // Call the user-defined callback function if set
        ESP_LOGI(TAG, "Sending message to user callback: %s", buffer);
        this->receive_callback_.call(message);
    }
}

// Convert Hex String to Byte Array
void NRF24Receiver::set_encryption_key(std::string encryption_key) {
    this->encryption_key_.clear();

    // 32 bytes in hex is 64 characters
    if (encryption_key.length() != (MAX_PAYLOAD * 2)) {
        ESP_LOGE(
            TAG,
            "Invalid key length. Expected 64 hex characters, got %d.",
            encryption_key.length()
        );
        return;
    }

    // Convert hex string to bytes
    for (size_t i = 0; i < encryption_key.length(); i += 2) {
        std::string byteString = encryption_key.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), nullptr, 16);
        encryption_key_.push_back(byte);
    }

    ESP_LOGI(TAG, "Encryption key successfully loaded.");
}

void NRF24Receiver::add_on_receive_callback(
    std::function<void(std::string)> &&callback
) {
    this->receive_callback_.add(std::move(callback));
}

// XOR Decryption
void NRF24Receiver::decrypt(char *message, size_t length) {
    for (size_t i = 0; i < length; i++) {
        // XOR each byte with the key
        message[i] ^= this->encryption_key_[i % 32];
    }
}

}  // namespace nrf24rx
}  // namespace esphome
