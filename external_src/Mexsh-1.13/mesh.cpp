/* $Id: mesh.cpp,v 1.65 2004/10/25 11:30:18 aspert Exp $ */


/*
 *
 *  Copyright (C) 2001-2004 EPFL (Swiss Federal Institute of Technology,
 *  Lausanne) This program is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *  USA.
 *
 *  In addition, as a special exception, EPFL gives permission to link
 *  the code of this program with the Qt non-commercial edition library
 *  (or with modified versions of Qt non-commercial edition that use the
 *  same license as Qt non-commercial edition), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt non-commercial edition.  If you modify this file, you may extend
 *  this exception to your version of the file, but you are not
 *  obligated to do so.  If you do not wish to do so, delete this
 *  exception statement from your version.
 *
 *  Authors : Nicolas Aspert, Diego Santa-Cruz and Davy Jacquet
 *
 *  Web site : http://mesh.epfl.ch
 *
 *  Reference :
 *   "MESH : Measuring Errors between Surfaces using the Hausdorff distance"
 *   in Proceedings of IEEE Intl. Conf. on Multimedia and Expo (ICME) 2002, 
 *   vol. I, pp. 705-708, available on http://mesh.epfl.ch
 *
 */







#include <time.h>
#include <string.h>
#include <qapplication.h>
#include <qprogressdialog.h>
#include <qpixmap.h>
#include <mesh.h>
#include <ScreenWidget.h>
#include <InitWidget.h>
#include <mesh_run.h>
#include <3dmodel.h>
#include <mex.h>

#ifndef _MESHICON_XPM
# define _MESHICON_XPM
# include <meshIcon.xpm>
#endif

/* The version string, also parsed by Makefile */
const char *version = "1.13";

/* The Copyright */
const char *copyright = "2001-2004 EPFL";

static void print_version(FILE *out) {
  fprintf(out,"MESH: Measuring Error between Surfaces using the "
          "Hausdorff distance\n");
  fprintf(out,"MESH version %s\n",version);
  fprintf(out,"\n");
  fprintf(out,"Copyright (C) %s\n",copyright);
  fprintf(out,
          "This is free software; see the source for copying conditions.\n");
  fprintf(out,
          "There is NO warranty; not even for MERCHANTABILITY or FITNESS "); 
  fprintf(out,"FOR A\nPARTICULAR PURPOSE.\n\n");
  fprintf(out,"Report bugs to <Nicolas.Aspert@epfl.ch> and ");
  fprintf(out,"<Diego.SantaCruz@epfl.ch>\n");
  fprintf(out,"\n");
}

