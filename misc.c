/* -*- mode: C -*- Time-stamp: "2013-06-16 16:22:34 holzplatten"
 *
 *       File:         misc.c
 *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
 *       Date:         Sun Jun  1 19:30:28 2013
 *
 *       
 *
 */

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


#include <misc.h>




/*-
  *      Routine:       calculate_inode_count
  *
  *      Purpose:
  *              Estima el número de inodos necesarios para un sistema
  *              de archivos de tamaño size.
  *      Conditions:
  *              none (por ahora)
  *      Returns:
  *              Un entero (100 por ahora).
  *
  */
unsigned long
calculate_inode_count(unsigned long size)
{
  return (unsigned long) 100;
}
