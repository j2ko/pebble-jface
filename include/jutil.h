#ifndef __JUTIL_H__
#define __JUTIL_H__

#include <pebble.h>
#include <math.h>

#ifndef M_PI 
  #define M_PI 3.1415926535897932384626433832795F 
#endif

typedef struct {
    double x;
    double y;
} Point;

static double deg_to_rad(int32_t angle) {
  return ((double)angle* M_PI)/180.0F;
}
static double rad_to_deg(double angle) {
   return ((angle*180.0F)/M_PI); 
} 

#define Point(x, y) ((Point){(x), (y)})
#define TO_GP(p) (GPoint((p.x-0.5), (p.y+0.5)))

#define DEG_TO_RAD deg_to_rad
#define RAD_TO_DEG rad_to_deg

#endif// __JUTIL_H__
