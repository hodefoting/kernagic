#ifndef KERNER_H
#define KERNER_H

#include "kernagic.h"

typedef struct _KernerSettings KernerSettings;

enum {
  KERNAGIC_GRAY,
  KERNAGIC_CADENCE,
  KERNAGIC_RYTHM
};

struct _KernerSettings
{
  int   mode;
  float minimum_distance;
  float maximum_distance;
  float alpha_target;
  float alpha_strength;
  float beta_target;
  float beta_strength;
};
extern KernerSettings kerner_settings;


float kerner_kern (KernerSettings *settings, Glyph *left, Glyph *right);
void init_kerner (void);
void kerner_debug_ui (void);

#endif
