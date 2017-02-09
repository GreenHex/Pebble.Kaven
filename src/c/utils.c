//
// Copyright (C) 2016, Vinodh Kumar M. <GreenHex@gmail.com>
//

#include <pebble.h>
#include "utils.h"
#include "global.h"

#define DEBUG

void log_pt( char *str, GPoint pt ) {
  #ifdef DEBUG
  APP_LOG( APP_LOG_LEVEL_INFO, "%s: ( %d, %d )", str, pt.x, pt.y );
  #endif
}

void log_rect( char *str, GRect rect ) {
  #ifdef DEBUG
  APP_LOG( APP_LOG_LEVEL_INFO, "%s: ( %d, %d, %d, %d )", str, rect.origin.x, rect.origin.y, rect.size.w, rect.size.h );
  #endif
}

void draw_seconds_ticks( DRAW_TICKS_PARAMS *pDTP ) {
  GRect bounds = layer_get_bounds( pDTP->layer );
  GPoint center_pt = grect_center_point( &bounds );
  GPath *gpath = gpath_create( pDTP->p_gpath_info );
  graphics_context_set_antialiased( pDTP->ctx, true );
  
  for ( int i = 0, angle = 0; i < 30; i += pDTP->increment ) { // drawing half the ticks is *probably* more efficient
    angle = TRIG_MAX_ANGLE * i / 60;
    gpath_rotate_to( gpath, angle );
    gpath_move_to( gpath, center_pt );
    graphics_context_set_stroke_color( pDTP->ctx, pDTP->tick_colour );
    graphics_context_set_stroke_width( pDTP->ctx, pDTP->tick_thk );
    gpath_draw_outline( pDTP->ctx, gpath );
  }
  graphics_context_set_fill_color( pDTP->ctx, GColorBlack /* pDTP->bg_colour */ );
  graphics_fill_circle( pDTP->ctx, center_pt, CLOCK_DIAL_SIZE_W/2 - 40 );
  // graphics_fill_rect( pDTP->ctx, grect_inset( bounds, GEdgeInsets( pDTP->tick_length ) ), 0, GCornerNone );
  gpath_destroy( gpath );
}

void draw_clock_hand( HAND_DRAW_PARAMS *pDP ) {
  graphics_context_set_antialiased( pDP->ctx, true );
  // dot outline
  graphics_context_set_stroke_color( pDP->ctx, pDP->dot_outline_colour );
  graphics_context_set_stroke_width( pDP->ctx, 1 );
  graphics_draw_circle( pDP->ctx, pDP->center_pt, pDP->dot_radius );
  // hand outline
  graphics_context_set_stroke_color( pDP->ctx, pDP->hand_outline_colour );
  graphics_context_set_stroke_width( pDP->ctx, pDP->hand_width + 2 );
  graphics_draw_line( pDP->ctx, pDP->from_pt, pDP->to_pt );
  // hand
  graphics_context_set_stroke_color( pDP->ctx, pDP->hand_colour );
  graphics_context_set_stroke_width( pDP->ctx, pDP->hand_width );
  graphics_draw_line( pDP->ctx, pDP->from_pt, pDP->to_pt );
  //
  /*
  graphics_context_set_stroke_color( pDP->ctx, pDP->hand_outline_colour );
  graphics_context_set_stroke_width( pDP->ctx, pDP->hand_width - 4 );
  graphics_draw_line( pDP->ctx, pDP->from_pt, pDP->to_pt );
  */
  // dot
  graphics_context_set_fill_color( pDP->ctx, pDP->dot_colour );
  graphics_fill_circle( pDP->ctx, pDP->center_pt, pDP->dot_radius - 1 );
  /*
  // dot outline
  graphics_context_set_stroke_color( pDP->ctx, GColorDarkGray );
  graphics_context_set_stroke_width( pDP->ctx, 1 );
  graphics_draw_circle( pDP->ctx, pDP->center_pt, pDP->dot_radius - 2 );
  */
  
}

void change_layer_colours( GContext *ctx, Layer *layer,
                   GColor old_bg_colour, GColor new_bg_colour,
                   GColor old_fg_colour, GColor new_fg_colour ) {
  #if defined( PBL_COLOR )
  // if ( gcolor_equal( old_bg_colour, GColorWhite ) ) return;
  GRect frame = layer_get_frame( layer );
  GPoint origin = layer_convert_point_to_screen( layer, GPointZero );
  GBitmap *fb = graphics_capture_frame_buffer( ctx );
  for( int y = 0; y < frame.size.h + 1; y++ ) {
    GBitmapDataRowInfo row = gbitmap_get_data_row_info( fb, origin.y + y - 1 );    
    for ( int x = 0; x < frame.size.w + 1; x++ ) {
      GColor pixel_colour = (GColor) { .argb = row.data[ origin.x + x ] };
      if ( gcolor_equal( pixel_colour, old_bg_colour ) ) {
        memset( &row.data[ origin.x + x ], new_bg_colour.argb, 1 );        
      } else if ( gcolor_equal( pixel_colour, old_fg_colour ) ) {
        memset( &row.data[ origin.x + x ], new_fg_colour.argb, 1 );
      }
    } 
  }
  graphics_release_frame_buffer( ctx, fb );
  #endif
}

void make_outline( GContext *ctx, Layer *layer, GColor fgColour, GColor outlineColor ) {
  #if defined( PBL_COLOR )
  GRect frame = layer_get_frame( layer );
  GPoint origin = layer_convert_point_to_screen( layer, GPointZero );
  GBitmap *fb = graphics_capture_frame_buffer( ctx );
  
  for( int y = 1; y < frame.size.h - 1; y++ ) {
    GBitmapDataRowInfo r0 = gbitmap_get_data_row_info( fb, origin.y + y - 1 );
    GBitmapDataRowInfo r1 = gbitmap_get_data_row_info( fb, origin.y + y );
    GBitmapDataRowInfo r2 = gbitmap_get_data_row_info( fb, origin.y + y + 1 );
    
    for ( int x = 1; x < frame.size.w - 1; x++ ) {
      GColor c0r0 = (GColor) { .argb = r0.data[ origin.x + x - 1 ] };
      GColor c1r0 = (GColor) { .argb = r0.data[ origin.x + x ] };
      GColor c2r0 = (GColor) { .argb = r0.data[ origin.x + x + 1 ] };
      GColor c0r1 = (GColor) { .argb = r1.data[ origin.x + x - 1 ] };
      GColor c1r1 = (GColor) { .argb = r1.data[ origin.x + x ] };
      GColor c2r1 = (GColor) { .argb = r1.data[ origin.x + x + 1 ] };
      GColor c0r2 = (GColor) { .argb = r2.data[ origin.x + x - 1 ] };
      GColor c1r2 = (GColor) { .argb = r2.data[ origin.x + x ] };
      GColor c2r2 = (GColor) { .argb = r2.data[ origin.x + x + 1 ] };
     
      if ( gcolor_equal( c1r1, GColorWhite ) ) {
        if ( gcolor_equal( c0r0, fgColour ) || gcolor_equal( c1r0, fgColour ) || gcolor_equal( c2r0, fgColour ) ||
            gcolor_equal( c0r1, fgColour ) || gcolor_equal( c2r1, fgColour ) ||
            gcolor_equal( c0r2, fgColour ) || gcolor_equal( c1r2, fgColour ) || gcolor_equal( c2r2, fgColour ) ) {
          memset( &r1.data[ origin.x + x ], outlineColor.argb, 1 );
        }
      }
    } 
  }
  graphics_release_frame_buffer( ctx, fb );
  #endif
}
