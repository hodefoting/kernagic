/* implementation of frank bloklands reinaissance spacing scheme */

#include "kernagic.h"

typedef struct Cadence
{
  int   left;
  char *utf8;
  int   right;
  int   flags;
} Cadence;

#define LEFT_EXTREME    (1<<0)
#define RIGHT_EXTREME   (1<<1)
#define RIGHT_STEM      (1<<2)
#define LEFT_STEM       (1<<3)
#define BOTH_EXTREME    (LEFT_EXTREME | RIGHT_EXTREME)
#define BOTH_STEM       (LEFT_STEM | RIGHT_STEM)

/* cadence unit table; from Frank Bloklands image at lettermodel.org august
 * 2013, manually transcribed to code constants by Øyvind Kolås */
Cadence cadence[]={
{1,"A",1,BOTH_EXTREME},
{8,"B",3,LEFT_STEM | RIGHT_EXTREME},
{3,"C",2,BOTH_EXTREME},
{8,"D",3,LEFT_STEM | RIGHT_EXTREME},
{8,"E",2,LEFT_STEM | RIGHT_EXTREME},
{8,"F",2,LEFT_STEM | RIGHT_EXTREME},
{3,"G",6,LEFT_EXTREME | RIGHT_STEM},
{8,"H",8,BOTH_STEM},
{8,"I",8,BOTH_STEM},
{6,"J",6,BOTH_STEM}, /* XXX: left side stem likely too lowe for ismple hit */
{8,"K",1,LEFT_STEM | RIGHT_EXTREME},
{8,"L",2,LEFT_STEM | RIGHT_EXTREME},
{8,"M",8,BOTH_STEM},
{8,"N",8,BOTH_STEM},
{3,"O",3,BOTH_EXTREME},
{8,"P",2,LEFT_STEM | RIGHT_EXTREME},
{3,"Q",3,BOTH_EXTREME},
{8,"R",1,LEFT_STEM | RIGHT_EXTREME},
{3,"S",3,BOTH_EXTREME},
{1,"T",1,BOTH_EXTREME},
{6,"U",6,BOTH_STEM},
{1,"V",1,BOTH_EXTREME},
{1,"W",1,BOTH_EXTREME},
{1,"X",1,BOTH_EXTREME},
{1,"Y",1,BOTH_EXTREME},
{2,"Z",2,BOTH_EXTREME},
{2,"a",6,LEFT_EXTREME| RIGHT_STEM},
{5,"b",2,LEFT_STEM | RIGHT_EXTREME},
{2,"c",1,BOTH_EXTREME},
{2,"d",6,LEFT_EXTREME | RIGHT_STEM},
{2,"e",2,BOTH_EXTREME},
{6,"f",1,LEFT_STEM | RIGHT_EXTREME},
{2,"g",1,BOTH_EXTREME},
{6,"h",6,BOTH_STEM},
{6,"i",6,BOTH_STEM},
{6,"j",5,BOTH_STEM},
{6,"k",1,LEFT_STEM | RIGHT_EXTREME},
{6,"l",6,BOTH_STEM},
{6,"m",6,BOTH_STEM},
{6,"n",6,BOTH_STEM},
{2,"o",2,BOTH_EXTREME},
{6,"p",2,LEFT_STEM | RIGHT_EXTREME},
{2,"q",5,LEFT_EXTREME| RIGHT_STEM},
{6,"r",1,LEFT_STEM | RIGHT_EXTREME},
{3,"s",3,BOTH_EXTREME},
{5,"t",1,LEFT_STEM | RIGHT_EXTREME},
{5,"u",5,BOTH_STEM},
{1,"v",1,BOTH_EXTREME},
{1,"w",1,BOTH_EXTREME},
{1,"x",1,BOTH_EXTREME},
{1,"y",1,BOTH_EXTREME},
{2,"z",2,BOTH_EXTREME},
{3,".",3,BOTH_EXTREME},
{3,":",3,BOTH_EXTREME},
{3,";",3,BOTH_EXTREME},
{3,",",3,BOTH_EXTREME}};

extern float scale_factor;

float left_most_center (Glyph *g)
{
  return g->leftmost[(int)(kernagic_x_height() * 1.5 * scale_factor)] / scale_factor;
}

float right_most_center (Glyph *g)
{
  return g->rightmost[(int)(kernagic_x_height() * 1.5 * scale_factor)] / scale_factor;
}

Cadence *glyph_get_cadence (Glyph *g)
{
  int i;
  for (i = 0; i < sizeof (cadence)/sizeof(cadence[0]) - 1; i++)
    {
      if (cadence[i].utf8[0] == g->unicode)
        return &cadence[i];
    }
  return &cadence[0];
}

float n_width = 0;

static void cadence_init (void)
{
  Glyph *g = kernagic_find_glyph_unicode ('n');
  if (!g)
    return;

  n_width = (right_most_center (g) - left_most_center(g)) / 24.0;
}

static void cadence_each (Glyph *g, GtkProgressBar *progress)
{
  float left;
  float right;
  Cadence *c;
  c = glyph_get_cadence (g);

  left = n_width * c->left;
  right = n_width * c->right;

  if (c->flags & LEFT_EXTREME)
    left -= g->ink_min_x;
  else
    left -= left_most_center(g);

  if (c->flags & RIGHT_EXTREME)
    right -= 0;
  else
    right -= (g->ink_max_x - right_most_center(g));

  kernagic_set_left_bearing (g,  left);
  kernagic_set_right_bearing (g, right);
}

static KernagicMethod method = {
  "cadence", 
  cadence_init,
  cadence_each,
  NULL
};

KernagicMethod *kernagic_cadence = &method;
