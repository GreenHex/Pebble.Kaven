//
// Copyright (C) 2016, Vinodh Kumar M. <GreenHex@gmail.com>
//

#pragma once
#include <pebble.h>
#include "global.h"

static GPathInfo PATH_TICK = {
  2, (GPoint []) {
    { 0, - ( CLOCK_DIAL_SIZE_W/2 - 4 ) },
    { 0, ( CLOCK_DIAL_SIZE_W/2 - 4 ) }
  }
};

void clock_init( Window* window );
void clock_deinit( void );
void implementation_teardown( Animation *animation );
