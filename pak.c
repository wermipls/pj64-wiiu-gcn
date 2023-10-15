#include "pak.h"

#include <stddef.h>
#include "config.h"
#include "gc_adapter.h"
#include "mapping.h"
#include "log.h"

static uint8_t rumble_state = 0x00;

static uint8_t crc_data(uint8_t *data)
{
    uint8_t p = 0x85;
    uint8_t crc = 0;

    for (size_t i = 32; i; --i) {
        crc ^= *data;
        for (size_t j = 8; j; --j) {
            uint8_t msb = crc & 0x80;
            crc <<= 1;
            if (msb) {
                crc ^= p;
            }
        }
        ++data;
    }

    return crc;
}

static uint16_t crc_address(uint16_t addr)
{
    const uint8_t p = 0x15;
    uint16_t pp = p << 10;
    addr &= 0xFFE0;
    uint16_t crc = addr;

    for (size_t i = 15; i >= 5; i--) {
        if (crc & (1<<i)) {
            crc ^= pp;
        }
        pp >>= 1;
    }

    return addr | (crc & 0x1F);
}

int pak_write(int controller, uint16_t address, uint8_t data[33])
{
    dlog(LOG_INFO, "pak write, %d, %04x, %02x", controller, address, data[0]);
    if (cfg.controller[controller].accessory != ACCESSORY_RUMBLE) {
        return 0;
    }

    uint16_t addr_crc = crc_address(address);
    if (addr_crc != address) {
        return -1;
    }

    address &= 0xFFE0;

    if (address == 0xC000) {
        if (*data == 0x01) {
            gc_set_rumble(get_port_mapping(controller), 1);
        } else if (*data == 0) {
            gc_set_rumble(get_port_mapping(controller), 0);
        }
    } else if (address == 0x8000) {
        if (*data == 0xfe) {
            rumble_state = 0x00;
        } else if (*data == 0x80) {
            rumble_state = 0x80;
        } 
    }

    data[32] = crc_data(data);

    return 0;
}


int pak_read(int controller, uint16_t address, uint8_t data[33])
{
    dlog(LOG_INFO, "pak read, %d, %04x, %02x", controller, address, data[0]);
    if (cfg.controller[controller].accessory != ACCESSORY_RUMBLE) {
        return 0;
    }

    uint16_t addr_crc = crc_address(address);
    if (addr_crc != address) {
        return -1;
    }
    address &= 0xFFE0;

    uint8_t response = (address == 0x8000) ? rumble_state : 0x00;

    for (size_t i = 0; i < 32; i++) {
        data[i] = response;
    }

    data[32] = crc_data(data);

    return 0;
}