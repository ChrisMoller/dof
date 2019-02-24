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
  
  int query   = 0;
  char *make  = NULL;
  char *model = NULL;
  double parms[3] = {NAN, NAN, INFINITY};
#define fl   parms[0]
#define near parms[1]
#define far  parms[2]
  units_e near_units = UNITS_INCHES;
  units_e far_units  = UNITS_INCHES;
  {
    int err = 0;
    struct option long_opts[] =
      {
       {"feet",  no_argument, NULL, 'f'},
       {"inch",  no_argument, NULL, 'i'},
       {"mm",    no_argument, NULL, 'm'},
       {"cm",    no_argument, NULL, 'c'},
       {"m",     no_argument, NULL, 'M'},
       {"q",     no_argument, NULL, 'q'},
       {"coc",   required_argument, NULL, 'o'},
       {"list-all",   no_argument, NULL, 'L'},
       {"list-make",  required_argument, NULL, 'C'},
       {"list-model", required_argument, NULL, 'T'},
       {0, 0, 0, 0}
      };
    int c;
    int idx;
    while (-1 != (c = getopt_long (argc, argv, "ifmcMq", long_opts, &idx))) {
      switch (c) {
      case 'i': near_units = far_units = UNITS_INCHES;	break;
      case 'f': near_units = far_units = UNITS_FT;	break;
      case 'm': near_units = far_units = UNITS_MM;	break;
      case 'c': near_units = far_units = UNITS_CM;	break;
      case 'M': near_units = far_units = UNITS_M;	break;
      case 'o': coc = atof (optarg);	break;
      case 'q': query = 1l; break;
      case 'L': list_all (NULL, NULL);    err = 2; break;
      case 'C': if (make)  free (make);  make  = strdup (optarg); break;
      case 'T': if (model) free (model); model = strdup (optarg); break;
      case '?': err = 1; break;
      }
    }
    if (make || model) {
      list_all (make, model);
      if (query) {
	fprintf (stdout, "Enter camer ID: ");
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
      fprintf (stderr, "error: %s [opts] fl near far\n", argv[0]);
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
	    case 'f': units = UNITS_FT;	break;
	    case 'm': units = UNITS_MM;	break;
	    case 'c': units = UNITS_CM;	break;
	    case 'M': units = UNITS_M;	break;
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