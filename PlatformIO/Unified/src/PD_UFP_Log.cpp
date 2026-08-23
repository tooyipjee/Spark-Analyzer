
/**
 * PD_UFP_Log.cpp
 *
 *      Author: Ryan Ma
 *      Edited: Kai Liebich
 *      Edit #2: Jason Too
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
// Optional: PD_UFP_Log_c, extended from PD_UFP_c to provide logging function.
//           Asynchronous, minimal impact on PD timing.
///////////////////////////////////////////////////////////////////////////////////////////////////
#define STATUS_LOG_MASK         (sizeof(status_log) / sizeof(status_log[0]) - 1)
#define STATUS_LOG_OBJ_MASK     (sizeof(status_log_obj) / sizeof(status_log_obj[0]) - 1)

PD_UFP_Log_c::PD_UFP_Log_c(pd_log_level_t log_level):
    status_log_write(0),
    status_log_read(0),
    status_log_counter(0),
    status_log_obj_read(0),
    status_log_obj_write(0),
    status_log_level(log_level)
{

}

uint8_t PD_UFP_Log_c::status_log_obj_add(uint16_t header, uint32_t * obj)
{
    if (obj) {
        uint8_t i, w = status_log_obj_write, r = status_log_obj_read;
        PD_msg_info_t info;
        PD_protocol_get_msg_info(header, &info);
        for (i = 0; i < info.num_of_obj && (uint8_t)(w - r) < STATUS_LOG_OBJ_MASK; i++) {
            status_log_obj[w++ & STATUS_LOG_OBJ_MASK] = obj[i];
        }
        status_log_obj_write = w;
        return i;
    }
    return 0;
}

void PD_UFP_Log_c::status_log_event(uint8_t status, uint32_t * obj)
{
    if (((status_log_write - status_log_read) & STATUS_LOG_MASK) >= STATUS_LOG_MASK) {
        return;
    }
    status_log_t * log = &status_log[status_log_write & STATUS_LOG_MASK];
    switch (status) {
    case STATUS_LOG_MSG_TX:
        log->msg_header = PD_protocol_get_tx_msg_header(&protocol);
        log->obj_count = status_log_obj_add(log->msg_header, obj);
        break;
    case STATUS_LOG_MSG_RX:
        log->msg_header = PD_protocol_get_rx_msg_header(&protocol);
        log->obj_count = status_log_obj_add(log->msg_header, obj);
        break;
    default:
        break;
    }
    log->status = status;
    log->time = clock_ms();
    status_log_write++;
}

// Optimize RAM usage on AVR MCU by allocate format string in program memory
#if defined(__AVR__)
#include <avr/pgmspace.h>
#define SNPRINTF snprintf_P
#else
#define SNPRINTF snprintf
#define PSTR(str) str
#endif

#define LOG(format, ...) do { n = SNPRINTF(buffer, maxlen, PSTR(format), ## __VA_ARGS__); } while (0)

int PD_UFP_Log_c::status_log_readline_msg(char * buffer, int maxlen, status_log_t * log)
{
    char * t = status_log_time;
    int n = 0;
    if (status_log_counter == 0) {
        // output message header
        char type = log->status == STATUS_LOG_MSG_TX ? 'T' : 'R';
        PD_msg_info_t info;
        PD_protocol_get_msg_info(log->msg_header, &info);
        if (status_log_level >= PD_LOG_LEVEL_VERBOSE) {
            const char * ext = info.extended ? "ext, " : "";
            LOG("%s%cX %s id=%d %sraw=0x%04X\n", t, type, info.name, info.id, ext, log->msg_header);
            if (info.num_of_obj) {
                status_log_counter++;
            }
        } else {
            LOG("%s%cX %s\n", t, type, info.name);
        }
    } else {
        // output object data
        int i = status_log_counter - 1;
        uint32_t obj = status_log_obj[status_log_obj_read++ & STATUS_LOG_OBJ_MASK];
        LOG("%s obj%d=0x%08lX\n", t, i, obj);
        if (++status_log_counter > log->obj_count) {
            status_log_counter = 0;
        }
    }
    return n;
}

int PD_UFP_Log_c::status_log_readline_src_cap(char * buffer, int maxlen)
{
    PD_power_info_t p;
    int n = 0;
    uint8_t i = status_log_counter;
    if (PD_protocol_get_power_info(&protocol, i, &p)) {
        const char * str_pps[] = {"", " BAT", " VAR", " PPS"};  /* PD_power_data_obj_type_t */
        char * t = status_log_time;
        uint8_t selected = PD_protocol_get_selected_power(&protocol);
        char min_v[8] = {0}, max_v[8] = {0}, power[8] = {0};
        if (p.min_v) SNPRINTF(min_v, sizeof(min_v)-1, PSTR("%d.%02dV-"), p.min_v / 20, (p.min_v * 5) % 100);
        if (p.max_v) SNPRINTF(max_v, sizeof(max_v)-1, PSTR("%d.%02dV"), p.max_v / 20, (p.max_v * 5) % 100);
        if (p.max_i) {
            SNPRINTF(power, sizeof(power)-1, PSTR("%d.%02dA"), p.max_i / 100, p.max_i % 100);
        } else {
            SNPRINTF(power, sizeof(power)-1, PSTR("%d.%02dW"), p.max_p / 4, p.max_p * 25);
        }
        LOG("%s   [%d] %s%s %s%s%s\n", t, i, min_v, max_v, power, str_pps[p.type], i == selected ? " *" : "");
        status_log_counter++;
    } else {
        status_log_counter = 0;
    }
    return n;
}

