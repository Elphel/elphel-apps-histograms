/*!
** -----------------------------------------------------------------------------**
** pnghist.c
**
** Copyright (C) 2006-2016 Elphel, Inc.
**
** -----------------------------------------------------------------------------**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
** -----------------------------------------------------------------------------**
*/

#include <string.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

//#include <elphel/autoexp.h>
// http://stackoverflow.com/questions/2442335/libpng-boostgil-png-infopp-null-not-found
// libpng 1.4 dropped definitions of png_infopp_NULL and int_p_NULL. So add
#define png_infopp_NULL (png_infopp)NULL
#define int_p_NULL (int*)NULL

#include <sys/mman.h>
#include <elphel/c313a.h>
#include <elphel/x393_devices.h>

 
//#include "zlib.h"
//#include <libpng16/png.h>
#include <png.h>
#define HIST_SIZE 256 * 4 * 4

#define HPNG_PLOT_BARS    1
#define HPNG_CONNECT_DOTS 2
#define HPNG_FILL_ZEROS   4
#define HPNG_LIN_INTERP   8
#define HPNG_RUN_SQRT     16

#define QRY_MAXPARAMS 64

#define THIS_DEBUG 0
// lighttpd requires the following setting to enable logging of the cgi program errors:
// ## where cgi stderr output is redirected
// server.breakagelog          = "/www/logs/lighttpd_stderr.log"


char  copyQuery [4096];
const char *uri;
const char *method;
const char *query;

struct key_value {
        char *key;
        char *value;
};

struct key_value gparams[QRY_MAXPARAMS+1];

int unescape(char *, int);
int hexdigit(char);
char * paramValue(struct key_value *, char *);
int parseQuery(struct key_value *, char *);


