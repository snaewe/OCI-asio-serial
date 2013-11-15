
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl GPSDDSLib
// ------------------------------
#ifndef GPSDDSLIB_EXPORT_H
#define GPSDDSLIB_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (GPSDDSLIB_HAS_DLL)
#  define GPSDDSLIB_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && GPSDDSLIB_HAS_DLL */

#if !defined (GPSDDSLIB_HAS_DLL)
#  define GPSDDSLIB_HAS_DLL 1
#endif /* ! GPSDDSLIB_HAS_DLL */

#if defined (GPSDDSLIB_HAS_DLL) && (GPSDDSLIB_HAS_DLL == 1)
#  if defined (GPSDDSLIB_BUILD_DLL)
#    define GPSDDSLib_Export ACE_Proper_Export_Flag
#    define GPSDDSLIB_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define GPSDDSLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* GPSDDSLIB_BUILD_DLL */
#    define GPSDDSLib_Export ACE_Proper_Import_Flag
#    define GPSDDSLIB_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define GPSDDSLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* GPSDDSLIB_BUILD_DLL */
#else /* GPSDDSLIB_HAS_DLL == 1 */
#  define GPSDDSLib_Export
#  define GPSDDSLIB_SINGLETON_DECLARATION(T)
#  define GPSDDSLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* GPSDDSLIB_HAS_DLL == 1 */

// Set GPSDDSLIB_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (GPSDDSLIB_NTRACE)
#  if (ACE_NTRACE == 1)
#    define GPSDDSLIB_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define GPSDDSLIB_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !GPSDDSLIB_NTRACE */

#if (GPSDDSLIB_NTRACE == 1)
#  define GPSDDSLIB_TRACE(X)
#else /* (GPSDDSLIB_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define GPSDDSLIB_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (GPSDDSLIB_NTRACE == 1) */

#endif /* GPSDDSLIB_EXPORT_H */

// End of auto generated file.
