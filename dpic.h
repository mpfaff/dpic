#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define distmax         3.40282347e+38	/*assumes at least IEEE single */
#define MaxReal         distmax
#define mdistmax        (-distmax)
#define pi              3.1415926535897931
#define ln10            2.3025850929940459

#define maxint          2147483647L
#define randmax         maxint

#define HASHLIM         9	/* Hash array upper val for var names */
#ifndef true
#define true    1
#define false   0
#endif

#include	"lxvars.h"
#include	"VERSIONDATE.h"

#define Free(p)	free(p)
                                /* Machine constants                  */
                                /* Assume simple ASCII; forget EBCDIC */
#define ordMINCH        0
#define ordMAXCH        127
#define ordNL           10
#define ordTAB          9
#define ordCR           13
#define ordETX          3
#define ordBSL          92

#define nlch            ((Char)ordNL)
#define tabch           ((Char)ordTAB)
#define crch            ((Char)ordCR)
#define etxch           ((Char)ordETX)
#define bslch           ((Char)ordBSL)

#define CHBUFSIZ        4095	/* upper limit of chbuf buffers       */

                                /* Lexical parameters                 */
#define FILENAMELEN     1024	/* max length of file names */

                                /* Lalr machine parameters            */
#define MAXERRCOUNT     3	    /* max no of errors before giving up  */

                                /* Draw types                         */
#define MFpic           1
#define MPost           2
#define PDF             3
#define PGF             4
#define Pict2e          5
#define PS              6
#define PSfrag          7
#define PSTricks        8
#define SVG             9
#define TeX             10
#define tTeX            11
#define xfig            12

#define SPLT            0.551784 /* optimum spline tension for arcs   */

#define pointd          72
#define DFONT           11      /* default svg font size, pt          */

#define Char char
typedef unsigned char uchar;
typedef unsigned char boolean;

                                /* Lexical types                      */
typedef short chbufinx;

typedef int symbol;

typedef short lxinx;

typedef uchar production;

typedef Char chbufarray[CHBUFSIZ + 1];
typedef Char mstring[FILENAMELEN];

                                /* Environment variable index         */
typedef uchar environx;

                                /* For storing names                  */
typedef struct nametype {
  double val;
  Char *segmnt;
  chbufinx seginx;
  int len;
  struct nametype *nextname;
} nametype;

                                /* Lexical input for loops and macros */
typedef struct fbuffer {
  Char *carray;
  int savedlen, readx, attrib;
  struct fbuffer *higherb, *prevb, *nextb;
} fbuffer;

                                /* Macro argument list pointer        */
typedef struct arg {
  fbuffer *argbody;
  struct arg *highera, *nexta;
} arg;

                                /* Pic position                       */
typedef struct postype {
  double xpos, ypos;
} postype;

                                /* Data structures for drawn primitives*/
#define PRIMbase \
  nametype *name, *textp, *outlinep, *shadedp; \
  struct primitive *parent, *son, *nextname; \
  postype aat; \
  double lparam, lthick; \
  int direction, spec, ptype

#define PRIMbox \
    struct { double boxheight, boxwidth, boxfill, boxradius; } boxstr

#define PRIMblock \
    struct { \
      double blockheight, blockwidth; \
      postype here; \
      nametype *(vars[HASHLIM + 1]); \
      int nvars[HASHLIM + 1]; \
      double *env; \
    } blockstr

#define PRIMcircle \
    struct { double cfill, cradius; } circlestr

#define PRIMellipse \
    struct { double elheight, elwidth, efill; } ellipsestr

#define PRIMline \
    struct { \
      postype endpos; \
      double height, width, lfill, aradius; \
      int atype; \
    } linestr

                                /* General and block primitives       */
typedef struct primitive {
  PRIMbase;
  union {
    PRIMbox;
    PRIMblock;
    PRIMcircle;
    PRIMellipse;
    PRIMline;
  } parms;
} primitive;

typedef struct XLboxprimitive {
  PRIMbase;
  union {
    PRIMbox;
  } parms;
} XLboxprimitive;

typedef struct XLcircleprimitive {
  PRIMbase;
  union {
    PRIMcircle;
  } parms;
} XLcircleprimitive;

typedef struct XLellipseprimitive {
  PRIMbase;
  union {
    PRIMellipse;
  } parms;
} XLellipseprimitive;

typedef struct XLlineprimitive {
  PRIMbase;
  union {
    PRIMline;
  } parms;
} XLlineprimitive;

typedef struct XLabelprimitive {
  PRIMbase;
} XLabelprimitive;

                                /* Abbreviations for readability.  These
                                   help somewhat but the code still retains
                                   p2c idioms (FORLIM, With, etc)      */
#define blockparms	parms.blockstr
#define boxparms	parms.boxstr
#define lineparms	parms.linestr
#define circleparms	parms.circlestr
#define ellipseparms	parms.ellipsestr

#define boxheight_	boxparms.boxheight
#define boxwidth_	boxparms.boxwidth
#define boxfill_	boxparms.boxfill
#define boxradius_	boxparms.boxradius

