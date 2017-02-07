//
// Copyright (C) 2016, Vinodh Kumar M. <GreenHex@gmail.com>
//

#include <pebble.h>
#include "global.h"
#include "clock.h"
#include "utils.h"
#include "battery.h"
#include "status.h"
#include "animation.h"

#define NUM_PBL_64_COLOURS 64

#ifdef RANDOMIZE_CLOCKFACE_COLOURS
const uint32_t PBL_64_COLOURS[ NUM_PBL_64_COLOURS ] = {
  0x000000, 0xFFFF00, 0xAAAAAA, 0x555555, 0xFFFFAA, 0xFFFF55, 0xFFAA55, 0xFF5500,
  0xFF0000, 0xFF0055, 0xFF5555, 0xFFAAAA, 0xFFFF00, 0xFFAA00, 0xAA5500, 0xAA5555,
  0xAA0000, 0xFF00AA, 0xFF55AA, 0xFFAAFF, 0x550000, 0xAA0055, 0xFF00FF, 0xFF55FF,
  0x550055, 0xAA00AA, 0xAA55AA, 0x000055, 0x5500AA, 0xAA00FF, 0xAA55FF, 0x0000AA,
  0x5500FF, 0x5555AA, 0x0055AA, 0x55AAAA, 0x55AA55, 0x00AA00, 0x00FF00, 0x55FF00,
  0xAAFF55, 0xAAFF00, 0x55AA00, 0x005500, 0x005555, 0xAAAA55, 0x555500, 0xAAAA00,
  0xAAFFAA, 0x55FF55, 0x00FF55, 0x00AA55, 0x00AAAA, 0x00AAFF, 0x0000FF, 0x5555FF,
  0xAAAAFF, 0x55FFAA, 0x00FFAA, 0x00FFFF, 0x55AAFF, 0x0055FF, 0x55FFFF, 0xAAFFFF
};
#endif

tm tm_time;
GColor foreground_colour;
GColor background_colour;

static Layer *window_layer = 0;
static Layer *dial_layer = 0;
static Layer *snooze_layer = 0;
static Layer *hours_layer = 0;
static Layer *minutes_layer = 0;
static Layer *seconds_layer = 0;
#ifdef ALWAYS_SHOW_SECONDS
static bool show_seconds = true;
#else
static bool show_seconds = false;
#endif
static AppTimer *secs_display_apptimer = 0;

static uint32_t const two_segments[] = { 200, 200, 200 };
VibePattern double_vibe_pattern = {
  .durations = two_segments,
  .num_segments = ARRAY_LENGTH( two_segments ),
};

#ifdef RANDOMIZE_CLOCKFACE_COLOURS
static void randomize_colours( void ) {
  #if defined( PBL_COLOR )
  background_colour = GColorFromHEX( PBL_64_COLOURS[ rand() % NUM_PBL_64_COLOURS ] );
  foreground_colour = gcolor_legible_over( background_colour );
  #endif
}
#endif

static void handle_clock_tick( struct tm *tick_time, TimeUnits units_changed ) {
  tm_time = *tick_time; // copy to global
  
  #ifdef DEBUG
  APP_LOG( APP_LOG_LEVEL_INFO, "clock.c: handle_clock_tick(): %02d:%02d:%02d", tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec );
  #endif
  
  layer_mark_dirty( dial_layer );
  if ( ( units_changed & HOUR_UNIT ) && ( !quiet_time_is_active() ) ) vibes_enqueue_custom_pattern( double_vibe_pattern );
  
  #ifdef RANDOMIZE_CLOCKFACE_COLOURS
  #if defined( PBL_COLOR )
  if ( units_changed & MINUTE_UNIT ) randomize_colours();
  #endif
  #endif
}

static void window_layer_update_proc( Layer *layer, GContext *ctx ) {
  GRect bounds = layer_get_bounds( layer );
  graphics_context_set_antialiased( ctx, true );
  graphics_context_set_fill_color( ctx, foreground_colour );
  graphics_fill_rect( ctx, bounds, 0, GCornerNone );
}

