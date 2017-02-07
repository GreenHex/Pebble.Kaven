//
// Copyright (C) 2016, Vinodh Kumar M. <GreenHex@gmail.com>
//

#pragma once
#include <pebble.h>
#include "global.h"

static GPathInfo PATH_TICK = {
  2, (GPoint []) {
    { 0, - ( CLOCK_DIAL_SIZE_W > CLOCK_DIAL_SIZE_H ? CLOCK_DIAL_SIZE_W : CLOCK_DIAL_SIZE_H ) },
    { 0, ( CLOCK_DIAL_SIZE_W >  CLOCK_DIAL_SIZE_H ? CLOCK_DIAL_SIZE_W : CLOCK_DIAL_SIZE_H ) }
  }
};

void clock_init( Window* window );
void clock_deinit( void );
void implementation_teardown( Animation *animation );
