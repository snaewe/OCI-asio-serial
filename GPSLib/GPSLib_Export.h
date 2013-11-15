
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl GPSLib
// ------------------------------
#ifndef GPSLIB_EXPORT_H
#define GPSLIB_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (GPSLIB_HAS_DLL)
#  define GPSLIB_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && GPSLIB_HAS_DLL */

#if !defined (GPSLIB_HAS_DLL)
#  define GPSLIB_HAS_DLL 1
#endif /* ! GPSLIB_HAS_DLL */

#if defined (GPSLIB_HAS_DLL) && (GPSLIB_HAS_DLL == 1)
#  if defined (GPSLIB_BUILD_DLL)
#    define GPSLib_Export ACE_Proper_Export_Flag
#    define GPSLIB_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define GPSLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* GPSLIB_BUILD_DLL */
#    define GPSLib_Export ACE_Proper_Import_Flag
#    define GPSLIB_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define GPSLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* GPSLIB_BUILD_DLL */
#else /* GPSLIB_HAS_DLL == 1 */
#  define GPSLib_Export
#  define GPSLIB_SINGLETON_DECLARATION(T)
#  define GPSLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* GPSLIB_HAS_DLL == 1 */

// Set GPSLIB_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (GPSLIB_NTRACE)
#  if (ACE_NTRACE == 1)
#    define GPSLIB_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define GPSLIB_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !GPSLIB_NTRACE */

#if (GPSLIB_NTRACE == 1)
#  define GPSLIB_TRACE(X)
#else /* (GPSLIB_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define GPSLIB_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (GPSLIB_NTRACE == 1) */

#endif /* GPSLIB_EXPORT_H */

// End of auto generated file.
