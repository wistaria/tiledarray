#include <tiled_range1.h>
#include <iostream>
#include <boost/array.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
#include "iterationtest.h"

using namespace TiledArray;

struct Range1BaseFixture {

  Range1BaseFixture() {
    a[0] = 0;
    a[1] = 3;
    a[2] = 7;
    a[3] = 10;
    a[4] = 20;
    a[5] = 50;
  }
  ~Range1BaseFixture() { }

  boost::array<std::size_t, 6> a;
};

struct Range1Fixture : public Range1BaseFixture {
  typedef TiledRange1<std::size_t> range1_type;
  Range1Fixture() : Range1BaseFixture(), r(a.begin(), a.end()), tiles(0,5),
      elements(0,50)
  {
    std::copy(r.begin(), r.end(), tile.begin());
  }
  ~Range1Fixture() { }

  range1_type r;
  range1_type::range_type tiles;
  range1_type::element_range_type elements;
  boost::array<range1_type::tile_range_type, 5> tile;
};

BOOST_FIXTURE_TEST_SUITE( range1_suite, Range1Fixture )

BOOST_AUTO_TEST_CASE( block_accessor )
{
  BOOST_CHECK_EQUAL(r.tiles(), tiles);
  BOOST_CHECK_EQUAL(r.elements(), elements);
  BOOST_CHECK_EQUAL(r.tile(0), tile[0]);
  BOOST_CHECK_EQUAL(r.tile(1), tile[1]);
  BOOST_CHECK_EQUAL(r.tile(2), tile[2]);
  BOOST_CHECK_EQUAL(r.tile(3), tile[3]);
  BOOST_CHECK_EQUAL(r.tile(4), tile[4]);
  BOOST_CHECK_THROW(r.tile(5), std::out_of_range);
}

BOOST_AUTO_TEST_CASE( block_info )
{
  BOOST_CHECK_EQUAL(r.tiles().size(), 5);
  BOOST_CHECK_EQUAL(r.tiles().start(), 0);
  BOOST_CHECK_EQUAL(r.tiles().finish(), 5);
  BOOST_CHECK_EQUAL(r.elements().size(), 50);
  BOOST_CHECK_EQUAL(r.elements().start(), 0);
  BOOST_CHECK_EQUAL(r.elements().finish(), 50);
  BOOST_CHECK_EQUAL(r.tile(0).size(), 3);
  BOOST_CHECK_EQUAL(r.tile(0).start(), 0);
  BOOST_CHECK_EQUAL(r.tile(0).finish(), 3);
}

BOOST_AUTO_TEST_CASE( constructor )
{
  BOOST_REQUIRE_NO_THROW(range1_type r0);
  range1_type r0;                  // check default construction and range info.
  BOOST_CHECK_EQUAL(r0.tiles(), range1_type::range_type(0,0));
  BOOST_CHECK_EQUAL(r0.elements(), range1_type::element_range_type(0,0));
  BOOST_CHECK_EQUAL(r0.tile(0), range1_type::tile_range_type(0,0));

  BOOST_REQUIRE_NO_THROW(range1_type r1(a.begin(), a.end()));
  range1_type r1(a.begin(), a.end());           // check construction with a
  BOOST_CHECK_EQUAL(r1.tiles(), tiles);         // iterators and the range info.
  BOOST_CHECK_EQUAL(r1.elements(), elements);
  BOOST_CHECK_EQUAL_COLLECTIONS(r1.begin(), r1.end(), tile.begin(), tile.end());

  BOOST_REQUIRE_NO_THROW(range1_type r2(r));
  range1_type r2(r);                                   // check copy constructor
  BOOST_CHECK_EQUAL(r1.tiles(), tiles);
  BOOST_CHECK_EQUAL(r1.elements(), elements);
  BOOST_CHECK_EQUAL_COLLECTIONS(r1.begin(), r1.end(), tile.begin(), tile.end());

  BOOST_REQUIRE_NO_THROW(range1_type r3(a.begin(), a.end(), 2));
  range1_type r3(a.begin(), a.end(), 2);// check construction with a with a tile offset.
  BOOST_CHECK_EQUAL(r3.tiles(), range1_type::range_type(2,7));
  BOOST_CHECK_EQUAL(r3.elements(), elements);
  BOOST_CHECK_EQUAL_COLLECTIONS(r3.begin(), r3.end(), tile.begin(), tile.end());

  BOOST_REQUIRE_NO_THROW(range1_type r4(a.begin() + 1, a.end()));
  range1_type r4(a.begin() + 1, a.end());// check construction with a with a element offset.
  BOOST_CHECK_EQUAL(r4.tiles(), range1_type::range_type(0,4));
  BOOST_CHECK_EQUAL(r4.elements(), range1_type::tile_range_type(3,50));
  BOOST_CHECK_EQUAL_COLLECTIONS(r4.begin(), r4.end(), tile.begin() + 1, tile.end());
}

