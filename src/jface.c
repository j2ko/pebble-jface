
#include <pebble.h>
#include <include/jutil.h>
//Todo: define app context to hold state.
//--- STATIC -----
Window *s_main_window;
TextLayer *s_time_layer;
Layer* s_canvas_layer;
GRect s_canvas_bounds;
int s_x = 0;
int s_direction = 1;
//----------------

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  unsigned int changed = (unsigned int)units_changed;
  
  s_x = (s_x + s_direction * units_changed)%36;  
  layer_mark_dirty(s_canvas_layer);
}

static Point rotate_point(Point p, Point anchor, double angle) {
  double s = sin(angle);
  double c = cos(angle);

  // translate point back to origin:
  p.x -= anchor.x;
  p.y -= anchor.y;

  // rotate point
  double xnew = p.x * c - p.y * s; 
  double ynew = p.x * s + p.y * c;
    
  return Point(xnew + anchor.x, ynew + anchor.y);
}

typedef void (*draw_quadrilateral_t)(Point pcenter, GContext *ctx, int sides, float rotation,float radius);

void draw_quadrilateral(Point pcenter, GContext *ctx, int sides, float rotation,float radius) {
  float dalpha = DEG_TO_RAD(360.0/(float)sides);  
  Point pnext, p0 = Point(pcenter.x, pcenter.y - radius);
  p0 = rotate_point(p0, pcenter, rotation);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_pixel(ctx, TO_GP(pcenter));
  for (int i = 0; i < sides; ++i) {    
    graphics_context_set_stroke_color(ctx, GColorRed );      
    pnext = rotate_point(p0, pcenter, dalpha);    ;    
    graphics_draw_line(ctx, TO_GP(p0), TO_GP(pnext));
    p0 = pnext;
  }
}

void draw_quadrilateral_path(Point pcenter, GContext *ctx, int sides, float rotation,float radius) {
  double dalpha = DEG_TO_RAD(360.0F/(double)sides); 
  Point p = Point(pcenter.x, pcenter.y - radius);
  p = rotate_point(p, pcenter, rotation);
  graphics_context_set_stroke_width(ctx, 2);
  GPathInfo pathinfo;
  pathinfo.num_points = sides;
  pathinfo.points = (GPoint*)malloc(sizeof(GPoint)*sides);  
  for (int i = 0; i < sides; ++i) {
      pathinfo.points[i] = TO_GP(p); //    pathinfo.points[i] = p;
      p = rotate_point(p, pcenter, dalpha);
  }
  
  GPath *path = gpath_create(&pathinfo);
  graphics_context_set_fill_color(ctx, GColorGreen);
  gpath_draw_filled(ctx, path);
  free(pathinfo.points);
  free(path);
}


static void main_update_proc(Layer *layer, GContext *ctx) {  
  Point center = Point(s_canvas_bounds.size.w/2.0, s_canvas_bounds.size.h/2.0);  
  graphics_context_set_stroke_width(ctx, 5);
  double radius = 20;
  int   sides = 4;
  double angle = DEG_TO_RAD(s_x*10);
  draw_quadrilateral_t drawQuad = s_x%2 ==0 ? draw_quadrilateral : draw_quadrilateral_path;

  graphics_context_set_antialiased (ctx, true);
  drawQuad(center, ctx, sides, angle, radius);
  Point temp = Point(center.x-2*radius, center.y);
  Point temp2 = Point(center.x, center.y-3.5*radius);
  temp = rotate_point(temp, center, angle);
  temp2 = rotate_point(temp2, center, angle);
  for (int i = 0; i < 6; ++i) {   
    drawQuad(temp, ctx, sides, angle, radius);  
    drawQuad(temp2, ctx, sides, angle, radius);  
    temp = rotate_point(temp, center, DEG_TO_RAD(60));
    temp2 = rotate_point(temp2, center, DEG_TO_RAD(60));
  }
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  s_canvas_bounds = layer_get_bounds(window_layer);
  s_canvas_layer = layer_create(s_canvas_bounds);
  layer_set_update_proc(s_canvas_layer, main_update_proc);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, s_canvas_layer);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

void handle_init(void) {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  }); 
  
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  window_stack_push(s_main_window, true);
}

void handle_deinit(void) {
  text_layer_destroy(s_time_layer);
  window_destroy(s_main_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
