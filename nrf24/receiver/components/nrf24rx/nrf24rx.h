#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"

#include <RF24.h>

// Buffer size is given by hardware, do not change it unless you know what
// you're doing.
#define MAX_PAYLOAD 32


namespace esphome {
namespace nrf24rx {

// FIXME: Make this configurable in YAML
const uint8_t PIPE_NUMBER = 1;
const uint8_t PIPE_ADDRESS[] = { 0xDE, 0xAD, 0xC0, 0xDE, 0x01 };


// FIXME: Make component a subclass of SPIDevice, possibly removing csn_pin
class NRF24Receiver : public Component {

public:
    void setup() override;
    void loop() override;

    void set_ce_pin(InternalGPIOPin *ce_pin) { this->ce_pin_ = ce_pin; }
    void set_csn_pin(InternalGPIOPin *csn_pin) { this->csn_pin_ = csn_pin; }
    void set_irq_pin(InternalGPIOPin *irq_pin) { this->irq_pin_ = irq_pin; }
    void set_encryption_key(std::string encryption_key);
    void add_on_receive_callback(std::function<void(std::string)> &&callback);

protected:
    InternalGPIOPin *ce_pin_;
    InternalGPIOPin *csn_pin_;
    InternalGPIOPin *irq_pin_;
    std::vector<uint8_t> encryption_key_;
    CallbackManager<void(std::string)> receive_callback_;

    RF24 radio_;

    volatile bool message_available_ = false;
    static void handle_irq_(NRF24Receiver *receiver);

    void decrypt(char *message, size_t length);
};


class NRF24OnReceiveTrigger : public Trigger<std::string> {
public:
    explicit NRF24OnReceiveTrigger(NRF24Receiver *receiver) {
        receiver->add_on_receive_callback(
            [this](const std::string &value) {
                this->trigger(value);
            }
        );
    }
};


}  // namespace nrf24rx
}  // namespace esphome
