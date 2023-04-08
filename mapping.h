#pragma once

#include "gc_adapter.h"
#include "config.h"
#include "util.h"

void process_inputs_analog(gc_inputs *i);
void process_inputs_digital(gc_inputs *i);
const char *mapping_get_label(enum MappingButtonAxis ba);
int get_buttonaxis_state(enum MappingButtonAxis ba, gc_inputs *i, gc_inputs *id, int is_analog);
int get_mapping_state(gc_inputs *i, gc_inputs *id, struct Mapping m, int is_analog);
