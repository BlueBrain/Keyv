/* Copyright (c) BBP/EPFL 2014-2014
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

#include <hello/world.h>
#include <hello/version.h>

#include <iostream>

namespace hello
{
void World::greet()
{
    std::cout << "Hello world version " << Version::getRevString() << std::endl;
}

int World::getN( const int n )
{
    /// \todo Try harder
    /// \bug Only works for integers
    return n;
}
}
