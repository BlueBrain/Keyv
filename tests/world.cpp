
/* Copyright (c) 2013-2014, EPFL/Blue Brain Project
 *                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>
 */

#include <hello/world.h>

#define BOOST_TEST_MODULE world
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( arithmetic_test )
{
    int a = 1;
    int b = 2;
    BOOST_CHECK_NE( a, b );
    b = a;
    BOOST_CHECK_EQUAL( a, b );
}

BOOST_AUTO_TEST_CASE( hello_test )
{
    hello::World world;
    world.greet();

    const int ret = world.getN( 3 );
    BOOST_CHECK_EQUAL( ret, 3 );
}