static void snooze_layer_update_proc( Layer *layer, GContext *ctx ) {
  if ( quiet_time_is_active() ) {
    GBitmap *snooze_bitmap = 0;
    GRect bounds = layer_get_bounds( layer );
    graphics_context_set_compositing_mode( ctx, GCompOpSet );
    if ( gcolor_equal( foreground_colour, GColorWhite ) ) {
      snooze_bitmap = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_MOUSE_B );
    } else {
      snooze_bitmap = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_MOUSE_W );
    }
    graphics_draw_bitmap_in_rect( ctx, snooze_bitmap, bounds );
    gbitmap_destroy( snooze_bitmap );
    change_layer_colours( ctx, layer, GColorWhite, foreground_colour, GColorBlack, background_colour );
    // change_layer_colours( ctx, layer, GColorWhite, background_colour, GColorBlack, GColorJaegerGreen );
  }
}

static void dial_layer_update_proc( Layer *layer, GContext *ctx ) {
  GRect bounds = layer_get_bounds( layer );
  graphics_context_set_antialiased( ctx, true );
  graphics_context_set_fill_color( ctx, background_colour );
  graphics_fill_rect( ctx, bounds, CLOCK_CORNER_RADIUS, GCornersAll );
  /*
  draw_seconds_ticks( & (DRAW_TICKS_PARAMS) { 
    .layer = layer, 
    .ctx = ctx, 
    .p_gpath_info = &PATH_TICK, 
    .increment = 1, 
    .tick_thk = 1, 
    .tick_length = 5,
    .tick_colour = foreground_colour, 
    .bg_colour = background_colour
  } ); */
  draw_seconds_ticks( & (DRAW_TICKS_PARAMS) { 
    .layer = layer, 
    .ctx = ctx, 
    .p_gpath_info = &PATH_TICK, 
    .increment = 5, 
    .tick_thk = 1, 
    .tick_length = 12, 
    .tick_colour = foreground_colour, 
    .bg_colour = background_colour
  } );
  draw_seconds_ticks( & (DRAW_TICKS_PARAMS) {
    .layer = layer,
    .ctx = ctx,
    .p_gpath_info = &PATH_TICK,
    .increment = 15,
    .tick_thk = 3,
    .tick_length = 15,
    .tick_colour = foreground_colour,
    .bg_colour = background_colour
  } );
  graphics_context_set_stroke_color( ctx, background_colour );
  graphics_context_set_stroke_width( ctx, CLOCK_TICK_EDGE_OFFSET );
  graphics_draw_round_rect( ctx, grect_inset( bounds, GEdgeInsets( CLOCK_TICK_EDGE_OFFSET / 2 ) ), CLOCK_CORNER_RADIUS );
}

static void hours_layer_update_proc( Layer *layer, GContext *ctx ) {
  GRect bounds = layer_get_bounds( layer );
  GPoint center_pt = grect_center_point( &bounds );
  
  uint32_t hour_angle = ( TRIG_MAX_ANGLE * ( ( ( tm_time.tm_hour % 12 ) * 6 ) + ( tm_time.tm_min / 10 ) ) ) / ( 12 * 6 );
  
  GPoint hour_hand = (GPoint) {
    .x = ( sin_lookup( hour_angle ) * HOUR_HAND_LENGTH / TRIG_MAX_RATIO ) + center_pt.x,
    .y = ( -cos_lookup( hour_angle ) * HOUR_HAND_LENGTH / TRIG_MAX_RATIO ) + center_pt.y
  }; 
  
  draw_clock_hand( & (HAND_DRAW_PARAMS) {
    .ctx = ctx,
    .center_pt = center_pt,
    .from_pt = center_pt,
    .to_pt = hour_hand,
    .hand_width = HOUR_HAND_WIDTH,
    .hand_colour = foreground_colour,
    .hand_outline_colour = background_colour,
    .dot_radius = HOUR_CENTER_DOT_RADIUS,
    .dot_colour = foreground_colour,
    .dot_outline_colour = background_colour
  } );
}

static void minutes_layer_update_proc( Layer *layer, GContext *ctx ) {
  GRect bounds = layer_get_bounds( layer );
  GPoint center_pt = grect_center_point( &bounds );
  
  uint32_t minute_angle = TRIG_MAX_ANGLE * tm_time.tm_min / 60;
  GPoint minute_hand = (GPoint) {
    .x = ( sin_lookup( minute_angle ) * MIN_HAND_LENGTH / TRIG_MAX_RATIO ) + center_pt.x,
    .y = ( -cos_lookup( minute_angle ) * MIN_HAND_LENGTH / TRIG_MAX_RATIO ) + center_pt.y
  };
  
  draw_clock_hand( & (HAND_DRAW_PARAMS) {
    .ctx = ctx,
    .center_pt = center_pt,
    .from_pt = center_pt,
    .to_pt = minute_hand,
    .hand_width = MIN_HAND_WIDTH,
    .hand_colour = foreground_colour,
    .hand_outline_colour = background_colour,
    .dot_radius = MIN_CENTER_DOT_RADIUS,
    .dot_colour = foreground_colour,
    .dot_outline_colour = background_colour
  } );
}

