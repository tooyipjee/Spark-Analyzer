
/**
 * PD_UFP.h
 *
 *      Author: Ryan Ma
 *      Edited: Kai Liebich
 *
 * Minimalist USB PD Ardunio Library for PD Micro board
 * Only support UFP(device) sink only functionality
 * Requires FUSB302_UFP.h, PD_UFP_Protocol.h and Standard Arduino Library
 *
 * Support PD3.0 PPS
 * 
 */
 
#include <stdint.h>
#include <string.h>

#include "PD_UFP.h"

#define t_PD_POLLING            100
#define t_TypeCSinkWaitCap      350
#define t_RequestToPSReady      580     // combine t_SenderResponse and t_PSTransition
#define t_PPSRequest            5000    // must less than 10000 (10s)

#define PIN_FUSB302_INT         12

enum {
    STATUS_LOG_MSG_TX,
    STATUS_LOG_MSG_RX,
    STATUS_LOG_DEV,
    STATUS_LOG_CC,
    STATUS_LOG_SRC_CAP,
    STATUS_LOG_POWER_READY,
    STATUS_LOG_POWER_PPS_STARTUP,
    STATUS_LOG_POWER_REJECT,
    STATUS_LOG_LOAD_SW_ON,
    STATUS_LOG_LOAD_SW_OFF,
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// PD_UFP_c
///////////////////////////////////////////////////////////////////////////////////////////////////
PD_UFP_c::PD_UFP_c():
    ready_voltage(0),
    ready_current(0),
    PPS_voltage_next(0),
    PPS_current_next(0),
    status_initialized(0),
    status_src_cap_received(0),
    status_power(STATUS_POWER_NA),
    time_polling(0),
    time_wait_src_cap(0),
    time_wait_ps_rdy(0),
    time_PPS_request(0),
    get_src_cap_retry_count(0),
    wait_src_cap(0),
    wait_ps_rdy(0),
    send_request(0)
{
    memset(&FUSB302, 0, sizeof(FUSB302_dev_t));
    memset(&protocol, 0, sizeof(PD_protocol_t));
}

void PD_UFP_c::init(uint8_t int_pin, enum PD_power_option_t power_option)
{
    init_PPS(int_pin, 0, 0, power_option);
}

void PD_UFP_c::init_PPS(uint8_t int_pin, uint16_t PPS_voltage, uint8_t PPS_current, enum PD_power_option_t power_option)
{
    this->int_pin = int_pin;
    // Initialize FUSB302
    pinMode(int_pin, INPUT_PULLUP); // Set FUSB302 int pin input ant pull up
    FUSB302.i2c_address = 0x22;
    FUSB302.i2c_read = FUSB302_i2c_read;
    FUSB302.i2c_write = FUSB302_i2c_write;
    FUSB302.delay_ms = FUSB302_delay_ms;
    if (FUSB302_init(&FUSB302) == FUSB302_SUCCESS && FUSB302_get_ID(&FUSB302, 0, 0) == FUSB302_SUCCESS) {
        status_initialized = 1;
    }

    // Two stage startup for PPS Voltge < 5V
    if (PPS_voltage && PPS_voltage < PPS_V(5.0)) {
        PPS_voltage_next = PPS_voltage;
        PPS_current_next = PPS_current;
        PPS_voltage = PPS_V(5.0);
    }

    // Initialize PD protocol engine
    PD_protocol_init(&protocol);
    PD_protocol_set_power_option(&protocol, power_option);
    PD_protocol_set_PPS(&protocol, PPS_voltage, PPS_current, false);

    status_log_event(STATUS_LOG_DEV);
}

void PD_UFP_c::run(void)
{
    if (timer() || digitalRead(int_pin) == 0) {
        FUSB302_event_t FUSB302_events = 0;
        for (uint8_t i = 0; i < 3 && FUSB302_alert(&FUSB302, &FUSB302_events) != FUSB302_SUCCESS; i++) {}
        if (FUSB302_events) {
            handle_FUSB302_event(FUSB302_events);
        }
    }
}

bool PD_UFP_c::set_PPS(uint16_t PPS_voltage, uint8_t PPS_current)
{
    if (status_power == STATUS_POWER_PPS && PD_protocol_set_PPS(&protocol, PPS_voltage, PPS_current, true)) {
        send_request = 1;
        return true;
    }
    return false;
}

void PD_UFP_c::set_power_option(enum PD_power_option_t power_option)
{
    if (PD_protocol_set_power_option(&protocol, power_option)) {
        send_request = 1;
    }
}

void PD_UFP_c::clock_prescale_set(uint8_t prescaler)
{
    if (prescaler) {
        clock_prescaler = prescaler;
    }
}

FUSB302_ret_t PD_UFP_c::FUSB302_i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t count)
{
    Wire.beginTransmission(dev_addr);
    Wire.write(reg_addr);
    Wire.endTransmission();
    Wire.requestFrom(dev_addr, count);
    while (Wire.available() && count > 0) {
        *data++ = Wire.read();
        count--;
    }
    return count == 0 ? FUSB302_SUCCESS : FUSB302_ERR_READ_DEVICE;
}

