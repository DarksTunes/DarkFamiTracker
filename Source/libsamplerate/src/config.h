/* config.h - generated by CMake. */

/* This cannot be generated if not building with CMake, so instead,
   we hardcode this as a file.*/

/* Name of package */
#define PACKAGE "libsamplerate"

/* Version number of package */
#define VERSION "0.2.2"

/* Target processor clips on negative float to int conversion. */
#define CPU_CLIPS_NEGATIVE 0

/* Target processor clips on positive float to int conversion. */
#define CPU_CLIPS_POSITIVE 0

/* Target processor is big endian. */
#define CPU_IS_BIG_ENDIAN 0

/* Target processor is little endian. */
#define CPU_IS_LITTLE_ENDIAN 1

/* Define to 1 if you have the `alarm' function. */
#define HAVE_ALARM 0

/* Define to 1 if you have the <alsa/asoundlib.h> header file. */
#define HAVE_ALSA 0

/* Set to 1 if you have libfftw3. */
#define HAVE_FFTW3 0

/* Define if you have C99's lrint function. */
#define HAVE_LRINT 0

/* Define if you have C99's lrintf function. */
#define HAVE_LRINTF 0

/* Define if you have signal SIGALRM. */
#define HAVE_SIGALRM 0

/* Define to 1 if you have the `signal' function. */
#define HAVE_SIGNAL 1

/* Set to 1 if you have libsndfile. */
#define HAVE_SNDFILE 0

/* Define to 1 if you have the <stdbool.h> header file. */
#define HAVE_STDBOOL_H

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 0

/* Define to 1 if you have the <sys/times.h> header file. */
#define HAVE_SYS_TIMES_H 0

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Define to 1 if the compiler supports simple visibility declarations. */
/* #undef HAVE_VISIBILITY */

/* define fast samplerate convertor */
#define ENABLE_SINC_FAST_CONVERTER

/* define balanced samplerate convertor */
#define ENABLE_SINC_MEDIUM_CONVERTER

/* define best samplerate convertor */
#define ENABLE_SINC_BEST_CONVERTER

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 
