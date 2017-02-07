//
// Copyright (C) 2016, Vinodh Kumar M. <GreenHex@gmail.com>
//

#pragma once
#include <pebble.h>
#include "global.h"

extern Layer *battery_layer;

void battery_init( Layer *parent_layer );
void battery_deinit( void );