struct autoexp_t autoexp;
int write_png(int height, int data[1024], int mode, int colors);
int main(int argc, char *argv[])
{
  int sensor_port=0;
  int subchannel = 0;
  int total_hist_entries; // =8*4 for 4 sensors
  int fd_histogram_cache;
  struct histogram_stuct_t  * histogram_cache; /// array of histogram
  const char histogram_driver_name[]=DEV393_PATH(DEV393_HISTOGRAM);
//#define DEV393_HISTOGRAM      ("histogram_cache","histograms_operations",138,18,"0666","c") ///< Access to acquired/calculated histograms for all ports/channels/colors

  int hists[1536];
  int rava[256];
  int i,j,k,a,l;
  int png_height=256;
  double dscale=3.0;
  int mode=14;
  int colors=57; /// colors 1 - r, 2 - g1, 4 - g2, 8 - b, 16 - w, 32 - g1+g2 
  int rav=0; // running average
  int request_enable=1; /// enable requesting histogram calculation for the specified frame (0 - use/wait what available)
  double dcoeff;
  int coeff;
  int lastmiddle_i,newmiddle_i;
  int hist_index;
  int before = 1; // by default - previous frame
  const char colors_calc[]={  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,0xa,0xb,0xc,0xd,0xe,0xf,
                      0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,
                        6,  7,  6,  7,  6,  7,  6,  7,0xe,0xf,0xe,0xf,0xe,0xf,0xe,0xf,
                      0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf,0xf};
  char * v;                    
#if 0                      
  if (argc<5) {printf("pnghist height scale mode colors average"); return 0;}
 
  rav=atoi(argv[5]);
  png_height=atoi(argv[1]);
  if (png_height>256) png_height=256;
  dscale=atof(argv[2]);
  mode=atoi(argv[3]);
  colors=atoi(argv[4]) & 0x3f;
#endif 

  method = getenv("REQUEST_METHOD");
  query = getenv("QUERY_STRING");
  uri = getenv("REQUEST_URI");

  i= (query)? strlen (query):0;
  if (i>(sizeof(copyQuery)-1)) i= sizeof(copyQuery)-1;
  if (i>0) strncpy(copyQuery,query, i);
  copyQuery[i]=0;
  unescape(copyQuery,sizeof(copyQuery));

  if (method == NULL)
  {
  /* Not a CGI! */
     printf("This program should be run as a CGI from the web server!\n");
     exit(1);
  }

  parseQuery(gparams, copyQuery);
  #define HPNG_PLOT_BARS    1
//#define HPNG_CONNECT_DOTS 2
//#define HPNG_FILL_ZEROS   4
//#define HPNG_LIN_INTERP   8
//#define HPNG_RUN_SQRT     16
/// Sample from camvc.html
///http://192.168.0.9/pnghist.cgi?sqrt=1&scale=5&average=5&height=128&fillz=1&linterpz=0&draw=2&colors=41
///FIXME: Some bump on low red - what is it?
//sensor_port
  if((v = paramValue(gparams, "sensor_port")) != NULL) sensor_port=strtol(v, NULL, 10);
  if((v = paramValue(gparams, "subchannel")) != NULL)  subchannel=strtol(v, NULL, 10);
  if((v = paramValue(gparams, "height"))      != NULL) png_height=strtol(v, NULL, 10);
  if((v = paramValue(gparams, "draw"))        != NULL) mode= (mode & 0xfc) | (strtol(v, NULL, 10) & 3);
  if((v = paramValue(gparams, "fillz"))       != NULL) mode= (mode & (~HPNG_FILL_ZEROS)) | (strtol(v, NULL, 10)?HPNG_FILL_ZEROS:0);
  if((v = paramValue(gparams, "linterpz"))    != NULL) mode= (mode & (~HPNG_LIN_INTERP)) | (strtol(v, NULL, 10)?HPNG_LIN_INTERP:0);
  if((v = paramValue(gparams, "sqrt"))        != NULL) mode= (mode & (~HPNG_RUN_SQRT))   | (strtol(v, NULL, 10)?HPNG_RUN_SQRT:0);
  if((v = paramValue(gparams, "colors"))      != NULL) colors=strtol(v, NULL, 10);
  if((v = paramValue(gparams, "average"))     != NULL) rav=strtol(v, NULL, 10);
  if((v = paramValue(gparams, "scale"))       != NULL) dscale=strtod(v,NULL);
  if((v = paramValue(gparams, "disrq"))       != NULL) request_enable=strtol(v, NULL, 10)?0:1;
  if((v = paramValue(gparams, "before"))      != NULL) before=strtol(v, NULL, 10);
//  before
//  int request_enable=1; /// enable requesting histogram calculation for the specified frame (0 - use/wait what available)

//  const char histogram_driver_name[]="/dev/histogram_cache"

  fd_histogram_cache= open(histogram_driver_name, O_RDONLY); /// instead of autoexp_fd 
  if (fd_histogram_cache <0) {
     fprintf(stdout, "Pragma: no-cache\n");
     fprintf(stdout, "Content-Type: text/plain\n\n");
     fprintf(stdout, "Device busy (%s)\r\n", histogram_driver_name);
     fflush(stdout);
     return -1;
  }
//  Select sensor port and subchannel
  if ((sensor_port< 0) || (sensor_port > 3) || (subchannel < 0) || (subchannel > 3)){
     fprintf(stdout, "Pragma: no-cache\n");
     fprintf(stdout, "Content-Type: text/plain\n\n");
     fprintf(stdout, "Wrong sensor port and/or subchannel number: %s\n", histogram_driver_name);
     fflush(stdout);
     return -1;
  }
  if (((total_hist_entries = lseek(fd_histogram_cache, LSEEK_HIST_SET_CHN + (sensor_port <<2) +subchannel, SEEK_END)))<0){ // set port/subchannel to use
      fprintf(stdout, "Pragma: no-cache\n");
      fprintf(stdout, "Content-Type: text/plain\n\n");
      fprintf(stdout, "port/subchannel combination does not exist: %s\n", histogram_driver_name);
      fflush(stdout);
      return -1;
  }
#ifdef THIS_DEBUG
   fprintf (stderr,"LSEEK_HIST_SET_CHN port = %d chn = %d - > 0x%x\n",sensor_port,subchannel, total_hist_entries);
   fflush(stderr);
#endif
   // now try to mmap
     histogram_cache = (struct histogram_stuct_t *) mmap(0, sizeof (struct histogram_stuct_t) * total_hist_entries , PROT_READ, MAP_SHARED, fd_histogram_cache, 0);
     if((int)histogram_cache == -1) {
        fprintf(stdout, "Pragma: no-cache\n");
        fprintf(stdout, "Content-Type: text/plain\n\n");
        fprintf(stdout, "problems with mmap: %s\n", histogram_driver_name);
        fflush(stdout);
        return -1;
     }

#if 0
  if ((offset & ~0xf) == LSEEK_HIST_SET_CHN){
      p = (offset >> 2)  & 3;
      s = (offset >> 0)  & 3;
      if (get_hist_index(p,s)<0)
          return -ENXIO; // invalid port/channel combination
      privData->port =       p;
      privData->subchannel = s;
      if((v = paramValue(gparams, "sensor_port")) != NULL) sensor_port=strtol(v, NULL, 10);
      if((v = paramValue(gparams, "subchannel")) != NULL)  subchannel=strtol(v, NULL, 10);


#endif
/// Now find the index of the most recently available histogram (maybe will wait a little if it is not yet available for the current frame - use previous?)
  lseek(fd_histogram_cache, LSEEK_HIST_WAIT_C, SEEK_END);          /// wait for all histograms, not just Y (G1)
  lseek(fd_histogram_cache, LSEEK_HIST_NEEDED + 0xf0, SEEK_END);    /// forward and cumulative histograms are needed
  if (request_enable) lseek(fd_histogram_cache, LSEEK_HIST_REQ_EN, SEEK_END);
  else                lseek(fd_histogram_cache, LSEEK_HIST_REQ_DIS, SEEK_END); /// disable requesting histogram for the specified frame, rely on the available ones
  hist_index=lseek(fd_histogram_cache, -before, SEEK_CUR);                     /// request histograms for the previous frame
  if(hist_index <0) {
     fprintf(stdout, "Pragma: no-cache\n");
     fprintf(stdout, "Content-Type: text/plain\n\n");
     fprintf(stdout, "Requested histograms are not available\n");
     fflush(stdout);
     return -1;
  }

#ifdef THIS_DEBUG
   fprintf (stderr,"hist_index =  0x%x\n",hist_index);
   fflush(stderr);
#endif

  /// Ignore missing histograms here
//  ww=(ww*autoexp.width)/100;
//  wh=(wh*autoexp.height)/100;
//  dcoeff=(png_height*1024)/(dscale*ww*wh);
  dcoeff=(png_height*256)/(dscale* histogram_cache[hist_index].cumul_hist_g[255]); /// cumul_hist_g[255] - total number of pixels for color G1
  memcpy (hists,histogram_cache[hist_index].hist,HIST_SIZE);
  close (fd_histogram_cache);
  coeff=dcoeff*65536;
#ifdef THIS_DEBUG
   for (i=0;i<1024;i++) {
     if ((i & 0x0ff)==0) fprintf (stderr,"\n");
     if ((i &  0x0f)==0) fprintf (stderr,"\n0x%03x:",i);
     fprintf (stderr," %05x",hists[i]);
   }
   fprintf (stderr,"\n\n\n");
   fflush(stderr);
#endif

  if ((mode & (HPNG_FILL_ZEROS | HPNG_LIN_INTERP)) | (rav>1)) { // spread between zeros - needed at high digital gains (near black with low gammas)
    for (j=0;j<4;j++)  if (colors_calc[colors] & (1<<j)) {
      lastmiddle_i=255;
      i=255;
      while ((hists[i+(j<<8)]==0) && (i>0)) i--;// skip high zeros
      while (i>0) {
       k=i;
       i--;
       while ((i>=0) && (hists[i+(j<<8)]==0)) i--;
       if ((k-i)>1) {
         a=hists[k+(j<<8)]/(k-i);
         for (l=k;l>i;l--) hists[l+(j<<8)] = a;
       }
       if ((mode & HPNG_LIN_INTERP) | (rav>1)) { ///linear interpolation - also for high digital gains (near black with low gammas)
         newmiddle_i= (k+i+1)/2;
         for (l=lastmiddle_i-1;l>newmiddle_i;l--) { 
            hists[l+(j<<8)]= hists[newmiddle_i+(j<<8)] + ((l-newmiddle_i)* (hists[lastmiddle_i+(j<<8)]-hists[newmiddle_i+(j<<8)]))/(lastmiddle_i-newmiddle_i);
         }   
         lastmiddle_i=newmiddle_i;
       }
      }
/// apply running average (if any)
      if (rav>0) {
        k=0;
        l=0;
        for (i=0;i<256;i++) {
         rava[i]=hists[i+(j<<8)];
         k+=rava[i];
         if ((i-rav)>=0) k-=rava[i-rav];
         else l++;
         hists[i+(j<<8)]=k/l;
        }
      }
    }
  }
/// calculate white if needed
  if (colors & 0x10) for (i=0;i<256;i++) hists[i+1024]=(hists[i]+hists[i+256]+hists[i+512]+hists[i+768])>>2;
/// calculate green if needed
  if (colors & 0x20) for (i=0;i<256;i++) hists[i+1280]=(hists[i+256]+hists[i+512])>>1;
  
  if (mode & HPNG_RUN_SQRT) {
    dcoeff=sqrt((32768.0*png_height)/coeff);
    for (j=0;j<6;j++) if (colors & (1 << j)) for (i=0;i<256;i++) hists[i+(j<<8)]=sqrt(hists[i+(j<<8)])*dcoeff;
  }
/// now scale data
  for (j=0;j<6;j++) if (colors & (1 << j)) for (i=0;i<256;i++) {
   hists[i+(j<<8)]=(hists[i+(j<<8)]*coeff) >> 16;
   if (hists[i+(j<<8)] > (png_height-1)) hists[i+(j<<8)] = png_height-1;
  }
   fprintf(stdout, "Pragma: no-cache\n");
   fprintf(stdout, "Content-Type: image/png\n\n");
#ifdef THIS_DEBUG
//   for (i=0;i<1536;i++) {
   for (i=0;i<1024;i++) {
     if ((i & 0x0ff)==0) fprintf (stderr,"\n");
     if ((i &  0x0f)==0) fprintf (stderr,"\n0x%03x:",i);
     fprintf (stderr," %05x",hists[i]);
   }
   fprintf (stderr,"\n\n\n");
#endif
   write_png(png_height, hists, mode, colors);
   fflush(stdout);
  return 0;
}
#define ERROR -1
#define OK 0
//#define HPNG_PLOT_BARS    1
//#define HPNG_CONNECT_DOTS 2

int write_png(int png_height, int data[1536], int mode, int colors)
{
//   FILE *fp;
   png_structp png_ptr;
   png_infop info_ptr;
//   const char clrs[]={1,2,2,4,15,2}; // dim white
   const char clrs[]={1,2,2,4,7,2}; // bright white
//   png_colorp palette;
   unsigned char cpalette[]= {0,   0,   0,
                        0xff,0,   0,
                        0,   0xff,0,
                        0xff,0xff,0,
                        0,   0,   0xff,
                        0xff,0,   0xff,
                        0,   0xff,0xff,
                        0xff,0xff,0xff,
                        0,   0,   0,
                        0x80,0,   0,
                        0,   0x80,0,
                        0x80,0x80,0,
                        0,   0,   0x80,
                        0x80,0,   0x80,
                        0,   0x80,0x80,
                        0x80,0x80,0x80};
  png_colorp  palette = (png_colorp ) cpalette;
  unsigned char ctransparency[]={0,  255,255,255,255,255,255,255,
                                 255,255,255,255,255,255,255,255};
  png_bytep   trans =   (png_bytep) ctransparency;
   png_uint_32 k;
   png_uint_32 height=png_height;
   png_uint_32 width=256;
   png_uint_32 bytes_per_pixel=1;
   png_byte image[256][256];
   png_bytep row_pointers[256];
   int i,j,c0,c,d,l,last;
   if (height>256) height=256;
   
   memset(image, 0, sizeof(image));
   for (j=0;j<6;j++) if (colors & (1 << j)){
     c0= clrs[j];
     last=png_height-1;
     for (i=0;i<256;i++) {
       c=c0 << ((i&1)?0:4);
       d=png_height-1-data[i+(j<<8)];
       if (d<=0) d=0;
       if (d>=png_height) d=png_height-1;
       image[d][i>>1] |= c;
       if (mode & 1){
//         l=d-mode;
//         if ((i&0x10)==0) l-=mode;
//         l-= (i & 0xf);
//         if (l<0) l=0;
         for (l=d+1; l<png_height; l++)image[l][i>>1] |= c;
//         for (l=png_height; l>d; l--)image[l][i] |= c;
       } else if (mode & 2) {
         for (l=d+1; l<last; l++) image[l][i>>1] |= c; // only one of "for " can fire
         for (l=d-1; l>last; l--) image[l][i>>1] |= c; 
         last=d;
       
       }
       
     }
   }
   
   /* open the file */
/*
   fp = fopen(file_name, "wb");
  
   if (fp == NULL)
      return (ERROR);
*/
   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also check that
    * the library version is compatible with the one used at compile time,
    * in case we are using dynamically linked libraries.  REQUIRED.
    */
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
      NULL,NULL,NULL);

   if (png_ptr == NULL)
   {
//      fclose(fp);
      return (ERROR);
   }

   /* Allocate/initialize the image information data.  REQUIRED */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
//      fclose(fp);
      png_destroy_write_struct(&png_ptr,  png_infopp_NULL);
      return (ERROR);
   }

   /* Set error handling.  REQUIRED if you aren't supplying your own
    * error handling functions in the png_create_write_struct() call.
    */
   if (setjmp(png_jmpbuf(png_ptr)))
   {
      /* If we get here, we had a problem reading the file */
//      fclose(fp);
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return (ERROR);
   }

   /* One of the following I/O initialization functions is REQUIRED */