FUSB302_ret_t PD_UFP_c::FUSB302_i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t count)
{
    Wire.beginTransmission(dev_addr);
    Wire.write(reg_addr);
    while (count > 0) {
        Wire.write(*data++);
        count--;
    }
    Wire.endTransmission();
    return FUSB302_SUCCESS;
}

FUSB302_ret_t PD_UFP_c::FUSB302_delay_ms(uint32_t t)
{
    delay(t / clock_prescaler);
    return FUSB302_SUCCESS;
}

void PD_UFP_c::handle_protocol_event(PD_protocol_event_t events)
{    
    if (events & PD_PROTOCOL_EVENT_SRC_CAP) {
        wait_src_cap = 0;
        get_src_cap_retry_count = 0;
        wait_ps_rdy = 1;
        time_wait_ps_rdy = clock_ms();
        status_log_event(STATUS_LOG_SRC_CAP);
    }
    if (events & PD_PROTOCOL_EVENT_REJECT) {
        if (wait_ps_rdy) {
            wait_ps_rdy = 0;
            status_log_event(STATUS_LOG_POWER_REJECT);
        }
    }    
    if (events & PD_PROTOCOL_EVENT_PS_RDY) {
        PD_power_info_t p;
        uint8_t i, selected_power = PD_protocol_get_selected_power(&protocol);
        PD_protocol_get_power_info(&protocol, selected_power, &p);
        wait_ps_rdy = 0;
        if (p.type == PD_PDO_TYPE_AUGMENTED_PDO) {
            // PPS mode
            FUSB302_set_vbus_sense(&FUSB302, 0);
            if (PPS_voltage_next) {
                // Two stage startup for PPS voltage < 5V
                PD_protocol_set_PPS(&protocol, PPS_voltage_next, PPS_current_next, false);
                PPS_voltage_next = 0;
                send_request = 1;
                status_log_event(STATUS_LOG_POWER_PPS_STARTUP);
            } else {
                time_PPS_request = clock_ms();
                status_power_ready(STATUS_POWER_PPS, 
                    PD_protocol_get_PPS_voltage(&protocol), PD_protocol_get_PPS_current(&protocol));
                status_log_event(STATUS_LOG_POWER_READY);
            }
        } else {
            FUSB302_set_vbus_sense(&FUSB302, 1);
            status_power_ready(STATUS_POWER_TYP, p.max_v, p.max_i);
            status_log_event(STATUS_LOG_POWER_READY);
        }
    }
}