static void seconds_layer_update_proc( Layer *layer, GContext *ctx ) {
  if ( !show_seconds ) return;
  
  GRect bounds = layer_get_bounds( layer );
  GPoint center_pt = grect_center_point( &bounds );
  
  uint32_t sec_angle = TRIG_MAX_ANGLE * tm_time.tm_sec / 60;
  uint32_t sec_tail_angle = ( TRIG_MAX_ANGLE * tm_time.tm_sec / 60 ) + ( TRIG_MAX_ANGLE / 2 );
  GPoint sec_hand = (GPoint) {
    .x = ( sin_lookup( sec_angle ) * SEC_HAND_LENGTH / TRIG_MAX_RATIO ) + center_pt.x,
    .y = ( -cos_lookup( sec_angle ) * SEC_HAND_LENGTH / TRIG_MAX_RATIO ) + center_pt.y
  };    
  GPoint sec_hand_tail = (GPoint) {
    .x = ( sin_lookup( sec_tail_angle ) * SEC_HAND_TAIL_LENGTH / TRIG_MAX_RATIO ) + center_pt.x,
    .y = ( -cos_lookup( sec_tail_angle ) * SEC_HAND_TAIL_LENGTH / TRIG_MAX_RATIO ) + center_pt.y
  }; 
  
  draw_clock_hand( & (HAND_DRAW_PARAMS) {
    .ctx = ctx,
    .center_pt = center_pt,
    .from_pt = sec_hand,
    .to_pt = sec_hand_tail,
    .hand_width = SEC_HAND_WIDTH,
    .hand_colour = foreground_colour,
    .hand_outline_colour = background_colour,
    .dot_radius = SEC_CENTER_DOT_RADIUS,
    .dot_colour = foreground_colour,
    .dot_outline_colour = background_colour
  } );
  
  /*
  #if defined( PBL_COLOR )
  GPoint sec_hand_tip = (GPoint) {
    .x = ( sin_lookup( sec_angle ) * ( SEC_HAND_LENGTH - SEC_HAND_TIP_LENGTH ) / TRIG_MAX_RATIO ) + center_pt.x,
    .y = ( -cos_lookup( sec_angle ) * ( SEC_HAND_LENGTH - SEC_HAND_TIP_LENGTH ) / TRIG_MAX_RATIO ) + center_pt.y
  };
  graphics_context_set_stroke_width( ctx, SEC_HAND_WIDTH );
  graphics_context_set_stroke_color( ctx, COLOUR_SEC_HAND_TIP );
  graphics_draw_line( ctx, sec_hand, sec_hand_tip );
  #endif
  */
}

#ifndef ALWAYS_SHOW_SECONDS
static void stop_seconds_display( void* data ) { // after timer elapses
  secs_display_apptimer = 0;
  show_seconds = false;
  layer_set_hidden( battery_layer, true );
  tick_timer_service_subscribe( MINUTE_UNIT, handle_clock_tick );
}

void start_seconds_display( AccelAxisType axis, int32_t direction ) {
  tick_timer_service_subscribe( SECOND_UNIT, handle_clock_tick );
  show_seconds = true;
  layer_set_hidden( battery_layer, false );
  if ( secs_display_apptimer ) {
    app_timer_reschedule( secs_display_apptimer, SHOW_SECONDS_TIMER_TIMEOUT_MS );
  } else {
    secs_display_apptimer = app_timer_register( SHOW_SECONDS_TIMER_TIMEOUT_MS, stop_seconds_display, 0 );
  }
}
#endif

