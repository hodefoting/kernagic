/* implementation of frank bloklands reinaissance spacing scheme */

#include <string.h>
#include <stdlib.h>
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

#define MAX_GLYPHS 4096

float n_basis = 24;

/* cadence unit table; from Frank Bloklands image at lettermodel.org august
 * 2013, manually transcribed to code constants by Øyvind Kolås */
Cadence cadence[MAX_GLYPHS]={
{2,"A",2,BOTH_EXTREME},
{8,"B",3,LEFT_STEM | RIGHT_EXTREME},
{3,"C",2,BOTH_EXTREME},
{8,"D",3,LEFT_STEM | RIGHT_EXTREME},
{8,"E",2,LEFT_STEM | RIGHT_EXTREME},
{8,"F",2,LEFT_STEM | RIGHT_EXTREME},
{3,"G",6,LEFT_EXTREME | RIGHT_STEM},
{8,"H",8,BOTH_STEM},
{8,"I",8,BOTH_STEM},
{6,"J",6,BOTH_STEM},
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
{5,"u",6,BOTH_STEM},
{1,"v",1,BOTH_EXTREME},
{1,"w",1,BOTH_EXTREME},
{1,"x",1,BOTH_EXTREME},
{1,"y",1,BOTH_EXTREME},
{2,"z",2,BOTH_EXTREME},
{3,".",3,BOTH_EXTREME},
{3,":",3,BOTH_EXTREME},
{3,";",3,BOTH_EXTREME},
{3,",",3,BOTH_EXTREME}
};

int n_cadence = 56;

static void cadence_add (const char *utf8,
                         int   left_stem,
                         float left,
                         int   right_stem,
                         float right)
{
  cadence[n_cadence].utf8 = g_strdup (utf8);
  cadence[n_cadence].left = left;
  cadence[n_cadence].right = right;
  cadence[n_cadence].flags = 0;
  if (left_stem)
    cadence[n_cadence].flags |= LEFT_STEM;
  else
    cadence[n_cadence].flags |= LEFT_EXTREME;
  if (right_stem)
    cadence[n_cadence].flags |= RIGHT_STEM;
  else
    cadence[n_cadence].flags |= RIGHT_EXTREME;
  n_cadence++;
}


enum {
  S_E_CHAR = 0,
  S_IN_CHAR,
  S_IN_COMMENT,
  S_E_LEFT_MODE,
  S_IN_LEFT_MODE,
  S_E_LEFT_VAL,
  S_IN_LEFT_VAL,
  S_E_RIGHT_MODE,
  S_IN_RIGHT_MODE,
  S_E_RIGHT_VAL,
  S_IN_RIGHT_VAL,
  S_IN_BASIS,
  S_DONE
};

static void cadence_init (void);

