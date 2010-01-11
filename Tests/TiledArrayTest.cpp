//============================================================================
// Name        : TiledArrayTest.cpp
// Author      : Justus Calvin
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C, Ansi-style
//============================================================================

#include <TiledArray/config.h>
#define BOOST_TEST_MAIN Tiled Array Tests
#if TA_UNIT_TEST_LINKAGE
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#else
#include <boost/test/included/unit_test.hpp>
#endif
#include "madness_fixture.h"

MadnessFixture::MadnessFixture() {
  madness::initialize(boost::unit_test::framework::master_test_suite().argc,
      boost::unit_test::framework::master_test_suite().argv);

  if(count == 0) {
    world = new madness::World(MPI::COMM_WORLD);
    world->args(boost::unit_test::framework::master_test_suite().argc,
        boost::unit_test::framework::master_test_suite().argv);
  }

  ++count;
  world->gop.fence();
}

MadnessFixture::~MadnessFixture() {
  world->gop.fence();

  --count;
  if(count == 0) {
    delete world;
    world = NULL;
  }
  madness::finalize();
}

madness::World* MadnessFixture::world = NULL;
unsigned int MadnessFixture::count = 0;


// This line will initialize mpi and madness.
BOOST_GLOBAL_FIXTURE( MadnessFixture )