#ifndef KERNER_H
#define KERNER_H

#include "kernagic.h"

typedef struct _KernerSettings KernerSettings;

#define KERNER_DEFAULT_MODE          0 /* only two at first, gray and cadence */
#define KERNER_DEFAULT_MIN          15
#define KERNER_DEFAULT_MAX          50
#define KERNER_DEFAULT_TARGET_GRAY  50
#define KERNER_DEFAULT_TRACKING    100
#define KERNER_DEFAULT_WEIGHT_GRAY 100
#define KERNER_DEFAULT_TARGET_FOO   50
#define KERNER_DEFAULT_WEIGHT_FOO    0

struct _KernerSettings
{
  KernagicMethod *method;
  float minimum_distance;
  float maximum_distance;
  float alpha_target;
  float tracking;
};
extern KernerSettings kerner_settings;


float kerner_kern (KernerSettings *settings, Glyph *left, Glyph *right);
void init_kerner (void);
void kerner_debug_ui (void);

#endif
