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
 *  scal_mult.h
 *  May 8, 2013
 *
 */

#ifndef TILEDARRAY_TILE_OP_SCAL_MULT_H__INCLUDED
#define TILEDARRAY_TILE_OP_SCAL_MULT_H__INCLUDED

#include <TiledArray/tile_op/permute.h>

namespace TiledArray {
  namespace math {

    /// Tile multiplication and scale operation

    /// This multiplication operation will multiply the content two tiles, then
    /// scale and apply a permutation to the result tensor. If no permutation is
    /// given or the permutation is null, then the result is not permuted.
    /// \tparam Result The result type
    /// \tparam Left The left-hand argument type
    /// \tparam Right The right-hand argument type
    /// \tparam LeftConsumable A flag that is \c true when the left-hand
    /// argument is consumable.
    /// \tparam RightConsumable A flag that is \c true when the right-hand
    /// argument is consumable.
    template <typename Result, typename Left, typename Right, bool LeftConsumable,
        bool RightConsumable>
    class ScalMult {
    public:
      typedef ScalMult<Result, Left, Right, false, false> ScalMult_; ///< This object type
      typedef typename madness::if_c<LeftConsumable, Left&, const Left&>::type first_argument_type; ///< The left-hand argument type
      typedef typename madness::if_c<RightConsumable, Right&, const Right&>::type second_argument_type; ///< The right-hand argument type
      typedef Result result_type; ///< The result tile type
      typedef typename TiledArray::detail::scalar_type<Result>::type scalar_type; ///< Scalar type

    private:
      Permutation perm_; ///< The result permutation
      scalar_type factor_; ///< The scaling factor

      // Element operation functor types

      typedef ScalMultiplies<typename Left::value_type,
          typename Right::value_type, typename Result::value_type> scal_multiplies_op;
      typedef ScalMultipliesAssign<typename Left::value_type,
                  typename Right::value_type> scal_multiplies_assign_left_op;
      typedef ScalMultipliesAssign<typename Right::value_type,
                  typename Left::value_type> scal_multiplies_assign_right_op;

      // Permuting tile evaluation function
      // These operations cannot consume the argument tile since this operation
      // requires temporary storage space.

      result_type permute(first_argument_type first, second_argument_type second) const {
        result_type result;
        TiledArray::math::permute(result, perm_, first, second,
            scal_multiplies_op(factor_));
        return result;
      }

      // Non-permuting tile evaluation functions
      // The compiler will select the correct functions based on the consumability
      // of the arguments.

      template <bool LC, bool RC>
      typename madness::disable_if_c<(LC && std::is_same<Result, Left>::value) ||
          (RC && std::is_same<Result, Right>::value), result_type>::type
      no_permute(first_argument_type first, second_argument_type second) const {
        return result_type(first.range(), first.data(), second.data(),
            scal_multiplies_op(factor_));
      }

      template <bool LC, bool RC>
      typename madness::enable_if_c<LC && std::is_same<Result, Left>::value, result_type>::type
      no_permute(first_argument_type first, second_argument_type second) const {
        vector_assign(first.size(), second.data(), first.data(),
            scal_multiplies_assign_left_op(factor_));
        return first;
      }

      template <bool LC, bool RC>
      typename madness::enable_if_c<(RC && std::is_same<Result, Right>::value) &&
          (!(LC && std::is_same<Result, Left>::value)), result_type>::type
      no_permute(first_argument_type first, second_argument_type second) const {
        vector_assign(second.size(), first.data(), second.data(),
            scal_multiplies_assign_right_op(factor_));
        return second;
      }

    public:
      /// Default constructor

      /// Construct a multiplication operation that does not permute the result tile
      /// and has a scaling factor of 1.
      ScalMult() : perm_(), factor_(1) { }

      /// Permute constructor

      /// Construct a multiplication operation that scales the result tensor
      /// \param factor The scaling factor for the operation [default = 1]
      ScalMult(const scalar_type factor) :
        perm_(), factor_(factor)
      { }

      /// Permute constructor

      /// Construct a multiplication operation that permutes and scales the result tensor
      /// \param perm The permutation to apply to the result tile
      /// \param factor The scaling factor for the operation [default = 1]
      ScalMult(const Permutation& perm, const scalar_type factor = scalar_type(1)) :
        perm_(perm), factor_(factor)
      { }

      /// Copy constructor

      /// \param other The multiplication operation object to be copied
      ScalMult(const ScalMult_& other) : perm_(other.perm_), factor_(other.factor_) { }

      /// Copy assignment

      /// \param other The multiplication operation object to be copied
      /// \return A reference to this object
      ScalMult_& operator=(const ScalMult_& other) {
        perm_ = other.perm_;
        factor_ = other.factor_;
        return *this;
      }

      /// Add and scale two non-zero tiles and possibly permute

      /// \param first The left-hand argument
      /// \param second The right-hand argument
      /// \return The scaled sum and permutation of \c first and \c second
      result_type operator()(first_argument_type first, second_argument_type second) const {
        TA_ASSERT(first.range() == second.range());

        if(perm_.dim() > 1)
          return permute(first, second);

        return no_permute<LeftConsumable, RightConsumable>(first, second);
      }
    }; // class ScalMult

  }  // namespace math
} // namespace TiledArray

#endif // TILEDARRAY_TILE_OP_SCAL_MULT_H__INCLUDED
