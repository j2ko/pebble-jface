
#include <pebble.h>
#include <include/jutil.h>
#define DEBUG

//--- STATIC -----
//todo: define app context to hold state.
Window *s_main_window;
TextLayer *s_time_layer;
TextLayer *s_date_layer;
Layer* s_canvas_layer;
GRect s_canvas_bounds;

static GBitmap *s_bitmap;
static BitmapLayer *s_bitmap_layer;
BatteryChargeState s_battery_state;
bool  s_is_connected;
#ifndef DEBUG
#define TIME_FMT_STR "%H:%M" 
#else
#define TIME_FMT_STR "%M:%S"
#endif
static const char * TIME_FMT = TIME_FMT_STR;
static const char * DATE_FMT = "%a %d  | %b";

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  static char time_buff[] = "00:00";
  strftime(time_buff, sizeof(time_buff), TIME_FMT, tick_time);
  text_layer_set_text(s_time_layer, time_buff);
  
  static char date_buff[32];
  strftime(date_buff, sizeof(date_buff), DATE_FMT, tick_time);
  text_layer_set_text(s_date_layer, date_buff);
}

static void battery_handler(BatteryChargeState new_state) {
  // Todo : move battery state to app context
  s_battery_state = new_state;
  layer_mark_dirty(s_canvas_layer);
}

static void connection_handler(bool is_connected) {
  s_is_connected = is_connected;
  layer_mark_dirty(s_canvas_layer);
}

static void main_update_proc(Layer *layer, GContext *ctx) {  
    const int margin = 6;
    const int height = 60;
    GRect border = GRect(margin, s_canvas_bounds.size.h - height - margin, s_canvas_bounds.size.w - 2*margin, height);
    GRect inner  = GRect(border.origin.x + 2, border.origin.y + 2, border.size.w - 4, border.size.h - 4);
    
    //Inner border
    graphics_context_set_stroke_width(ctx, 4);
    graphics_context_set_stroke_color(ctx, GColorOrange);
    graphics_draw_round_rect(ctx, inner, 15);
    
    //Outer border
    graphics_context_set_stroke_width(ctx, 1);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_round_rect(ctx, border , 15);
    
    //Date backround
    GRect date_fix = GRect(margin*2, s_canvas_bounds.size.h/2 + 23, s_canvas_bounds.size.w - margin*4, 14);
    graphics_context_set_fill_color(ctx, GColorIndigo);
    graphics_context_set_stroke_color(ctx, GColorIndigo);
    graphics_fill_rect(ctx, date_fix, 0, GCornerNone);
    
    //Battery indicator
    int charge_percent = s_battery_state.is_charging ? 100 : s_battery_state.charge_percent;
    
    GRect indicator_border = GRect(margin, margin, 15, 11);    
    GRect battery_level = GRect(margin, margin, (15*charge_percent)/100, 10);
    graphics_context_set_fill_color(ctx, GColorFashionMagenta);
    graphics_fill_rect(ctx, battery_level, 0, GCornerNone);
    graphics_context_set_stroke_color(ctx, GColorIndigo);
    graphics_draw_rect(ctx, indicator_border);
    graphics_draw_rect(ctx, GRect(margin + 15, margin + 3, 2 , 4));
    
    //Battery charging        
    if (s_battery_state.is_charging) {
        GPoint from = GPoint(indicator_border.origin.x + 3, indicator_border.origin.y + 5);
        GPoint to = GPoint(from.x + 3, from.y);
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_draw_line(ctx, from, to);
        graphics_draw_rect(ctx, GRect(to.x, to.y-2, 3, 5));
        graphics_draw_line(ctx, GPoint(to.x + 3, to.y - 1), GPoint(to.x + 4, to.y - 1));
        graphics_draw_line(ctx, GPoint(to.x + 3, to.y + 1), GPoint(to.x + 4, to.y + 1));
    }
    
    //Bluetooth connection indicator   
    if (s_is_connected) 
    {
        //Bluetooth connection indicator vertical
        int width = 18;
        int height = 10;        
        GPoint tmp, pivot = GPoint(s_canvas_bounds.size.w - margin - width, margin + 6);        
        graphics_context_set_stroke_color(ctx, GColorIndigo);
        
        graphics_draw_line(ctx, pivot, GPoint(pivot.x + width, pivot.y));
        
        tmp = GPoint(pivot.x + width - 5, pivot.y - 5);        
        graphics_draw_line(ctx, GPoint(pivot.x + width, pivot.y), tmp);
        graphics_draw_line(ctx, GPoint(pivot.x + 5, pivot.y + 3), tmp);
        
        tmp = GPoint(pivot.x + 5, pivot.y - 5);
        graphics_draw_line(ctx, pivot, tmp);
        graphics_draw_line(ctx, GPoint(pivot.x + width - 5, pivot.y + 3), tmp);        
    }  
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  s_canvas_bounds = layer_get_bounds(window_layer);
  
  // Canvas layer initialization  
  s_canvas_layer = layer_create(s_canvas_bounds);
  layer_set_update_proc(s_canvas_layer, main_update_proc);
  
  // Bitmap layer
  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_KRANG);
  s_bitmap_layer = bitmap_layer_create(s_canvas_bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  const int heightTime = 42;
  const int offsetTime = 15;
  s_time_layer = text_layer_create(GRect(0, s_canvas_bounds.size.h - heightTime - offsetTime , s_canvas_bounds.size.w, heightTime));
  text_layer_set_text_color(s_time_layer, GColorIndigo);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  //todo: I need custom font for date - to much extra space on top of default
  const int heightDate = 16;
  const int offsetDate = 20;
  s_date_layer = text_layer_create(GRect(0, s_canvas_bounds.size.h/2 + offsetDate , s_canvas_bounds.size.w, heightDate));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, s_canvas_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
  
  //Initial fill of time/date values
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  tick_handler(t, 0);
  
  //Initial values of battery level/connection state
  s_battery_state = battery_state_service_peek();
  s_is_connected = connection_service_peek_pebble_app_connection();
}

static void main_window_unload(Window *window) {
  bitmap_layer_destroy(s_bitmap_layer);
  gbitmap_destroy(s_bitmap);
  layer_destroy(s_canvas_layer);
}

void handle_init(void) {
  s_main_window = window_create();
  window_set_background_color(s_main_window,GColorChromeYellow);

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  }); 
  
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  window_stack_push(s_main_window, true);
  battery_state_service_subscribe(battery_handler);  
  connection_service_subscribe((ConnectionHandlers) {
           .pebblekit_connection_handler = connection_handler,
  });
}

void handle_deinit(void) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  window_destroy(s_main_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
