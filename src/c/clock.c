//
// Copyright (C) 2016, Vinodh Kumar M. <GreenHex@gmail.com>
//

#include <pebble.h>
#include "global.h"
#include "clock.h"
#include "utils.h"
#include "status.h"

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

extern bool show_name;

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
  graphics_fill_rect( ctx, bounds, CLOCK_CORNER_RADIUS, GCornerNone );

  graphics_context_set_fill_color( ctx, foreground_colour );
  graphics_fill_radial( ctx, grect_inset( bounds, GEdgeInsets( 4 ) ), GOvalScaleModeFitCircle, 16, 0, DEG_TO_TRIGANGLE ( 360 ) );
  graphics_fill_radial( ctx, grect_inset( bounds, GEdgeInsets( 24 ) ), GOvalScaleModeFitCircle, 16, 0, DEG_TO_TRIGANGLE ( 360 ) );
  
  draw_seconds_ticks( & (DRAW_TICKS_PARAMS) { 
    .layer = layer, 
    .ctx = ctx, 
    .p_gpath_info = &PATH_TICK, 
    .increment = 5, 
    .tick_thk = 1, 
    .tick_length = 30, 
    .tick_colour = GColorWhite, 
    .bg_colour = background_colour
  } );
  //
  return;
  //
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
    .hand_colour = GColorBlack, /* foreground_colour, */
    .hand_outline_colour = GColorWhite, /* background_colour, */
    .dot_radius = HOUR_CENTER_DOT_RADIUS,
    .dot_colour = GColorBlack,
    .dot_outline_colour = GColorWhite, /* background_colour, */
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
    .hand_colour = GColorBlack, /* foreground_colour, */
    .hand_outline_colour = GColorWhite, /* background_colour, */
    .dot_radius = MIN_CENTER_DOT_RADIUS,
    .dot_colour = GColorBlack,
    .dot_outline_colour = GColorWhite, /* background_colour, */
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
    .hand_colour = GColorOrange,
    .hand_outline_colour = GColorOrange,
    .dot_radius = SEC_CENTER_DOT_RADIUS,
    .dot_colour = GColorOrange,
    .dot_outline_colour = GColorOrange
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
  show_name = show_seconds = false;
  tick_timer_service_subscribe( MINUTE_UNIT, handle_clock_tick );
}