void kernagic_set_cadence (const char *cadence_path)
{
  gchar * input;
  g_file_get_contents (cadence_path, &input, NULL, NULL);
  if (input)
  {
    gchar *p;
    GString *tmp = g_string_new ("");

    int state = S_E_CHAR;
    char *utf8 = NULL;
    float left =0, right= 0;
    int left_mode = 0;
    int right_mode = 0;

    n_cadence = 0;
    for (p = input; *p; p++)
      {
        switch (*p)
          {
            case '#':  /* bit of a hack; trying to be tolerant, means
                          this char has to be encoded with U+ .. 
                          instead of utf8 when parsing is extended 
                          to also do such parsing */
              state = S_IN_COMMENT;
            if (state <= S_IN_RIGHT_VAL)
               break;
            case '\n':
              if (state == S_IN_RIGHT_VAL)
              { 
                right = atof (tmp->str);
              }
              else if (state == S_IN_BASIS)
              {
                n_basis = g_strtod (tmp->str, NULL);
                state = S_E_CHAR;
                g_string_assign (tmp, "");
                cadence_init ();
                break;
              }

              if (state >= S_E_RIGHT_VAL)
                cadence_add (utf8, left_mode, left, right_mode, right);

              state = S_E_CHAR;
              g_string_assign (tmp, "");
              break;
            case ' ':
              switch (state)
                {
                  case S_IN_COMMENT:
                  case S_E_CHAR: 
                  case S_E_LEFT_MODE:
                  case S_E_LEFT_VAL:
                  case S_E_RIGHT_MODE:
                  case S_E_RIGHT_VAL:
                    break;

                  case S_IN_CHAR:
                    {
                      if (utf8)
                        g_free (utf8);
                      utf8 = g_strdup (tmp->str);
                      state = S_E_LEFT_MODE;
                      g_string_assign (tmp, "");
                      if (!strcmp (utf8, "n_basis"))
                      {
                        state = S_IN_BASIS;
                      }
                      break;
                    }
                  case S_IN_LEFT_MODE:
                    {
                      if (!strcmp (tmp->str, "stem"))
                        left_mode = 1;
                      else
                        left_mode = 0;
                      state = S_E_LEFT_VAL;
                      g_string_assign (tmp, "");
                      break;
                    }
                  case S_IN_LEFT_VAL:
                    {
                      left = atof (tmp->str);
                      state = S_E_RIGHT_MODE;
                      g_string_assign (tmp, "");
                      break;
                    }
                  case S_IN_RIGHT_MODE:
                    {
                      if (!strcmp (tmp->str, "stem"))
                        right_mode = 1;
                      else
                        right_mode = 0;
                      state = S_E_RIGHT_VAL;
                      g_string_assign (tmp, "");
                      break;
                    }
                  case S_IN_RIGHT_VAL:
                    {
                      right = atof (tmp->str);
                      state = S_DONE;
                      g_string_assign (tmp, "");
                      break;
                    }
                }
              break;
            default:
              switch (state)
                {
                  case S_E_CHAR: 
                    state = S_IN_CHAR;
                    break;
                  case S_E_LEFT_MODE:
                    state = S_IN_LEFT_MODE;
                    break;
                  case S_E_LEFT_VAL:
                    state = S_IN_LEFT_VAL;
                    break;
                  case S_E_RIGHT_MODE:
                    state = S_IN_RIGHT_MODE;
                    break;
                  case S_E_RIGHT_VAL:
                    state = S_IN_RIGHT_VAL;
                    break;
                  default:
                    break;
                }
              g_string_append_c (tmp, *p);
          }
      }
    g_string_free (tmp, TRUE);
    g_free (input);
  }
  else
  {
  }
}

extern float scale_factor;

float left_most_center (Glyph *g)
{
  /* we only look at a single scan-line in the middle of the x-height */
  return g->leftmost[(int)(kernagic_x_height() * 1.5 * scale_factor)] / scale_factor;
}

float right_most_center (Glyph *g)
{
  /* we only look at a single scan-line in the middle of the x-height */
  return g->rightmost[(int)(kernagic_x_height() * 1.5 * scale_factor)] / scale_factor;
}

static float left_extreme (Glyph *g)
{
  /* due to how our curve have been clipped eaelier; 0 left most pixel
   * with ink in the whole glyph */
  return 0.0;
}

static float right_extreme (Glyph *g)
{
  float max = 0;
  for (int y = 0; y < kernagic_x_height () * 2 * scale_factor; y ++)
    if (g->rightmost[y] > max)
      max = g->rightmost[y];
  return max / scale_factor;
}

Cadence *glyph_get_cadence (Glyph *g)
{
  unsigned int i;
  for (i = 0; i < n_cadence; i++)
    {
      if (cadence[i].utf8[0] == g->unicode)
        return &cadence[i];
    }

  /* this fallback is not very good.
   *
   * Non-found glyphs get the treatment of 'A' - other more automatic
   * remappings could perhaps be done to improve the range of fonts this
   * method can be applied to.
   */

  return &cadence[0];
}

static float n_width = 0;

#define LEFT_EXTREME    (1<<0)
#define RIGHT_EXTREME   (1<<1)
#define RIGHT_STEM      (1<<2)
#define LEFT_STEM       (1<<3)
#define BOTH_EXTREME    (LEFT_EXTREME | RIGHT_EXTREME)
#define BOTH_STEM       (LEFT_STEM | RIGHT_STEM)

static void cadence_init (void)
{
  Glyph *g = kernagic_find_glyph_unicode ('n');
  if (!g)
    return;
  n_width = (right_most_center (g) - left_most_center(g)) / n_basis;
}

static void cadence_each (Glyph *g, GtkProgressBar *progress)
{
  float left;
  float right;
  Cadence *c;
  c = glyph_get_cadence (g);

  left = n_width * c->left;
  right = n_width * c->right;

  {
    if (c->flags & LEFT_EXTREME)
      left -= left_extreme (g);
    else if (c->flags & LEFT_STEM)
      left -= left_most_center(g);
    else
      fprintf (stderr, "table problem\n");
  }

  {
    if (c->flags & RIGHT_EXTREME)
      right -= (g->ink_max_x - right_extreme (g));
    else if (c->flags & RIGHT_STEM)
      right -= (g->ink_max_x - right_most_center(g));
    else
      fprintf (stderr, "table problem\n");
  }

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
