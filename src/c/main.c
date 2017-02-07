//
// Copyright (C) 2016, Vinodh Kumar M. <GreenHex@gmail.com>
//

#include <pebble.h>
#include "global.h"
#include "clock.h"

static Window *window;

static void window_load( Window *window ) { // order is important as layers are drawn over each other.
  clock_init( window );
}

static void window_unload( Window *window ) {
  clock_deinit();
}

static void init( void ) {
  window = window_create();
  window_set_window_handlers( window, ( WindowHandlers ) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push( window, false );
}

static void destroy( void ) {
  window_destroy( window );
}

int main( void ) {
  init();
  #ifdef DEBUG
  APP_LOG( APP_LOG_LEVEL_DEBUG, "After init(): Heap: %d bytes used, %d bytes free", (int) heap_bytes_used(), (int) heap_bytes_free() );  
  #endif
  app_event_loop();
  destroy();
  #ifdef DEBUG
  APP_LOG( APP_LOG_LEVEL_DEBUG, "After window_destroy() Heap: %d bytes used, %d bytes free", (int) heap_bytes_used(), (int) heap_bytes_free() );
  #endif
  return 0;
}