#ifdef ALLOW_TIMELINE_QV
static void unobstructed_change_proc( AnimationProgress progress, void *context ) {
  GRect uo_bounds = layer_get_unobstructed_bounds( (Layer *) context );
  GRect bounds = layer_get_bounds( (Layer *) context );
  GRect clock_dial_rect = CLOCK_DIAL_RECT;
  clock_dial_rect.origin = GPointZero;
  clock_dial_rect.size.h = clock_dial_rect.size.h * uo_bounds.size.h / bounds.size.h;
  
  layer_set_bounds( dial_layer, clock_dial_rect );
  layer_set_bounds( hours_layer, clock_dial_rect );
  layer_set_bounds( minutes_layer, clock_dial_rect );
  layer_set_bounds( seconds_layer, clock_dial_rect );
  /*
  GRect date_window_frame = DATE_WINDOW_FRAME;
  date_window_frame.origin.y = clock_dial_rect.origin.y + clock_dial_rect.size.h/2 - date_window_frame.size.h/2;
  layer_set_frame( date_layer, date_window_frame );
  GRect battery_gauge_frame = BATTERY_GAUGE_FRAME;
  battery_gauge_frame.origin.y = clock_dial_rect.origin.y + clock_dial_rect.size.h/2 - battery_gauge_frame.size.h/2;
  layer_set_frame( battery_layer, battery_gauge_frame );
  GRect snooze_layer_frame = SNOOZE_LAYER_FRAME;
  snooze_layer_frame.origin.y = snooze_layer_frame.origin.y * uo_bounds.size.h / bounds.size.h;
  layer_set_frame( bitmap_layer_get_layer( snooze_layer ), snooze_layer_frame );
  layer_mark_dirty( bitmap_layer_get_layer( snooze_layer ) );
  */
  layer_mark_dirty( dial_layer );
}
#endif

void clock_init( Window* window ){
  time_t now = time( NULL ) - ( 60 * 45 );
  tm_time = *localtime( &now );
  
  window_layer = window_get_root_layer( window );
  srand( time( NULL ) );
  
  foreground_colour = BACKGROUND_COLOUR;
  background_colour = FOREGROUND_COLOUR;
  #ifdef RANDOMIZE_CLOCKFACE_COLOURS
  randomize_colours();
  #endif
  
  layer_set_update_proc( window_layer, window_layer_update_proc );

  dial_layer = layer_create( CLOCK_DIAL_RECT );
  layer_set_update_proc( dial_layer, dial_layer_update_proc );
  layer_add_child( window_layer, dial_layer );
  GRect dial_layer_bounds = layer_get_bounds( dial_layer ); 
  
  snooze_layer = layer_create( SNOOZE_LAYER_FRAME );
  layer_set_update_proc( snooze_layer , snooze_layer_update_proc );
  layer_add_child( dial_layer, snooze_layer );
  
  battery_init( dial_layer );
  status_init( window_layer );
  
  hours_layer = layer_create( dial_layer_bounds );
  layer_set_update_proc( hours_layer, hours_layer_update_proc );
  layer_add_child( dial_layer, hours_layer );
  
  minutes_layer = layer_create( dial_layer_bounds );
  layer_set_update_proc( minutes_layer, minutes_layer_update_proc );
  layer_add_child( dial_layer, minutes_layer );
  
  seconds_layer = layer_create( dial_layer_bounds );
  layer_set_update_proc( seconds_layer, seconds_layer_update_proc );
  layer_add_child( dial_layer, seconds_layer );
  
  start_animation( 50, 1200, AnimationCurveEaseInOut, (void *) dial_layer );
}

void implementation_teardown( Animation *animation ) {
  #ifdef ALLOW_TIMELINE_QV
  unobstructed_area_service_subscribe( (UnobstructedAreaHandlers) { .change = unobstructed_change_proc }, window_layer );
  #endif
  
  #ifdef SHOW_SECONDS
  tick_timer_service_subscribe( SECOND_UNIT, handle_clock_tick );
  #else
  tick_timer_service_subscribe( MINUTE_UNIT, handle_clock_tick );
  #endif
  accel_tap_service_subscribe( start_seconds_display );
  
  time_t now = time( NULL );
  handle_clock_tick( localtime( &now ), 0 );

  animation_destroy( animation );
}

void clock_deinit( void ) {
  if ( seconds_layer ) layer_destroy( seconds_layer );
  if ( minutes_layer ) layer_destroy( minutes_layer );
  if ( hours_layer ) layer_destroy( hours_layer );
  status_deinit();
  battery_deinit();
  if ( snooze_layer ) layer_destroy( snooze_layer );
  if ( dial_layer ) layer_destroy( dial_layer );
}
