/* Provide a arpa/inet.h header file for systems lacking it (e.g., Mingw).
   Copyright (C) 2005, 2006 Free Software Foundation, Inc.
   Written by Simon Josefsson.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _ARPA_INET_H
#define _ARPA_INET_H

/* This file is supposed to be used on platforms that lack
   arpa/inet.h.  It is intended to provide definitions and prototypes
   needed by an application. */

#include <sys/types.h>

/* Get socklen_t.  Also gets ntohl, htonl, etc on mingw, which should
   be declared by this header file, since our sys/socket.h replacement
   includes winsock2.h.  */
#include <sys/socket.h>

/* The inet_ntop and inet_pton functions aren't universally available,
   in particular Mingw doesn't have it.  Declare them.  */

#if !HAVE_DECL_INET_NTOP
/* Converts an internet address from internal format to a printable,
   presentable format.
   AF is an internet address family, such as AF_INET or AF_INET6.
   SRC points to a 'struct in_addr' (for AF_INET) or 'struct in6_addr'
   (for AF_INET6).
   DST points to a buffer having room for CNT bytes.
   The printable representation of the address (in numeric form, not
   surrounded by [...], no reverse DNS is done) is placed in DST, and
   DST is returned.  If an error occurs, the return value is NULL and
   errno is set.  If CNT bytes are not sufficient to hold the result,
   the return value is NULL and errno is set to ENOSPC.  A good value
   for CNT is 46.

   For more details, see the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/inet_ntop.html>.  */
extern const char *inet_ntop (int af, const void *restrict src,
			      char *restrict dst, socklen_t cnt);
#endif


#if !HAVE_DECL_INET_PTON
extern int inet_pton (int af, const char *restrict src,
		      void *restrict dst);
#endif

#endif /* _ARPA_INET_H */