/* Prints usage information to the out stream */
static void print_usage(FILE *out)
{
  print_version(out);
  fprintf(out,"Usage: mesh [[options] file1 file2]\n");
  fprintf(out,"\n");
  fprintf(out,"The program measures the distance from the 3D model in\n");
  fprintf(out,"file1 to the one in file2. The models must be given as\n");
  fprintf(out,"triangular meshes in RAW, VRML2, Inventor 2.x, PLY ASCII,\n");
  fprintf(out,"PLY binary, and SMF formats. Normals are supported in RAW\n");
  fprintf(out,"and VRML2 formats. It also reads gzipped files directly.\n");
  fprintf(out,"The VRML parser reads IndexedFaceSets nodes only,\n");
  fprintf(out,"ignoring all transformations, and does not support USE tags\n");
  fprintf(out,"(DEF tags are ignored). Likewise the Inventor 2.x reader is\n");
  fprintf(out,"somewhat limited. The file type is autodetected.\n");
  fprintf(out,"After the distance is calculated the result is displayed\n");
  fprintf(out,"as overall measures in text form and as a detailed distance\n");
  fprintf(out,"map in graphical form.\n");
  fprintf(out,"If no options nor filenames are given a dialog is shown\n");
  fprintf(out,"to select the input file names as well as the parameters.\n");
  fprintf(out,"\n");
  fprintf(out,"Display:\n");
  fprintf(out,"After calculating the distance, the error distribution on\n");
  fprintf(out,"model 1 is displayed in graphical form. There are three\n");
  fprintf(out,"modes to display the error distribution: vertex error, face\n");
  fprintf(out,"mean error and sample error.\n");
  fprintf(out,"In vertex error, each vertex is assigned a color based on\n");
  fprintf(out,"the calculated error at that vertex (if not calculated at\n");
  fprintf(out,"that vertex, dark gray is used) and the color is\n");
  fprintf(out,"interpolated between vertices. Invisible vertices are added\n");
  fprintf(out,
          "as necessary to display error at sample points which are not\n");
  fprintf(out,"vertices of model 1. The ratio between the total number of\n");
  fprintf(out,"error samples and error samples displayed is controlled by\n");
  fprintf(out,"the \"vertex error downsampling\" parameter. For low\n");
  fprintf(out,"downsampling values it can be very accurate, but it can use\n");
  fprintf(out,"a large number of triangles. For high downsampling values\n");
  fprintf(out,"the result can be misleading, although very fast.\n");
  fprintf(out,"In mean face error, each triangle is assigned a color based\n");
  fprintf(out,
          "on the mean error for that face (if that face has no samples\n");
  fprintf(out,"a dark gray is used).\n");
  fprintf(out,"In sample error mode  a color is assigned to each sample\n");
  fprintf(out,"point based on the error at that point, and  applied to the\n");
  fprintf(out,"model using texture mapping (if a face has no sample points\n");
  fprintf(out,"a dark gray is assigned to it). This mode is as accurate as\n");
  fprintf(out,"the vertex error mode with a downsampling value of 1,\n");
  fprintf(out,"although it can be very slow if model 1 has a large number\n");
  fprintf(out,"of triangles. If model 1 has few triangles and there is a\n");
  fprintf(out,
          "large number of samples this mode can be considerably faster\n");
  fprintf(out,"than vertex error mode with a low downsampling value.\n");
  fprintf(out,"A colorbar showing the correspondance between error values\n");
  fprintf(out,"and color is displayed. The colorbar also shows the\n");
  fprintf(out,"approximate distribution, on the surface of model 1, of the\n");
  fprintf(out,"error values using a histogram (approximate because the\n");
  fprintf(out,"distribution of the sample points on the surface is not\n");
  fprintf(out,"truly uniform).\n");
  fprintf(out,"Note that the -mf flag can force a minimum number of samples\n");
  fprintf(out,"per face.\n");
  fprintf(out,"\n");
  fprintf(out,"options:");
  fprintf(out,"\n");
  fprintf(out,"  -h\tDisplays this help message and exits.\n");
  fprintf(out,"\n");
  fprintf(out,"  -v\tDisplays version number, legal info and exits.\n");
  fprintf(out,"\n");
  fprintf(out,"  -s\tCalculate a symmetric distance measure. It calculates\n");
  fprintf(out,"    \tthe distance in the two directions and uses the max\n");
  fprintf(out,"    \tas the symmetric distance (Hausdorff distance).\n");
  fprintf(out,"\n");
  fprintf(out,"  -q\tQuiet, do not print progress meter.\n");
  fprintf(out,"\n");
  fprintf(out,"  -va\tBe verbose about non-manifold vertices during the\n");
  fprintf(out,"     \tmodel analysis phase.\n");
  fprintf(out,"\n");
  fprintf(out,"  -t\tDisplay only textual results, do not display the GUI.\n");
  fprintf(out,"\n");
  fprintf(out,
          "  -l s\tSet the sampling step to s, which is a percentage of\n");
  fprintf(out,"      \tthe bounding box diagonal of the second model. The\n");
  fprintf(out,"      \ttriangles of the first model are sampled, in order\n");
  fprintf(out,"      \tto calculate an approximation of the distance, so\n");
  fprintf(out,"      \tthat the sampling density (number of samples per\n");
  fprintf(out,
          "      \tunit surface) is 1/(s^2) (i.e. one sample per square\n");
  fprintf(out,"      \tof side length s). A probabilistic model is used so\n");
  fprintf(out,
          "      \tthat the resulting number is as close as possible to\n");
  fprintf(out,"      \tthe target. The default is 0.5\n\n");
  fprintf(out,"  -mf f\tEnsure that each triangle has a sampling frequency\n");
  fprintf(out,"       \tof at least f. Normally the sampling frequency for\n");
  fprintf(out,"       \ta triangle is determined by the sampling step and\n");
  fprintf(out,"       \ttriangle size. For some combinations this can lead\n");
  fprintf(out,"       \tto some triangles having no or few samples. With a\n");
  fprintf(out,"       \tnon-zero f parameter this can be avoided, although\n");
  fprintf(out,"       \tit disturbs the uniform distribution of samples.\n");
  fprintf(out,"       \tWith f set to 1, all triangles get at least one\n");
  fprintf(out,
          "       \tsample. With f set to 2, all triangles get at least\n");
  fprintf(out,"       \tthree samples, and thus all vertices get a sample.\n");
  fprintf(out,"       \tHigher values of f are less useful. By default it\n");
  fprintf(out,"       \tis zero in non-GUI mode and two in GUI mode.\n\n");
  fprintf(out,"  -wlog\tDisplay textual results in a window instead of on\n");
  fprintf(out,
          "       \tstandard output. Not compatible with the -t option.\n\n");
  fprintf(out,"  -tex\tEnables the display of the error by texture mapping\n");
  fprintf(out,"      \ton the model. \n");
  fprintf(out,"      \tWARNING : handle with care. Some platforms will \n");
  fprintf(out,"      \tfail to display this, and using this on big models\n");
  fprintf(out,"      \twill probably crash your computer with a\n");
  fprintf(out,"      \tswap storm. Not compatible with the -t option.\n");
  fprintf(out,"\n");
}

