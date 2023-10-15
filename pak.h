#pragma once

#include <stdint.h>

int pak_write(int controller, uint16_t address, uint8_t data[33]);
int pak_read(int controller, uint16_t address, uint8_t data[33]);
