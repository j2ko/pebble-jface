#include <pebble.h>

#ifndef DEBUG
#define TIME_FMT_24_STR "%I:%M" 
#define TIME_HANDLER_UNIT MINUTE_UNIT
#define DLOG(MSG)
#define TRACE()
#else
#define TIME_FMT_24_STR "%M:%S"
#define TIME_HANDLER_UNIT SECOND_UNIT
#define DLOG(MSG) printf(MSG)
#define TRACE() printf("%s\n",__FUNCTION__)
#endif
#define TIME_FMT_12_STR "%I:%M"
#define KEY_BACKGROUND_COLOR 0
#define KEY_TIME_COLOR 1
#define KEY_DATE_COLOR 2
#define KEY_TWENTY_FOUR_HOUR_FORMAT 3
#define KEY_DEFAULT_CHANGED 4

static const char * DATE_FMT = "%a %d  | %b";

//Watchface ctx, holds reference to all resources
//I know that for embedded static is preferred but 
//for me it's looks more handy
typedef struct {
    GRect canvas_bounds;
    BatteryChargeState battery_state;
    Window *main_window;
    TextLayer *time_layer;
    TextLayer *date_layer;
    BitmapLayer *bitmap_layer;
    Layer *canvas_layer;
    GBitmap *bitmap;
    bool is_connected;
    struct {
       uint32_t backgroundColor;
       uint32_t timeColor;
       uint32_t dateColor;
       bool is_time_format_24;
    } settings;
} krang_ctx_t;

//For now using this function as context holder/getter.
krang_ctx_t* get_context() {
    static krang_ctx_t krang_ctx;

    return &krang_ctx;
}

static krang_ctx_t* load_persistent_data(krang_ctx_t* ctx) {
   TRACE();
   if (persist_read_bool(KEY_DEFAULT_CHANGED)) {
       ctx->settings.backgroundColor = persist_read_int(KEY_BACKGROUND_COLOR);
       ctx->settings.timeColor = persist_read_int(KEY_TIME_COLOR);           
       ctx->settings.dateColor = persist_read_int(KEY_DATE_COLOR);           
       ctx->settings.is_time_format_24 = 
           persist_read_bool(KEY_TWENTY_FOUR_HOUR_FORMAT);
   } else {
       //Default config (fresh install)
       ctx->settings.backgroundColor = 0xFFAA00;// YellowChrome
       ctx->settings.timeColor = 0x5500AA; // Indigo        
       ctx->settings.dateColor = 0xFFFFFF; // White 
       ctx->settings.is_time_format_24 = true; 
   }
   
   return ctx;
}

const char* get_timeformat(bool is_24_hour) {
   return is_24_hour ? TIME_FMT_24_STR : TIME_FMT_12_STR; 
}

static void update_time(struct tm *tick_time) {
    TRACE();
    if (tick_time == NULL) {
        time_t now = time(NULL);
        tick_time = localtime(&now);
    }
    
    static char time_buff[6];
    strftime(time_buff, sizeof(time_buff), 
            get_timeformat(get_context()->settings.is_time_format_24), 
            tick_time);
    text_layer_set_text(get_context()->time_layer, time_buff);

    static char date_buff[32];
    strftime(date_buff, sizeof(date_buff), DATE_FMT, tick_time);
    text_layer_set_text(get_context()->date_layer, date_buff);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    TRACE();
    update_time(tick_time);
}

static void battery_handler(BatteryChargeState new_state) {
    TRACE();
    get_context()->battery_state = new_state;
    layer_mark_dirty(get_context()->canvas_layer);
}

static void connection_handler(bool is_connected) {
    TRACE();
    get_context()->is_connected = is_connected;
    layer_mark_dirty(get_context()->canvas_layer);
    
}