int PD_UFP_Log_c::status_log_readline(char * buffer, int maxlen)
{
    if (status_log_write == status_log_read) {
        return 0;
    }
    
    status_log_t * log = &status_log[status_log_read & STATUS_LOG_MASK];
    int n = 0;
    char * t = status_log_time;
    if (t[0] == 0) {    // Convert timestamp number to string
        SNPRINTF(t, sizeof(status_log_time)-1, PSTR("%04u: "), log->time);
        return 0; 
    }

    switch (log->status) {
    case STATUS_LOG_MSG_TX:
    case STATUS_LOG_MSG_RX:
        n = status_log_readline_msg(buffer, maxlen, log);
        break;
    case STATUS_LOG_DEV:
        if (status_initialized) {
            uint8_t version_ID = 0, revision_ID = 0;
            FUSB302_get_ID(&FUSB302, &version_ID, &revision_ID);
            LOG("\n%sFUSB302 ver ID:%c_rev%c\n", t, 'A' + version_ID, 'A' + revision_ID);
        } else {
            LOG("\n%sFUSB302 init error\n", t);
        }
        break;
    case STATUS_LOG_CC: {
        const char *detection_type_str[] = {"USB", "1.5", "3.0"};
        uint8_t cc1 = 0, cc2 = 0;
        FUSB302_get_cc(&FUSB302, &cc1, &cc2);
        if (cc1 == 0 && cc2 == 0) {
            LOG("%sUSB attached vRA\n", t);
        } else if (cc1 && cc2 == 0) {
            LOG("%sUSB attached CC1 vRd-%s\n", t, detection_type_str[cc1 - 1]);
        } else if (cc2 && cc1 == 0) {
            LOG("%sUSB attached CC2 vRd-%s\n", t, detection_type_str[cc2 - 1]);
        } else {
            LOG("%sUSB attached unknown\n", t);
        }
        break; }
    case STATUS_LOG_SRC_CAP:
        n = status_log_readline_src_cap(buffer, maxlen);
        break;
    case STATUS_LOG_POWER_READY: {
        uint16_t v = ready_voltage;
        uint16_t a = ready_current;
        if (status_power == STATUS_POWER_TYP) {
            LOG("%s%d.%02dV %d.%02dA supply ready\n", t, v / 20, (v * 5) % 100, a / 100, a % 100);
        } else if (status_power == STATUS_POWER_PPS) {
            LOG("%sPPS %d.%02dV %d.%02dA supply ready\n", t, v / 50, (v * 2) % 100, a / 20, (a * 5) % 100);
        }
        break; }
    case STATUS_LOG_POWER_PPS_STARTUP:
        LOG("%sPPS 2-stage startup\n", t);
        break;
    case STATUS_LOG_POWER_REJECT:
        LOG("%sRequest Rejected\n", t);
        break;
    case STATUS_LOG_LOAD_SW_ON:
        LOG("%sLoad SW ON\n", t);
        break;
    case STATUS_LOG_LOAD_SW_OFF:
        LOG("%sLoad SW OFF\n", t);
        break;
    }
    if (status_log_counter == 0) {
        t[0] = 0;
        status_log_read++;
        status_log_counter = 0;
    }
    return n;
}

void PD_UFP_Log_c::print_status(HardwareSerial & serial)
{
    // // Wait for enough tx buffer in serial port to avoid blocking
    // if (serial && serial.availableForWrite() >= SERIAL_TX_BUFFER_SIZE - 1) {
    //     char buf[SERIAL_TX_BUFFER_SIZE];
    //     if (status_log_readline(buf, sizeof(buf) - 1)) {
    //         serial.print(buf);
    //     }
    // }
}