void start_seconds_display( AccelAxisType axis, int32_t direction ) {
  tick_timer_service_subscribe( SECOND_UNIT, handle_clock_tick );
  show_name = show_seconds = true;
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

#define NUM_DIGITS 12
static RotBitmapLayer *m_layer[ NUM_DIGITS ];
static RotBitmapLayer *h_layer[ NUM_DIGITS ];
static GBitmap *m_bitmap[ NUM_DIGITS ];
static GBitmap *h_bitmap[ NUM_DIGITS ];

uint32_t M_BITMAPS[ NUM_DIGITS ] = {
  RESOURCE_ID_IMAGE_M_00,
  RESOURCE_ID_IMAGE_M_05,
  RESOURCE_ID_IMAGE_M_10,
  RESOURCE_ID_IMAGE_M_15,
  RESOURCE_ID_IMAGE_M_20,
  RESOURCE_ID_IMAGE_M_25,
  RESOURCE_ID_IMAGE_M_30,
  RESOURCE_ID_IMAGE_M_35,
  RESOURCE_ID_IMAGE_M_40,
  RESOURCE_ID_IMAGE_M_45,
  RESOURCE_ID_IMAGE_M_50,
  RESOURCE_ID_IMAGE_M_55
};

uint32_t H_BITMAPS[ NUM_DIGITS ] = {
  RESOURCE_ID_IMAGE_H_12,
  RESOURCE_ID_IMAGE_H_01,
  RESOURCE_ID_IMAGE_H_02,
  RESOURCE_ID_IMAGE_H_03,
  RESOURCE_ID_IMAGE_H_04,
  RESOURCE_ID_IMAGE_H_05,
  RESOURCE_ID_IMAGE_H_06,
  RESOURCE_ID_IMAGE_H_07,
  RESOURCE_ID_IMAGE_H_08,
  RESOURCE_ID_IMAGE_H_09,
  RESOURCE_ID_IMAGE_H_10,
  RESOURCE_ID_IMAGE_H_11
};

static void r_layer_update_proc( Layer *layer, GContext *ctx ) {
  GRect bounds = layer_get_bounds( layer );
  graphics_context_set_antialiased( ctx, true );
  graphics_context_set_fill_color( ctx, GColorGreen );
  graphics_fill_rect( ctx, bounds, 0, GCornerNone );
}

void clock_init( Window* window ){  
  window_layer = window_get_root_layer( window );
  foreground_colour = BACKGROUND_COLOUR;
  background_colour = FOREGROUND_COLOUR;
  #ifdef RANDOMIZE_CLOCKFACE_COLOURS
  srand( time( NULL ) );
  randomize_colours();
  #endif
  
  layer_set_update_proc( window_layer, window_layer_update_proc );

  dial_layer = layer_create( CLOCK_DIAL_RECT );
  layer_set_update_proc( dial_layer, dial_layer_update_proc );
  layer_add_child( window_layer, dial_layer );
  GRect dial_layer_bounds = layer_get_bounds( dial_layer ); 
  
  for ( int i = 0;  i < NUM_DIGITS ; i++ ) {
    m_bitmap[i] = gbitmap_create_with_resource( M_BITMAPS[i] );
    m_layer[i] = rot_bitmap_layer_create( m_bitmap[i] );
    rot_bitmap_layer_set_angle( m_layer[i], DEG_TO_TRIGANGLE ( i * 30 ) );
    rot_bitmap_set_compositing_mode( m_layer[i], GCompOpSet );
    layer_add_child( dial_layer, (Layer *) m_layer[i] );
    GRect b = layer_get_frame( (Layer *) m_layer[i] );
    GRect a = grect_centered_from_polar( grect_inset( CLOCK_DIAL_RECT, GEdgeInsets( 12 ) ),
                                        GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE ( i * 30 ), GSize( 0, 0 ) );
    // layer_set_update_proc( (Layer *) m_layer[i], r_layer_update_proc );
    b.origin.x = a.origin.x - b.size.w / 2;
    b.origin.y = a.origin.y - b.size.h / 2;
    layer_set_frame( (Layer *) m_layer[i], b );
  }
  for ( int i = 0;  i < NUM_DIGITS ; i++ ) {
    h_bitmap[i] = gbitmap_create_with_resource( H_BITMAPS[i] );
    h_layer[i] = rot_bitmap_layer_create( h_bitmap[i] );
    rot_bitmap_layer_set_angle( h_layer[i], DEG_TO_TRIGANGLE ( i * 30 ) );
    rot_bitmap_set_compositing_mode( h_layer[i], GCompOpSet );
    // rot_bitmap_set_src_ic( h_layer, GPoint( -6, -6 ) );
    layer_add_child( dial_layer, (Layer *) h_layer[i] );
    GRect b = layer_get_frame( (Layer *) h_layer[i] );
    GRect a = grect_centered_from_polar( grect_inset( CLOCK_DIAL_RECT, GEdgeInsets( 32 ) ),
                                        GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE ( i * 30 ), GSize( 0, 0 ) );
    // layer_set_update_proc( (Layer *) h_layer[i], r_layer_update_proc );
    b.origin.x = a.origin.x - b.size.w / 2;
    b.origin.y = a.origin.y - b.size.h / 2;
    layer_set_frame( (Layer *) h_layer[i], b );
  }
  
  snooze_layer = layer_create( SNOOZE_LAYER_FRAME );
  layer_set_update_proc( snooze_layer , snooze_layer_update_proc );
  layer_add_child( dial_layer, snooze_layer );
  
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
}

void clock_deinit( void ) {
  for ( int i = 0; i < NUM_DIGITS; i++ ) {
    gbitmap_destroy( m_bitmap[i] );
    gbitmap_destroy( h_bitmap[i] );
    rot_bitmap_layer_destroy( m_layer[i] );
    rot_bitmap_layer_destroy( h_layer[i] );
  }
  if ( seconds_layer ) layer_destroy( seconds_layer );
  if ( minutes_layer ) layer_destroy( minutes_layer );
  if ( hours_layer ) layer_destroy( hours_layer );
  status_deinit();
  if ( snooze_layer ) layer_destroy( snooze_layer );
  if ( dial_layer ) layer_destroy( dial_layer );
}