static void invalidate() { 
    TRACE();
    krang_ctx_t *ctx = get_context();
    window_set_background_color(ctx->main_window, GColorFromHEX(ctx->settings.backgroundColor) /*GColorChromeYellow*/ );
    text_layer_set_text_color(ctx->time_layer, GColorFromHEX(ctx->settings.timeColor));
    text_layer_set_text_color(ctx->date_layer, GColorFromHEX(ctx->settings.dateColor));
    update_time(NULL);
    layer_mark_dirty(get_context()->canvas_layer);  
    //todo: invalidate should also refresh all visible items - time/date
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
    TRACE();
    Tuple *background_color_t = dict_find(iter, KEY_BACKGROUND_COLOR);
    Tuple *time_color_t = dict_find(iter, KEY_TIME_COLOR);
    Tuple *date_color_t = dict_find(iter, KEY_DATE_COLOR);
    Tuple *twenty_four_hour_format_t = dict_find(iter, KEY_TWENTY_FOUR_HOUR_FORMAT);

    if (background_color_t) {
        int background_color = background_color_t->value->int32;

        persist_write_int(KEY_BACKGROUND_COLOR, background_color);
    }

    if (time_color_t) {
        int time_color = time_color_t->value->int32;
        persist_write_int(KEY_TIME_COLOR, time_color);
    }

    if (date_color_t) {
        int date_color = date_color_t->value->int32;
        persist_write_int(KEY_DATE_COLOR, date_color);
    }  

    if (twenty_four_hour_format_t) {
        bool twenty_four_hour_format = twenty_four_hour_format_t->value->int8;

        persist_write_int(KEY_TWENTY_FOUR_HOUR_FORMAT, twenty_four_hour_format);
    }
    
    persist_write_bool(KEY_DEFAULT_CHANGED, true);
    load_persistent_data(get_context());
    invalidate();
}

static void main_update_proc(Layer *layer, GContext *gctx) {  
    TRACE();
    krang_ctx_t *ctx = get_context();
    GRect canvas_bounds = ctx->canvas_bounds;

    const int margin = 6;
    const int height = 60;
    GRect border = GRect(margin, 
            canvas_bounds.size.h - height - margin, 
            canvas_bounds.size.w - 2*margin, 
            height);
    GRect inner  = GRect(border.origin.x + 2, 
            border.origin.y + 2, 
            border.size.w - 4, 
            border.size.h - 4);

    //Inner border
    graphics_context_set_stroke_width(gctx, 4);
    graphics_context_set_stroke_color(gctx, GColorOrange);
    graphics_draw_round_rect(gctx, inner, 15);

    //Outer border
    graphics_context_set_stroke_width(gctx, 1);
    graphics_context_set_stroke_color(gctx, GColorBlack);
    graphics_draw_round_rect(gctx, border , 15);

    //Date backround
    GRect date_fix = GRect(margin*2, 
            canvas_bounds.size.h/2 + 23, 
            canvas_bounds.size.w - margin*4, 
            14);
    graphics_context_set_fill_color(gctx, GColorFromHEX(ctx->settings.timeColor));
    graphics_context_set_stroke_color(gctx, GColorIndigo);
    graphics_fill_rect(gctx, date_fix, 0, GCornerNone);

    //Battery indicator
    int charge_percent = ctx->battery_state.is_charging ? 
        100 : ctx->battery_state.charge_percent;

    GRect indicator_border = GRect(margin, margin, 15, 11);    
    GRect battery_level = GRect(margin, margin, (15*charge_percent)/100, 10);
    graphics_context_set_fill_color(gctx, GColorFashionMagenta);
    graphics_fill_rect(gctx, battery_level, 0, GCornerNone);
    graphics_context_set_stroke_color(gctx, GColorIndigo);
    graphics_draw_rect(gctx, indicator_border);
    graphics_draw_rect(gctx, GRect(margin + 15, margin + 3, 2 , 4));

    //Battery charging        
    if (ctx->battery_state.is_charging) {
        GPoint from = GPoint(indicator_border.origin.x + 3, 
                indicator_border.origin.y + 5);
        GPoint to = GPoint(from.x + 3, from.y);
        graphics_context_set_stroke_color(gctx, GColorWhite);
        graphics_draw_line(gctx, from, to);
        graphics_draw_rect(gctx, GRect(to.x, to.y-2, 3, 5));
        graphics_draw_line(gctx, GPoint(to.x + 3, to.y - 1), 
                GPoint(to.x + 4, to.y - 1));
        graphics_draw_line(gctx, GPoint(to.x + 3, to.y + 1), 
                GPoint(to.x + 4, to.y + 1));
    }

    //Bluetooth connection indicator   
    if (ctx->is_connected) 
    {
        //Bluetooth connection indicator vertical
        int width = 18;
        int height = 10;        
        GPoint tmp, pivot = GPoint(canvas_bounds.size.w - margin - width,
                margin + 6);        
        graphics_context_set_stroke_color(gctx, GColorIndigo);

        graphics_draw_line(gctx, pivot, GPoint(pivot.x + width, pivot.y));

        tmp = GPoint(pivot.x + width - 5, pivot.y - 5);        
        graphics_draw_line(gctx, GPoint(pivot.x + width, pivot.y), tmp);
        graphics_draw_line(gctx, GPoint(pivot.x + 5, pivot.y + 3), tmp);

        tmp = GPoint(pivot.x + 5, pivot.y - 5);
        graphics_draw_line(gctx, pivot, tmp);
        graphics_draw_line(gctx, GPoint(pivot.x + width - 5, pivot.y + 3), tmp);        
    }  
}

