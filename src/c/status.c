#include <pebble.h>
#include "status.h"
#include "utils.h"

extern tm tm_time;
bool show_name = false;

static Layer* status_layer = 0;
static TextLayer *status_text_layer = 0;

static void status_layer_update_proc( Layer *layer, GContext *ctx ) {
  GRect bounds = layer_get_bounds( layer );
  graphics_context_set_fill_color( ctx, foreground_colour );
  graphics_fill_rect( ctx, bounds, CLOCK_CORNER_RADIUS, GCornerNone );  
}

// #define DATE_STRING "%a, %e-%b-%Y"
#define DATE_STRING "%a %e"
#define ALT_STATUS_FONT RESOURCE_ID_FONT_PRELUDE_BOLD_26
// #define ALT_STATUS_FONT RESOURCE_ID_FONT_PRELUDE_MEDIUM_30

static void status_text_layer_update_proc( Layer *layer, GContext *ctx ) {
  char date_str[] = "AAA, DD-MMM-YYYY";
  GRect date_window_bounds = layer_get_bounds( layer );
  date_window_bounds.origin.x += STATUS_TEXT_HOR_ADJ;
  date_window_bounds.origin.y -= STATUS_TEXT_VER_ADJ;
  graphics_context_set_text_color( ctx, PBL_IF_COLOR_ELSE( GColorArmyGreen, GColorBlack ) /* background_colour */ );
  // strftime( date_str, sizeof( date_str ), show_name?"KAVAN":DATE_STRING, &tm_time );
  strftime( date_str, sizeof( date_str ), DATE_STRING, &tm_time );
  if ( date_str[3] == date_str[4] ) { // remove double spaces between day and single digit date 
    memmove( &date_str[4], &date_str[5], sizeof( date_str ) - 4 );
  }
  #ifdef ALT_STATUS_FONT
  GFont font = fonts_load_custom_font( resource_get_handle( ALT_STATUS_FONT ) );
  graphics_draw_text( ctx, date_str, font, date_window_bounds,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL );
  fonts_unload_custom_font( font );
  #else
  graphics_draw_text( ctx, date_str, fonts_get_system_font( FONT_KEY_BITHAM_30_BLACK ), date_window_bounds,
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