/* Copyright (c) BBP/EPFL 2014-2015
 *               Stefan.Eilemann@epfl.ch
 *
 * This file is part of Hello <https://github.com/BlueBrain/Hello>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef HELLO_WORLD_H
#define HELLO_WORLD_H

#include <hello/api.h>

/**
 * The namespace to rule the world.
 *
 * The hello namespace implements the world to provide a template project.
 */
namespace hello
{
/**
 * The world class.
 *
 * Does all the work in the world. Not thread safe.
 */
class World
{
public:
    /** Greet the caller. @version 1.0 */
    HELLO_API void greet();

    /** @return the input value. */
    HELLO_API static int getN(const int n);
};

} // namespace hello

#endif // HELLO_WORLD_H