//#ifdef streams /* I/O initialization method 1 */
   /* set up the output control if you are using standard C streams */
//   png_init_io(png_ptr, fp);
   png_init_io(png_ptr, stdout);
//#else  no_streams /* I/O initialization method 2 */
   /* If you are using replacement read functions, instead of calling
    * png_init_io() here you would call */
//   png_set_write_fn(png_ptr, (void *)user_io_ptr, user_write_fn,
//      user_IO_flush_function);
   /* where user_io_ptr is a structure you want available to the callbacks */
//#endif// no_streams /* only use one initialization method */

   /* This is the hard way */

   /* Set the image information here.  Width and height are up to 2^31,
    * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
    * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
    * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
    * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
    * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
    * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
    */
   png_set_IHDR(png_ptr,
                info_ptr,
                256, // width
                png_height, // height
                4,          // bit_depth,
                PNG_COLOR_TYPE_PALETTE, //PNG_COLOR_TYPE_???,
                PNG_INTERLACE_NONE, //PNG_INTERLACE_????,
                PNG_COMPRESSION_TYPE_BASE,
                PNG_FILTER_TYPE_BASE);

   /* set the palette if there is one.  REQUIRED for indexed-color images */
//   palette = (png_colorp)png_malloc(png_ptr, PNG_MAX_PALETTE_LENGTH
//             * png_sizeof (png_color));
   /* ... set palette colors ... */
   png_set_PLTE(png_ptr,
                info_ptr,
                palette,
                sizeof(cpalette)/3); // PNG_MAX_PALETTE_LENGTH);
    png_set_tRNS(png_ptr,
                info_ptr,
                trans,
                sizeof(trans), //16,
                NULL); //, png_color_16p trans_values)                
  /* You must not free palette here, because png_set_PLTE only makes a link to
      the palette that you malloced.  Wait until you are about to destroy
      the png structure. */

   /* optional significant bit chunk */
   /* if we are dealing with a grayscale image then */
