/* -*- mode: C -*- Time-stamp: "2013-06-09 19:30:37 holzplatten"
 *
 *       File:         misc.c
 *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
 *       Date:         Sun Jun  1 19:30:28 2013
 *
 *       
 *
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