#define blockheight_ blockparms.blockheight
#define blockwidth_	blockparms.blockwidth
#define here_		blockparms.here

#define circlefill_	circleparms.cfill
#define circleradius_ circleparms.cradius

#define ellipseheight_ ellipseparms.elheight
#define ellipsewidth_  ellipseparms.elwidth
#define ellipsefill_   ellipseparms.efill

#define lineheight_	lineparms.height
#define endpos_		lineparms.endpos
#define linewidth_	lineparms.width
#define linefill_	lineparms.lfill
#define aradius_	lineparms.aradius
#define lineatype_	lineparms.atype
                                /* arcs use the line structure        */
#define startangle_	lineparms.endpos.xpos
#define arcangle_	lineparms.endpos.ypos

                                /* Parse stack production attributes  */
typedef struct attribute {
  chbufinx chbufx;
  int toklen;
  primitive *prim, *internal;
  nametype *varname;
  double xval, yval, startchop, endchop;
  int lexval, state;
} attribute;

                         /* EXTRN is null in main.c, extern elsewhere*/
EXTRN FILE *input, *output, *errout, *copyin, *redirect;

#ifdef DDEBUG
EXTRN FILE *log_;
EXTRN int oflag, debuglevel;   /* debug level and open logfile flag  */
EXTRN arg *currentmacro;       /* last-found macro                   */
#endif

EXTRN mstring infname;         /* name of current input file         */
EXTRN mstring outfnam;         /* name of current output file        */

EXTRN boolean inputeof;        /* end-of-input flag                  */
EXTRN boolean forbufend;       /* end of for buffer                  */
EXTRN int argct;               /* argument counter for options       */
EXTRN int drawmode;            /* output conversion                  */
EXTRN boolean safemode;        /* disable sh and copy                */

                               /* Lexical analyzer character buffer  */
EXTRN Char *chbuf;
EXTRN chbufinx chbufi, oldbufi; /* character buffer indices          */

                               /* Lexical variables                  */
EXTRN Char ch;                 /* current character                  */
EXTRN short newsymb;           /* current lexical symbol             */
EXTRN int lexsymb;             /* lexical value                      */
EXTRN int lexstate;            /* 0..4: <.PS; .PS; in pic; .PE; >.PE */
EXTRN boolean inlogic;         /* set < to <compare> in context      */
EXTRN boolean instr;           /* set while reading a string         */
EXTRN double floatvalue;       /* numerical value of float read      */
EXTRN fbuffer *inbuf, *savebuf, *freeinbuf, *topbuf;

EXTRN int oldsymb;             /* last lexical symbol                */
EXTRN arg *macros, *args, *freearg; /* lists of macros and args      */

                               /* Error handling                     */
EXTRN int errcount;            /* becomes nonzero when errors found  */
EXTRN int lineno;              /* current input line number          */
EXTRN int currprod;            /* current production for error msgs  */

                               /* Production variables               */
EXTRN attribute forattr;       /* to set up for loop                 */
EXTRN primitive *envblock;     /* block containing the current scope */
EXTRN primitive *tail;         /* for tree branches                  */
EXTRN primitive *globalenv;    /* the global environment block       */
EXTRN double dptextratio;      /* text parameters for SVG,PDF,PS     */
EXTRN double dpPPI;            /* pixels per inch                    */
EXTRN double xfigres, xdispres;/* xfig resolution and display res    */

EXTRN double north, south, east, west;
EXTRN double xfheight;         /* for calculating xfig and svg coords*/
EXTRN Char *freeseg;           /* segment open to store strings      */
EXTRN short freex;             /* next free location                 */
EXTRN Char *tmpbuf;            /* buffer for snprintf                */
EXTRN Char *tmpfmt;            /* snprintf, findvar buffer           */
EXTRN double scale, fsc;       /* scale factor and final scale factor*/
EXTRN int splcount, spltot;    /* spline depth counter               */
EXTRN int pdfobjcount;         /* pdf objects                        */
EXTRN primitive *snode;        /* temporary node storage             */
EXTRN boolean bfill;           /* fill flag for linear objects       */
EXTRN double vfill;            /* fill value */
EXTRN nametype *sshade, *soutline; /* temp values for linear objects */
EXTRN double lastfillval;      /* last-used fill density             */
EXTRN int printstate;          /* for passing output state info      */
                               /* graphics state parameters          */
EXTRN boolean gsocolor, gsfcolor, gsgcolor;
                               /* stroke, fill, gray fill flags      */
EXTRN double gslinethick;      /* last-used line thickness           */
EXTRN int gslinecap, gslinejoin; /* 0 = butt                         */
EXTRN double gsdashw, gsdashs; /* line dash and space lengths        */
EXTRN nametype *stream, *cx;   /* pdf stream storage and current seg */
EXTRN int pdfoffs[8];          /* pdf output byte counts             */

EXTRN attribute *pyylval;      /* production stack element           */
