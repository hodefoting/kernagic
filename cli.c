#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "kerner.h"

/* XXX: support creating a new ufo, instead of overwriting the old one..
 *      could be added by making an initial copy?
 */

static int interactive = 1;

gboolean kernagic_strip_left_bearing = KERNAGIC_DEFAULT_STRIP_LEFT_BEARING;
char *kernagic_output = NULL;
char *kernagic_sample_string = NULL;
char *kernagic_output_png = NULL;

void help (void)
{
  printf ("kernagic [options] <font.ufo>\n"
          "\n"
          "Options:\n"
//          "   -c use cadence units, try this for fonts with traditional proportions\n"
//          "   -g use gray level     (default)\n"
//          "      suboptions influencing gray:\n"
          "       -m <0..100>   minimum distance default = %i\n"
          "       -M <0..100>   maximum distance default = %i\n"
          "       -t <0..100>   target gray value default = %i\n"
          "       -l            don't strip left bearing\n"
          "       -L            strip left bearing (default)\n"
          "\n"
          "   -o <output.ufo>  create a copy of the input font, withoutit kernagic overwrites the input ufo\n"
   /*     "   -s <string>      sample string (for use with -p)\n"
          "   -p <output.png>   render sample string with settings (do not change font)\n" */
          "\n"

          "Examples:\n"
    //      "    Use the cadence unit method on Test.ufo, overwriting file\n"
    //      "      kernagic -c Test.ufo\n"
   /*     "    Preview default settings on string shoplift to test.png\n"
          "      kernagic Original.ufo -s 'shoplift' -p test.png\n"
          "    Run gray with a different gray target, still previewing to test.png\n"
          "      kernagic Original.ufo -s 'shoplift' -p test.png -t 0.4\n"
          "    Write a final adjusted font to Output.ufo\n" */
          "      kernagic Original.ufo -s 'shoplift' -o Output.ufo -t 0.32 -m 10 -M 30\n",
    KERNER_DEFAULT_MIN,
    KERNER_DEFAULT_MAX,
    KERNER_DEFAULT_TARGET_GRAY
          );
  exit (0);
}

const char *ufo_path = NULL;

void parse_args (int argc, char **argv)
{
  int no;
  for (no = 1; no < argc; no++)
    {
      if (!strcmp (argv[no], "--help") ||
          !strcmp (argv[no], "-h"))
        help ();
      else if (!strcmp (argv[no], "-m"))
        {
#define EXPECT_ARG if (!argv[no+1]) {fprintf (stderr, "expected argument after %s\n", argv[no]);exit(-1);}

          EXPECT_ARG;
          kerner_settings.minimum_distance = atof (argv[++no]);
        }
      else if (!strcmp (argv[no], "-M"))
        {
          EXPECT_ARG;
          kerner_settings.maximum_distance = atof (argv[++no]);
        }
      else if (!strcmp (argv[no], "-t"))
        {
          EXPECT_ARG;
          kerner_settings.alpha_target = atof (argv[++no]);
        }
      else if (!strcmp (argv[no], "-g"))
        {
          kerner_settings.mode = 0;
        }
      else if (!strcmp (argv[no], "-c"))
        {
          kerner_settings.mode = 1;
        }
      else if (!strcmp (argv[no], "-l"))
        {
          kernagic_strip_left_bearing = 0;
        }
      else if (!strcmp (argv[no], "-L"))
        {
          kernagic_strip_left_bearing = 1;
        }
      else if (!strcmp (argv[no], "-o"))
      {
        EXPECT_ARG;
        kernagic_output = argv[++no];
        interactive = 0;

        if (!ufo_path)
          {
            fprintf (stderr, "no font file to work on specified before -o\n");
            exit (-1);
          }

        char cmd[512];
        /* XXX: does not work on windows */
        sprintf (cmd, "rm -rf %s;cp -Rva %s %s", kernagic_output, ufo_path, kernagic_output);
        fprintf (stderr, "%s\n", cmd);
        system (cmd);
        ufo_path = kernagic_output;
      }
      else if (!strcmp (argv[no], "-p"))
      {
        EXPECT_ARG;
        kernagic_output_png = argv[++no];
        interactive = 0;
      }
      else if (!strcmp (argv[no], "-s"))
      {
        EXPECT_ARG;
        kernagic_sample_string = argv[++no];
      }
      else if (argv[no][0] == '-')
        {
          fprintf (stderr, "unknown argument %s\n", argv[no]);
          exit (-1);
        }
      else
        {
          ufo_path = argv[no];
        }
    }
}

int kernagic_gtk (int argc, char **argv);

int main (int argc, char **argv)
{
  parse_args (argc, argv);

  if (interactive)
    return kernagic_gtk (argc, argv);

  if (!ufo_path)
    {
      fprintf (stderr, "no font file to work on specified\n");
      exit (-1);
    }

  kernagic_load_ufo (ufo_path, kernagic_strip_left_bearing);

  kernagic_set_glyph_string (NULL);
  fprintf (stderr, "computing!\n");
  kernagic_compute (NULL);
  fprintf (stderr, "done fitting!\n");
  fprintf (stderr, "saving\n");
  kernagic_save_kerning_info ();
  fprintf (stderr, "done saving!\n");

  return 0;
}