BOOST_AUTO_TEST_CASE( ostream )
{
  boost::test_tools::output_test_stream output;
  output << r;
  BOOST_CHECK( !output.is_empty( false ) );
  BOOST_CHECK( output.check_length( 42, false ) );
  BOOST_CHECK( output.is_equal( "( tiles = [ 0, 5 ), elements = [ 0, 50 ) )" ) );
}

BOOST_AUTO_TEST_CASE( element2tile )
{
  boost::array<std::size_t, 50> e = {{0,0,0,1,1,1,1,2,2,2,3,3,3,3,3,3,3,3,3,3,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}};
  boost::array<std::size_t, 50> c;

  for(std::size_t i = 0; i < 50; ++i)
    c[i] = r.element2tile(i);

  BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), e.begin(), e.end());
}

BOOST_AUTO_TEST_CASE( resize )
{
  range1_type r1;
  BOOST_CHECK_EQUAL(r1.resize(a.begin(), a.end(), 0), r); // check retiling function
  BOOST_CHECK_EQUAL(r1, r);
}

BOOST_AUTO_TEST_CASE( comparison )
{
  range1_type r1(r);
  BOOST_CHECK(r1 == r);     // check equality operator
  BOOST_CHECK(! (r1 != r)); // check not-equal operator
  r1.resize(a.begin(), a.end(), 3);
  BOOST_CHECK(! (r1 == r)); // check for inequality with different start  point for tiles
  BOOST_CHECK(r1 != r);
  boost::array<std::size_t, 6> a1 = a;
  a1[2] = 8;
  r1.resize(a1.begin(), a1.end(), 0);
  BOOST_CHECK(! (r1 == r)); // check for inequality with different tile boundaries.
  BOOST_CHECK(r1 != r);
  a1[2] = 7;
  a1[4] = 50;
  r1.resize(a1.begin(), a1.end() - 1, 0);
  BOOST_CHECK(! (r1 == r)); // check for inequality with different number of tiles.
  BOOST_CHECK(r1 != r);
}

BOOST_AUTO_TEST_CASE( iteration )
{
  BOOST_CHECK_EQUAL(const_iteration_test(r, tile.begin(), tile.end()), 5);
                                    // check for proper iteration functionality.
  BOOST_CHECK_EQUAL( * r.find(11), tile[3]); // check that find returns an
                                             // iterator to the correct tile.

  BOOST_CHECK( r.find(55) == r.end()); // check that the iterator points to
                           // the end() iterator if the element is out of range.
}

BOOST_AUTO_TEST_CASE( assignment )
{
  range1_type r1;
  BOOST_CHECK_NE( r1, r);
  BOOST_CHECK_EQUAL((r1 = r), r); // check operator=
  BOOST_CHECK_EQUAL(r1, r);
}

BOOST_AUTO_TEST_SUITE_END()