void PD_UFP_c::handle_FUSB302_event(FUSB302_event_t events)
{
    if (events & FUSB302_EVENT_DETACHED) {
        PD_protocol_reset(&protocol);
        return;
    }
    if (events & FUSB302_EVENT_ATTACHED) {
        uint8_t cc1 = 0, cc2 = 0, cc = 0;
        FUSB302_get_cc(&FUSB302, &cc1, &cc2);
        PD_protocol_reset(&protocol);
        if (cc1 && cc2 == 0) {
            cc = cc1;
        } else if (cc2 && cc1 == 0) {
            cc = cc2;
        }
        /* TODO: handle no cc detected error */
        if (cc > 1) {
            wait_src_cap = 1;
        } else {
            set_default_power();
        }
        status_log_event(STATUS_LOG_CC);
    }
    if (events & FUSB302_EVENT_RX_SOP) {
        PD_protocol_event_t protocol_event = 0;
        uint16_t header;
        uint32_t obj[7];
        FUSB302_get_message(&FUSB302, &header, obj);
        PD_protocol_handle_msg(&protocol, header, obj, &protocol_event);
        status_log_event(STATUS_LOG_MSG_RX, obj);
        if (protocol_event) {
            handle_protocol_event(protocol_event);
        }
    }
    if (events & FUSB302_EVENT_GOOD_CRC_SENT) {
        uint16_t header;
        uint32_t obj[7];
        delay_ms(2);  /* Delay respond in case there are retry messages */
        if (PD_protocol_respond(&protocol, &header, obj)) {
            status_log_event(STATUS_LOG_MSG_TX, obj);
            FUSB302_tx_sop(&FUSB302, header, obj);
        }
    }
}

bool PD_UFP_c::timer(void)
{
    uint16_t t = clock_ms();
    if (wait_src_cap && t - time_wait_src_cap > t_TypeCSinkWaitCap) {
        time_wait_src_cap = t;
        if (get_src_cap_retry_count < 3) {
            uint16_t header;
            get_src_cap_retry_count += 1;
            /* Try to request soruce capabilities message (will not cause power cycle VBUS) */
            PD_protocol_create_get_src_cap(&protocol, &header);
            status_log_event(STATUS_LOG_MSG_TX);
            FUSB302_tx_sop(&FUSB302, header, 0);
        } else {
            get_src_cap_retry_count = 0;
            /* Hard reset will cause the source power cycle VBUS. */
            FUSB302_tx_hard_reset(&FUSB302);
            PD_protocol_reset(&protocol);
        }
    }
    if (wait_ps_rdy) {
        if (t - time_wait_ps_rdy > t_RequestToPSReady) {
            wait_ps_rdy = 0;
            set_default_power();
        }
    } else if (send_request || (status_power == STATUS_POWER_PPS && t - time_PPS_request > t_PPSRequest)) {
        wait_ps_rdy = 1;
        send_request = 0;
        time_PPS_request = t;
        uint16_t header;
        uint32_t obj[7];
        /* Send request if option updated or regularly in PPS mode to keep power alive */
        PD_protocol_create_request(&protocol, &header, obj);
        status_log_event(STATUS_LOG_MSG_TX, obj);
        time_wait_ps_rdy = clock_ms();
        FUSB302_tx_sop(&FUSB302, header, obj);
    }
    if (t - time_polling > t_PD_POLLING) {
        time_polling = t;
        return true;
    }
    return false;
}

void PD_UFP_c::set_default_power(void)
{
    status_power_ready(STATUS_POWER_TYP, PD_V(5), PD_A(1));
    status_log_event(STATUS_LOG_POWER_READY);
}

void PD_UFP_c::status_power_ready(status_power_t status, uint16_t voltage, uint16_t current)
{
    ready_voltage = voltage;
    ready_current = current;
    status_power = status;
}

uint8_t PD_UFP_c::clock_prescaler = 1;

void PD_UFP_c::delay_ms(uint16_t ms)
{
    delay(ms / clock_prescaler);
}

uint16_t PD_UFP_c::clock_ms(void)
{
    return (uint16_t)millis() * clock_prescaler;
}