/* Initializes *pargs to default values and parses the command line arguments
 * in argc,argv. */ 


static void parse_args(char *fname1, char *fname2, struct args *pargs)
{
 char *endptr;
 int i;

 memset(pargs,0,sizeof(*pargs));
 pargs->sampling_step = 0.5;
 pargs->min_sample_freq = -1;
 i = 1;
 pargs->do_symmetric = 1;
 pargs->m1_fname = fname1;
 pargs->m2_fname = fname2;
 pargs->no_gui = 1;
 pargs->sampling_step /= 100; /* convert percent to fraction */
}

/*****************************************************************************/
/*            'main' starts here.....                                        */
/*****************************************************************************/


void mexFunction(
    int nlhs, mxArray *plhs[],
    int nrhs, const mxArray *prhs[])
{
//int main( int argc, char **argv )
//{
  int i;
  QString m1,n1,o1;
  struct args pargs;
  QApplication *a;
  InitWidget *b;
  ScreenWidget *c; 
  TextWidget *textOut;
  QProgressDialog *qProg;
  QPixmap *qpxMeshIcon=NULL;
  struct model_error model1,model2;
  int rcode;
  struct outbuf *log;
  struct prog_reporter pr;
  double output;
  double *y;
  
  /* Initialize application */
  a = NULL;
  b = NULL;
  c = NULL;
  qProg = NULL;
  memset(&model1,0,sizeof(model1));
  memset(&model2,0,sizeof(model2));
  memset(&pr,0,sizeof(pr));
  log = NULL;
  i = 0;
  
  if (nrhs != 2) {
      mexErrMsgTxt("Two input arguments required.");
  } else if (nlhs > 1) {
      mexErrMsgTxt("Too many output arguments.");
  }
    
  a = NULL; /* No QT app needed */  

  pr.prog = stdio_prog;
  pr.cb_out = stdout;
       
  char *fname1, *fname2;
  size_t buflen1, buflen2;
  /* input must be a string */
  if ( mxIsChar(prhs[0]) != 1  || mxIsChar(prhs[0]) != 1)
      mexErrMsgIdAndTxt( "MATLAB:revord:inputNotString",
              "Input must be a string.");
  
  /* input must be a row vector */
  if (mxGetM(prhs[0])!=1 || mxGetM(prhs[1])!=1)
      mexErrMsgIdAndTxt( "MATLAB:revord:inputNotVector",
              "Input must be a row vector.");
  
  /* get the length of the input string */
  buflen1 = (mxGetM(prhs[0]) * mxGetN(prhs[0])) + 1;  
  buflen2 = (mxGetM(prhs[1]) * mxGetN(prhs[1])) + 1;

  fname1 = mxArrayToString(prhs[0]);
  fname2 = mxArrayToString(prhs[1]);
   
  plhs[0] = mxCreateDoubleMatrix(1,1, mxREAL);
  y = mxGetPr(plhs[0]);  
  y[0] = 1;

  parse_args((char *) fname1, (char *) fname2, &pargs);
  
  output = mesh_run(&pargs, &model1, &model2, log, &pr);
  y[0] = output;
  
  /* Free widgets */
  outbuf_delete(log);
  delete qProg;
  delete qpxMeshIcon;
  delete b;
  delete c;
  delete a; // QApplication must be last QT thing to delete
  /* Free model data */
  if (model1.mesh != NULL) __free_raw_model(model1.mesh);
  free(model1.verror);
  free(model1.info);
  free_face_error(model1.fe);
  if (model2.mesh != NULL) __free_raw_model(model2.mesh);
  free(model2.verror);
  free(model2.info);
    
  mxFree(fname1);
  mxFree(fname2);  
  /* Return exit code */
}