#if 0   
   sig_bit.gray = 1; //true_bit_depth;
   /* otherwise, if we are dealing with a color image then */
   sig_bit.red = 1; //true_red_bit_depth;
   sig_bit.green = 1; //true_green_bit_depth;
   sig_bit.blue = 1; //true_blue_bit_depth;
   /* if the image has an alpha channel then */
   sig_bit.alpha = 1; //true_alpha_bit_depth;
   png_set_sBIT(png_ptr, info_ptr, sig_bit);
#endif

   /* Optional gamma chunk is strongly suggested if you have any guess
    * as to the correct gamma of the image.
    */
//   png_set_gAMA(png_ptr, info_ptr, gamma);

   /* Optionally write comments into the image */
#if 0  
   text_ptr[0].key = "Title";
   text_ptr[0].text = "Histogram";// "Mona Lisa";
   text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
   text_ptr[1].key = "Author";
   text_ptr[1].text = "Elphel"; //"Leonardo DaVinci";
   text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
   text_ptr[2].key = "Description";
   text_ptr[2].text = "color histograms for the camera image/video"; //"<long text>";
   text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
#ifdef PNG_iTXt_SUPPORTED
   text_ptr[0].lang = NULL;
   text_ptr[1].lang = NULL;
   text_ptr[2].lang = NULL;
