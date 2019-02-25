#define _GNU_SOURCE
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "coc-table.h"

typedef enum {
	      UNITS_INCHES,
	      UNITS_FT,
	      UNITS_MM,
	      UNITS_CM,
	      UNITS_M,
} units_e;

static double
distance_conversion (double v, units_e u)
{
  double rv;

  switch (u) {
  case UNITS_INCHES:	rv = 25.4 * v; break;
  case UNITS_FT:	rv = 12.0 * 25.4 * v; break;
  case UNITS_MM:	rv =        v; break;
  case UNITS_CM:	rv = 10.0 * v; break;
  case UNITS_M:		rv = 1000.0 * v; break;
  }

  return rv;
}

static double
compute (double coc, double fl, double near, double far)
{
  double N;

  N = fl * fl / coc;

  if (isinf (far)) N /= 2.0 *  near;
  else N *= (far - near) / (2.0 * far * near);

  return N;
}

static void
show_help (const char *app)
{
  fprintf (stderr, "\nUsage: %s [opts] fl near far\n", app);
  fprintf (stderr, "\nwhere\n");
  fprintf (stderr, "\tfl is the focal length of the lens, in millimetres,\n");
  fprintf (stderr, "\tnear is the near distance of the depth of field,\n");
  fprintf (stderr, "\tfar is the far distance of the depth of field.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "\nopts:\n");
  fprintf (stderr, "\t-i, --inch  \tInterpret dimensionless near and far distances as inches.\n");
  fprintf (stderr, "\t-f, --feet  \t... feet.\n");
  fprintf (stderr, "\t-m, --mm    \t... millimetres.\n");
  fprintf (stderr, "\t-c, --cm    \t... centimetres.\n");
  fprintf (stderr, "\t-M, --metres\t... metres.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "\t\tThe default unit is centimetres.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "\t-o <arg>, --coc <arg>\tSet the \"circle of confusion\" value.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "\t\tThe default CoC is 0.019.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "\t--list-all\t\tList all supported cameras.\n");
  fprintf (stderr, "\t--list-maker <arg>\tList all supported cameras of the given \
manufacturer.\n");
  fprintf (stderr, "\t--list-model <arg>\tList all supported cameras of the given \
model.\n");
  fprintf (stderr, "\t\t\t\t--list-maker and --list-model can be used in conjunction.\n");
  fprintf (stderr, "\t-q, --query\tUsed with --list-maker and/or --list-model, allows\
the user to specify a camera for which the DOF is to be computed..\n");
  fprintf (stderr, "\t-V\t\tShow version and quit.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Examples:");
  fprintf (stderr, "\n\t$dof 55 66 71\n");
  fprintf (stderr, "\t[0.019](55 660 710) => 8.49395\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "The format of the result is:\n");
  fprintf (stderr, "\n\t[coc](fl nd fd) ==> A\n");
  fprintf (stderr, "\nwhere\n");
  fprintf (stderr, "\n\tCoc is the circle of confusion value of the camera in use,\n");
  fprintf (stderr, "\tfl is the focal length of the lens,\n");
  fprintf (stderr, "\tnd is the near distance, in millimetres, of the depth of field,\n");
  fprintf (stderr, "\tfd is the far distance, in millimetres, of the depth of field.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "The near and far distances can be appended with any of the units.\n");
  fprintf (stderr, "i, f, m, c, or M for inches, feet, millimetres, centimetres, or\n");
  fprintf (stderr, "metres respectively.  These override any units set by options:\n");
  fprintf (stderr, "\n\t$dof 55 26i 28i\n");
  fprintf (stderr, "\t[0.019](55 660.4 711.2) => 8.61007\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "The CoC value is a charcteristic of the various cameras and --list options \
can be used to select a camera:");
  fprintf (stderr, "\n");
  fprintf (stderr, "\n\t$dof -q --list-make Canon --list-model xs\n");
  fprintf (stderr, "\t  32: \"Canon\" \"Digital Rebel XS / 1000D\" 0.019\n");
  fprintf (stderr, "\t  35: \"Canon\" \"Digital Rebel XSi / 450D\" 0.019\n");
  fprintf (stderr, "\tEnter camera ID:\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "The Coc, whether supplied by the -o or --coc options or through\n");
  fprintf (stderr, "camera selection, is saved across invocations.\n");
}

int
main (int argc, char *argv[], char * env[])
{
  char *cfg_dir  = NULL;
  char *cfg_file = NULL;
  double coc = 0.019;

  asprintf (&cfg_dir, "%s/.config", getenv ("HOME"));
  if (cfg_dir) {
    struct stat statbuf;
    if (0 == stat (cfg_dir, &statbuf)) {
      if (S_ISDIR (statbuf.st_mode)) asprintf (&cfg_file, "%s/dof", cfg_dir);
    }
    free (cfg_dir);
  }
  if (cfg_file) {
    struct stat statbuf;
    if (0 == stat (cfg_file, &statbuf)) {
      if (S_ISREG (statbuf.st_mode)) {
	FILE *file = fopen (cfg_file, "r");
	if (file) {
	  double coct = 0.0;
	  if (1 == fscanf (file, " coc : %lg ", &coct)) coc = coct;
	  fclose (file);
	}
      }
    }
    free (cfg_dir);
  }

  extern char *optarg;
  int query   = 0;
  char *maker  = NULL;
  char *model = NULL;
  double parms[3] = {NAN, NAN, INFINITY};
#define fl   parms[0]
#define near parms[1]
#define far  parms[2]
  units_e near_units = UNITS_CM;
  units_e far_units  = UNITS_CM;
  {
    int err = 0;
    struct option long_opts[] =
      {
       {"inch",   no_argument, NULL, 'i'},
       {"feet",   no_argument, NULL, 'f'},
       {"mm",     no_argument, NULL, 'm'},
       {"cm",     no_argument, NULL, 'c'},
       {"metres", no_argument, NULL, 'M'},
       {"query",  no_argument, NULL, 'q'},
       {"coc",    required_argument, NULL, 'o'},
       {"list-all",   no_argument, NULL, 'L'},
       {"list-maker",  required_argument, NULL, 'C'},
       {"list-model", required_argument, NULL, 'T'},
       {0, 0, 0, 0}
      };
    int c;
    int idx;
    while (-1 != (c = getopt_long (argc, argv, "ifmcMo:qV", long_opts, &idx))) {
      switch (c) {
      case 'i': near_units = far_units = UNITS_INCHES;	break;
      case 'f': near_units = far_units = UNITS_FT;	break;
      case 'm': near_units = far_units = UNITS_MM;	break;
      case 'c': near_units = far_units = UNITS_CM;	break;
      case 'M': near_units = far_units = UNITS_M;	break;
      case 'o': coc = atof (optarg);	break;
      case 'q': query = 1; break;
      case 'L': list_all (NULL, NULL);    err = 2; break;
      case 'C': maker = strdupa (optarg); break;
      case 'T': model = strdupa (optarg); break;
      case 'V': fprintf (stderr, "Version %s\n", VERSION); err = 2; break;
      case '?': err = 1; break;
      }
    }
    if (query || maker || model) {
      list_all (maker, model);
      if (query) {
	fprintf (stdout, "Enter camera ID: ");
	fflush (stdout);
	int val = -1;
	int rc = fscanf (stdin, " %d", &val);
	if (rc > 0) {
	  coc = coc_table_coc (val);
	  if (coc < 0.0) err = 3;
	}
	else {
	  fprintf (stderr, "Invalid coc.\n");
	  exit (1);
	}
      }
    }
    
    switch (err) {
    case 1:
      show_help (argv[0]);
      // fall through
    case 2:
      exit (1);
      break;
    case 3:
      fprintf (stderr, "Invalid camera selection");
      exit (1);
      break;
    }
    if (err == 0) {
      int px;
      for (px = 0; optind < argc; px++) {
	if (px < 3) {
	  char *ep;
	  parms[px] = strtod (argv[optind], &ep);
	  units_e units = UNITS_MM;
	  if (ep && *ep != 0 && ep != argv[optind]) {
	    switch (*ep) {
	    case 'i': units = UNITS_INCHES;	break;
	    case 'f': units = UNITS_FT;		break;
	    case 'm': units = UNITS_MM;		break;
	    case 'c': units = UNITS_CM;		break;
	    case 'M': units = UNITS_M;		break;
	    }
	    switch(px) {
	    case 1: near_units = units; break;
	    case 2: far_units = units;  break;
	    }
	  }
	  optind++;
	}
	else break;
      }
      if (fl == NAN || near == NAN) err = 1;
    }
  }

  near = distance_conversion (near, near_units);
  far  = distance_conversion (far, far_units);
  fprintf (stdout, "[%g](%g %g %g) => %g\n",
	   coc, fl, near, far, compute (coc, fl, near, far));
  if (cfg_file) {
    FILE *file = fopen (cfg_file, "w");
    if (file) {
      fprintf (file, "coc: %g", coc);
      fclose (file);
    }
    free (cfg_file);
  }
  exit (0);
}