static void main_window_load(Window *window) {
    TRACE();
    krang_ctx_t *ctx = load_persistent_data(get_context());
    
    window_set_background_color(ctx->main_window, GColorFromHEX(ctx->settings.backgroundColor) /*GColorChromeYellow*/ );


    // Get information about the Window
    Layer *window_layer = window_get_root_layer(window);
    GRect canvas_bounds = layer_get_bounds(window_layer);
    ctx->canvas_bounds = canvas_bounds;

    // Canvas layer initialization  
    ctx->canvas_layer = layer_create(canvas_bounds);
    layer_set_update_proc(ctx->canvas_layer, main_update_proc);

    // Bitmap layer
    ctx->bitmap = gbitmap_create_with_resource(RESOURCE_ID_KRANG);
    ctx->bitmap_layer = bitmap_layer_create(canvas_bounds);
    bitmap_layer_set_bitmap(ctx->bitmap_layer, ctx->bitmap);
    bitmap_layer_set_compositing_mode(ctx->bitmap_layer, GCompOpSet);
    const int heightTime = 42;
    const int offsetTime = 15;
    ctx->time_layer = text_layer_create(GRect(0, 
                canvas_bounds.size.h - heightTime - offsetTime , 
                canvas_bounds.size.w, heightTime)
            );
    text_layer_set_text_color(ctx->time_layer, GColorFromHEX(ctx->settings.timeColor) /*GColorIndigo*/);
    text_layer_set_background_color(ctx->time_layer, GColorClear);
    text_layer_set_font(ctx->time_layer, 
            fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS)
            );
    text_layer_set_text_alignment(ctx->time_layer, GTextAlignmentCenter);

    //todo: I need custom font for date - to much extra space on top of default
    const int heightDate = 16;
    const int offsetDate = 20;
    ctx->date_layer = text_layer_create(GRect(0, 
                canvas_bounds.size.h/2 + offsetDate, 
                canvas_bounds.size.w, heightDate)
            );
    text_layer_set_text_color(ctx->date_layer, GColorFromHEX(ctx->settings.dateColor));
    text_layer_set_background_color(ctx->date_layer, GColorClear);
    text_layer_set_font(ctx->date_layer, 
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text_alignment(ctx->date_layer, GTextAlignmentCenter);

    //Add it as a child layer to the Window's root layer
    layer_add_child(window_layer, ctx->canvas_layer);
    layer_add_child(window_layer, text_layer_get_layer(ctx->time_layer));
    layer_add_child(window_layer, text_layer_get_layer(ctx->date_layer));
    layer_add_child(window_layer, bitmap_layer_get_layer(ctx->bitmap_layer));
   
    //Initial fill of time/date values
    update_time(NULL);

    //Initial values of battery level/connection state
    ctx->battery_state = battery_state_service_peek();
    ctx->is_connected = connection_service_peek_pebble_app_connection();
}

static void main_window_unload(Window *window) {
    TRACE();
    krang_ctx_t *ctx = get_context();
    
    bitmap_layer_destroy(ctx->bitmap_layer);
    gbitmap_destroy(ctx->bitmap);
    text_layer_destroy(ctx->time_layer);
    text_layer_destroy(ctx->date_layer);
    layer_destroy(ctx->canvas_layer);
}

void handle_init(void) {
    TRACE();
    Window* main_window = window_create();
    get_context()->main_window = main_window;
    
    window_set_window_handlers(main_window, (WindowHandlers) {
            .load = main_window_load,
            .unload = main_window_unload
    }); 

    tick_timer_service_subscribe(TIME_HANDLER_UNIT, tick_handler);
    window_stack_push(main_window, true);

    battery_state_service_subscribe(battery_handler);  
    connection_service_subscribe((ConnectionHandlers) {
            .pebblekit_connection_handler = connection_handler,
    });
    app_message_register_inbox_received(inbox_received_handler);
    app_message_open(app_message_inbox_size_maximum(), 
            app_message_outbox_size_maximum());
}

void handle_deinit(void) {
    TRACE();
    window_destroy(get_context()->main_window);
}

int main(void) {
    TRACE();
    handle_init();
    app_event_loop();
    handle_deinit();
}