#endif
   png_set_text(png_ptr, info_ptr, text_ptr, 3);
#endif

   /* other optional chunks like cHRM, bKGD, tRNS, tIME, oFFs, pHYs, */
   /* note that if sRGB is present the gAMA and cHRM chunks must be ignored
    * on read and must be written in accordance with the sRGB profile */

   /* Write the file header information.  REQUIRED */
   png_write_info(png_ptr, info_ptr);

   /* If you want, you can write the info in two steps, in case you need to
    * write your private chunk ahead of PLTE:
    *
    *   png_write_info_before_PLTE(write_ptr, write_info_ptr);
    *   write_my_chunk();
    *   png_write_info(png_ptr, info_ptr);
    *
    * However, given the level of known- and unknown-chunk support in 1.1.0
    * and up, this should no longer be necessary.
    */

   /* Once we write out the header, the compression type on the text
    * chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
    * PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out again
    * at the end.
    */

   /* set up the transformations you want.  Note that these are
    * all optional.  Only call them if you want them.
    */

   /* invert monochrome pixels */
//   png_set_invert_mono(png_ptr);

   /* Shift the pixels up to a legal bit depth and fill in
    * as appropriate to correctly scale the image.
    */
//   png_set_shift(png_ptr, &sig_bit);

   /* pack pixels into bytes */
