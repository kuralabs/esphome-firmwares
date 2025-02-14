from esphome import (
    codegen as cg,
    config_validation as cv,
    automation,
    const,
    pins,
)


CODEOWNERS = ["@carlos-jenkins"]


nrf24rx_ns = cg.esphome_ns.namespace("nrf24rx")
NRF24Receiver = nrf24rx_ns.class_(
    "NRF24Receiver",
    cg.Component,
)
NRF24OnReceiveTrigger = nrf24rx_ns.class_(
    "NRF24OnReceiveTrigger",
    automation.Trigger.template(cg.std_string),
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(NRF24Receiver),
        cv.Required('ce_pin'): pins.gpio_output_pin_schema,
        cv.Required('csn_pin'): pins.gpio_output_pin_schema,
        cv.Required('irq_pin'): pins.gpio_input_pin_schema,
        cv.Optional('encryption_key'): cv.string,
        cv.Required('on_receive'): automation.validate_automation(
            {
                cv.GenerateID(): cv.declare_id(NRF24OnReceiveTrigger),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):

    cg.add_library("nRF24/RF24", "1.4.11")

    component = cg.new_Pvariable(
        config[const.CONF_ID],
    )
    await cg.register_component(component, config)

    ce_pin = await cg.gpio_pin_expression(config['ce_pin'])
    cg.add(component.set_ce_pin(ce_pin))

    csn_pin = await cg.gpio_pin_expression(config['csn_pin'])
    cg.add(component.set_csn_pin(csn_pin))

    irq_pin = await cg.gpio_pin_expression(config['irq_pin'])
    cg.add(component.set_irq_pin(irq_pin))

    if 'encryption_key' in config:
        cg.add(component.set_encryption_key(config['encryption_key']))

    for conf in config.get('on_receive', []):
        trigger = cg.new_Pvariable(conf[const.CONF_ID], component)
        await automation.build_automation(
            trigger,
            [(cg.std_string, 'message')],
            conf,
        )
