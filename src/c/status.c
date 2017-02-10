#include <pebble.h>
#include "status.h"
#include "utils.h"

extern tm tm_time;
bool show_time = false;

static Layer* status_layer = 0;
static TextLayer *status_text_layer = 0;

static void status_layer_update_proc( Layer *layer, GContext *ctx ) {
  GRect bounds = layer_get_bounds( layer );
  graphics_context_set_fill_color( ctx, foreground_colour );
  graphics_fill_rect( ctx, bounds, CLOCK_CORNER_RADIUS, GCornerNone );  
}

// #define DATE_STRING "%a, %e-%b-%Y"
#define DATE_STRING "%a %e"
#define TIME_STRING "%I:%M"
#define TIME_FULL_HOUR_STRING "%I o'clock"
#define ALT_STATUS_FONT RESOURCE_ID_FONT_PRELUDE_BOLD_26
// #define ALT_STATUS_FONT RESOURCE_ID_FONT_PRELUDE_MEDIUM_30

static void status_text_layer_update_proc( Layer *layer, GContext *ctx ) {
  char str[] = "AAA, DD-MMM-YYYY";
  GRect date_window_bounds = layer_get_bounds( layer );
  date_window_bounds.origin.x += STATUS_TEXT_HOR_ADJ;
  date_window_bounds.origin.y -= STATUS_TEXT_VER_ADJ;
  graphics_context_set_text_color( ctx, PBL_IF_COLOR_ELSE( GColorArmyGreen, GColorBlack ) /* background_colour */ );
  // strftime( str, sizeof( str ), DATE_STRING, &tm_time );
  strftime( str, sizeof( str ), show_time?tm_time.tm_min?TIME_STRING:TIME_FULL_HOUR_STRING:DATE_STRING, &tm_time );
  if ( show_time ) {
    if ( str[0] == '0' ) {
      memmove( &str[0], &str[1], sizeof( str ) - 1 );
    }
  } else { // it's a date
    if ( str[3] == str[4] ) { // remove double spaces between day and single digit date 
      memmove( &str[4], &str[5], sizeof( str ) - 4 );
    }
  }
  #ifdef ALT_STATUS_FONT
  GFont font = fonts_load_custom_font( resource_get_handle( ALT_STATUS_FONT ) );
  graphics_draw_text( ctx, str, font, date_window_bounds,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL );
  fonts_unload_custom_font( font );
  #else
  graphics_draw_text( ctx, str, fonts_get_system_font( FONT_KEY_BITHAM_30_BLACK ), date_window_bounds,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL );
  #endif
  // make_outline( ctx, layer, GColorDarkGray, GColorBlack );
}

void status_init( Layer *parent_layer ) {
  status_layer = layer_create( STATUS_WINDOW_RECT );
  layer_set_update_proc( status_layer, status_layer_update_proc );
  layer_add_child( parent_layer, status_layer );
  
  status_text_layer = text_layer_create( layer_get_bounds( status_layer ) );
  layer_set_update_proc( text_layer_get_layer( status_text_layer ), status_text_layer_update_proc );
  layer_add_child( status_layer, text_layer_get_layer( status_text_layer ) );
}

void status_deinit( void ) {
  if ( status_text_layer ) text_layer_destroy( status_text_layer );
  if ( status_layer ) layer_destroy( status_layer );
}