//   png_set_packing(png_ptr);

   /* swap location of alpha bytes from ARGB to RGBA */
//   png_set_swap_alpha(png_ptr);

   /* Get rid of filler (OR ALPHA) bytes, pack XRGB/RGBX/ARGB/RGBA into
    * RGB (4 channels -> 3 channels). The second parameter is not used.
    */
//   png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);

   /* flip BGR pixels to RGB */
//   png_set_bgr(png_ptr);

   /* swap bytes of 16-bit files to most significant byte first */
//   png_set_swap(png_ptr);

   /* swap bits of 1, 2, 4 bit packed pixel formats */
//   png_set_packswap(png_ptr);

   /* turn on interlace handling if you are not using png_write_image() */
#if 0   
   if (interlacing)
      number_passes = png_set_interlace_handling(png_ptr);
   else
      number_passes = 1;
#endif   

   /* The easiest way to write the image (you may have a different memory
    * layout, however, so choose what fits your needs best).  You need to
    * use the first method if you aren't handling interlacing yourself.
    */
//   png_uint_32 k, height, width;
//   png_byte image[height][width*bytes_per_pixel];
//   png_bytep row_pointers[height];
#if 0
   if (height > PNG_UINT_32_MAX/png_sizeof(png_bytep))
     png_error (png_ptr, "Image is too tall to process in memory");
