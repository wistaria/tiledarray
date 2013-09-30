/*
 *  This file is a part of TiledArray.
 *  Copyright (C) 2013  Virginia Tech
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Justus Calvin
 *  Department of Chemistry, Virginia Tech
 *
 *  dist_eval_array_eval.cpp
 *  Sep 15, 2013
 *
 */

#include "TiledArray/dist_eval/array_eval.h"
#include "unit_test_config.h"
#include "array_fixture.h"
#include "TiledArray/tile_op/scal.h"
#include "TiledArray/shape.h"


using namespace TiledArray;

// Array evaluator fixture
struct ArrayEvalImplFixture : public TiledRangeFixture {
  typedef Array<int, GlobalFixture::dim> ArrayN;
  typedef math::Scal<ArrayN::value_type::eval_type,
      ArrayN::value_type::eval_type, false> op_type;
  typedef detail::ArrayEvalImpl<ArrayN, op_type, DensePolicy> impl_type;


  ArrayEvalImplFixture() : op(3), array(*GlobalFixture::world, tr) {
    for(ArrayN::range_type::const_iterator it = array.range().begin(); it != array.range().end(); ++it)
      if(array.is_local(*it)) {
        ArrayN::value_type tile(array.trange().make_tile_range(*it));
        for(ArrayN::value_type::iterator tile_it = tile.begin(); tile_it != tile.end(); ++tile_it)
          *tile_it = GlobalFixture::world->rand() % 101;
        array.set(*it, tile); // Fill the tile at *it (the index)
      }
  }

  ~ArrayEvalImplFixture() { }

   op_type op;
   ArrayN array;
}; // ArrayEvalFixture

BOOST_FIXTURE_TEST_SUITE( array_eval_suite, ArrayEvalImplFixture )

BOOST_AUTO_TEST_CASE( constructor )
{
  BOOST_REQUIRE_NO_THROW(impl_type(array, DenseShape(), array.get_pmap(), Permutation(), op));

  impl_type impl(array, DenseShape(), array.get_pmap(), Permutation(), op);

  BOOST_CHECK_EQUAL(& impl.get_world(), GlobalFixture::world);
  BOOST_CHECK(impl.pmap() == array.get_pmap());
  BOOST_CHECK_EQUAL(impl.range(), tr.tiles());
  BOOST_CHECK_EQUAL(impl.trange(), tr);
  BOOST_CHECK_EQUAL(impl.size(), tr.tiles().volume());
  BOOST_CHECK(impl.is_dense());
  for(std::size_t i = 0; i < tr.tiles().volume(); ++i)
    BOOST_CHECK(! impl.is_zero(i));
}

BOOST_AUTO_TEST_CASE( eval_scale )
{
  impl_type impl(array, DenseShape(), array.get_pmap(), Permutation(), op);
  std::shared_ptr<impl_type> pimpl(& impl, & madness::detail::no_delete<impl_type>);
  BOOST_REQUIRE_NO_THROW(impl.eval(pimpl));

  impl_type::pmap_interface::const_iterator it = impl.pmap()->begin();
  const impl_type::pmap_interface::const_iterator end = impl.pmap()->end();

  // Check that each tile has been properly scaled.
  for(; it != end; ++it) {
    // Get the original type
    ArrayN::value_type array_tile = array.find(*it);

    // Get the array evaluator tile.
    madness::Future<impl_type::value_type> impl_tile;
    BOOST_REQUIRE_NO_THROW(impl_tile = impl.move(*it));

    // Force the evaluation of the tile
    impl_type::eval_type eval_tile;
    BOOST_REQUIRE_NO_THROW(eval_tile = impl_tile.get());

    // Check that the result tile is correctly modified.
    BOOST_CHECK_EQUAL(eval_tile.range(), array_tile.range());
    for(std::size_t i = 0ul; i < eval_tile.size(); ++i) {
      BOOST_CHECK_EQUAL(eval_tile[i], 3 * array_tile[i]);
    }

  }
}

BOOST_AUTO_TEST_CASE( eval_permute )
{
  // Create permutation to be applied in the array evaluations
  std::array<std::size_t, GlobalFixture::dim> p;
  for(std::size_t i = 0; i < p.size(); ++i)
    p[i] = (i + p.size() - 1) % p.size();
  Permutation perm(p.begin(), p.end());

  // Redefine the types for the new operation.
  typedef math::Noop<ArrayN::value_type, ArrayN::value_type, false> op_type;
  typedef detail::ArrayEvalImpl<ArrayN, op_type, DensePolicy> impl_type;

  // Construct the permuting operation.
  op_type op(perm);

  // Construct and evaluate
  impl_type impl(array, DenseShape(), array.get_pmap(), perm, op);
  std::shared_ptr<impl_type> pimpl(& impl, & madness::detail::no_delete<impl_type>);
  BOOST_REQUIRE_NO_THROW(impl.eval(pimpl));

  // Check that each tile has been moved to the correct location and has been
  // properly permuted.
  impl_type::pmap_interface::const_iterator it = impl.pmap()->begin();
  const impl_type::pmap_interface::const_iterator end = impl.pmap()->end();
  Permutation inv_perm = -perm;
  for(; it != end; ++it) {
    // Get the original type
    ArrayN::value_type array_tile = array.find(inv_perm ^ impl.range().idx(*it));

    // Get the corresponding array evaluator tile.
    madness::Future<impl_type::value_type> impl_tile;
    BOOST_REQUIRE_NO_THROW(impl_tile = impl.move(*it));

    // Force the evaluation of the tile
    impl_type::eval_type eval_tile;
    BOOST_REQUIRE_NO_THROW(
    try { \
      eval_tile = impl_tile.get(); \
    } catch(std::exception& e) { \
      std::cout << e.what() << "\n"; \
      throw; \
    } \
    );

    // Check that the result tile is correctly modified.
    BOOST_CHECK_EQUAL(eval_tile.range(), perm ^ array_tile.range());
    for(std::size_t i = 0ul; i < eval_tile.size(); ++i) {
      BOOST_CHECK_EQUAL(eval_tile[perm ^ array_tile.range().idx(i)], array_tile[i]);
    }

  }
}

BOOST_AUTO_TEST_SUITE_END()