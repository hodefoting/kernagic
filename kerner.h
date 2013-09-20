#ifndef KERNER_H
#define KERNER_H

#include "kernagic.h"

typedef struct _KernerSettings KernerSettings;

#define KERNER_DEFAULT_MODE          0 /* only two at first, gray and period */
#define KERNER_DEFAULT_MIN          15
#define KERNER_DEFAULT_MAX          50
#define KERNER_DEFAULT_DIVISOR       4
#define KERNER_DEFAULT_TARGET_GRAY  50
#define KERNER_DEFAULT_OFFSET        2
#define KERNER_DEFAULT_TRACKING    100

struct _KernerSettings
{
  KernagicMethod *method;
  float minimum_distance;
  float maximum_distance;
  float divisor;
  float alpha_target;
  float offset;
  float tracking;
};
extern KernerSettings kerner_settings;

float kerner_kern (KernerSettings *settings, Glyph *left, Glyph *right);
void init_kerner (void);
void kerner_debug_ui (void);

#endif