#endif
   for (k = 0; k < height; k++)
     row_pointers[k] = (png_bytep) image + k*width*bytes_per_pixel;

   /* One of the following output methods is REQUIRED */
   png_write_image(png_ptr, row_pointers);


   /* You can write optional chunks like tEXt, zTXt, and tIME at the end
    * as well.  Shouldn't be necessary in 1.1.0 and up as all the public
    * chunks are supported and you can use png_set_unknown_chunks() to
    * register unknown chunks into the info structure to be written out.
    */

   /* It is REQUIRED to call this to finish writing the rest of the file */
   png_write_end(png_ptr, info_ptr);

   /* If you png_malloced a palette, free it here (don't free info_ptr->palette,
      as recommended in versions 1.0.5m and earlier of this example; if
      libpng mallocs info_ptr->palette, libpng will free it).  If you
      allocated it with malloc() instead of png_malloc(), use free() instead
      of png_free(). */
//   png_free(png_ptr, palette);
//   palette=NULL;

   /* Similarly, if you png_malloced any data that you passed in with
      png_set_something(), such as a hist or trans array, free it here,
      when you can be sure that libpng is through with it. */
//   png_free(png_ptr, trans);
//   trans=NULL;

   /* clean up after the write, and free any memory allocated */
   png_destroy_write_struct(&png_ptr, &info_ptr);

   /* close the file */
//   fclose(fp);

   /* that's it */
   return (OK);
}

int unescape (char * s, int l) {
  int i=0;
  int j=0;
  while ((i<l) && s[i]) {
    if ((s[i]=='%') && (i<(l-2)) && s[i+1]){ // behavior from Mozilla
      s[j++]=(hexdigit(s[i+1])<<4) | hexdigit(s[i+2]);
      i+=3;
    } else s[j++]=s[i++];
  }
  if (i<l) s[j]=0;
  return j;
}

int hexdigit (char c) {
  int i;
  i=c-'0';
  if ((i>=0) && (i<10)) return i;
  i=c-'a'+10;
  if ((i>=10) && (i<16)) return i;
  i=c-'A'+10;
  if ((i>=10) && (i<16)) return i;
  return 0; // could be -1??
}

char * paramValue(struct key_value * params, char * skey) { // returns pointer to parameter value, NULL if not defined
  int i=0;
  if (skey)
   while ((i<QRY_MAXPARAMS) && (params[i].key)) {
    if (strcmp(skey,params[i].key)==0) return   params[i].value;

    i++;
   }
  return NULL;
}

int parseQuery(struct key_value * params, char * qry) {
  char * cp;
  int l=0;
  cp=strtok(qry,"=");
  while ((cp) && (l<QRY_MAXPARAMS)) {
    params[l].key=cp;
    cp=strtok(NULL,"&");
    params[l++].value=cp;
    if (cp) cp=strtok(NULL,"=");
  }
 params[l].key=NULL;
 return l;
}
