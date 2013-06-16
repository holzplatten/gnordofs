/*
  Copyright (C) 2013 Pedro J. Ruiz López <holzplatten@es.gnu.org>

  This file is part of GnordoFS.

  GnordoFS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  GnordoFS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef _MISC_H_
#define _MISC_H_

#include <syslog.h>

#define DEBUG(...) syslog(LOG_LOCAL0 | LOG_DEBUG, __VA_ARGS__);
#define DEBUG_VERBOSE(...) syslog(LOG_LOCAL0 | LOG_DEBUG, __VA_ARGS__);

unsigned long calculate_inode_count(unsigned long size);

#endif
