// -*- c-basic-offset: 4 -*-
/** @file hugin_math.h
 *
 *  @brief misc math function & classes used by other parts
 *         of the program
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "hugin_math.h"
#include "hugin_utils/utils.h"

 /** namespace for various utils */
namespace hugin_utils
{
    int _gcd(int a, int b)
    {
        // calculate greated common divisor using Euclidean algorithm
        return b == 0 ? a : _gcd(b, a % b);
    }

    int gcd(int a, int b)
    {
        // enforce that both arguments are positive
        return _gcd(abs(a), abs(b));
    }

} // namespace
