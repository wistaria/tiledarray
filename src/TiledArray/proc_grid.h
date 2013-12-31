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
 *  proc_grid.h
 *  Nov 6, 2013
 *
 */

#ifndef TILEDARRAY_GRID_H__INCLUDED
#define TILEDARRAY_GRID_H__INCLUDED

#include <TiledArray/error.h>
#include <TiledArray/madness.h>
#include <TiledArray/pmap/cyclic_pmap.h>
#include <TiledArray/math/eigen.h>
#include <TiledArray/math/math.h>
#include <stdint.h>

namespace TiledArray {
  namespace detail {

    /// A 2D processor grid

    /// ProcGrid attempts to create a near optimal 2D grid of P processes for
    /// an MxN grid of tiles. The size of the grid is optimized such that the
    /// total communication time required for SUMMA and the number of unused
    /// processes is minimized. The total communication time of SUMMA is given
    /// by:
    /// \f[
    ///   T = \frac{MK}{P_{\rm{row}}} \left(\alpha + \frac{mk}{\beta}\right)
    ///     \left((P/P_{\rm{row}}) - 1\right) + \frac{KN}{P/P_{\rm{row}}}
    ///     \left(\alpha + \frac{kn}{\beta}\right) \left(P_{\rm{row}} - 1\right)
    /// \f]
    /// where \f$P_{\rm{row}}\f$ is the number of process rows; \f$M\f$,
    /// \f$N\f$, and \f$K\f$ are the number of tile rows and columns in a matrix
    /// product with average tiles sizes of \f$m\f$, \f$n\f$, and \f$k\f$,
    /// respectively; \f$P\f$ is the number or available processes; \f$\alpha\f$
    /// is the message latency; and \f$\beta\f$ is the message data rate. If we
    /// evaluate \f$dT/dP_{\rm{row}} = 0\f$ and assume that
    /// \f$\alpha \approx 0\f$, the expression simplifies to:
    /// \f[
    ///   Nn(2 P_{\rm{row}}^4 - P_{\rm{row}}^3) + Mm(P P_{\rm{row}} - P^2) = 0
    /// \f]
    /// where the positive, real root of \f$P_{\rm{row}}\f$ give the optimal
    /// optimal communication time.
    class ProcGrid {
    public:
      typedef uint_fast32_t size_type;

    private:
      madness::World& world_; ///< The world where this process grid lives
      size_type rows_; ///< Number of element rows
      size_type cols_; ///< Number of element columns
      size_type size_; ///< Number of elements
      size_type proc_rows_; ///< Number of rows in the process grid
      size_type proc_cols_; ///< Number of columns in the process grid
      size_type proc_size_; ///< Number of processes in the process grid. This
                         ///<  may be less than the number of processes in world.
      ProcessID rank_row_; ///< This process's row in the process grid
      ProcessID rank_col_; ///< This process's column in the process grid
      size_type local_rows_; ///< The number of local element rows
      size_type local_cols_; ///< The number of local element columns
      size_type local_size_; ///< Number of local elements


      /// Compute the number of process rows that minimizes communication

      /// This function computes the optimal number of process row such that the
      /// communication time of a single SUMMA iteration is minimum.
      /// \param nprocs The number of processes
      /// \param M The number of row blocks
      /// \param N The number of column blocks
      /// \return The number of process rows that minimizes communication time
      static size_type optimal_proc_row(const double nprocs, const double Mm,
          const double Nn)
      {
        // Compute the initial guess for P_row. This is the optimal guess when
        // Mm is equal to Nn.
        double P_row_estimate = std::sqrt(nprocs);

        // Here we want to find the positive, real root of the polynomial:
        //   Nn(2x^4 - x^3) + Mm(Px - 2P^2) = 0
        // using a Newton-Raphson algorithm.

        // Precompute some constants
        const double PMm = nprocs * Mm;
        const double two_P = 2.0 * nprocs;

        const unsigned int max_it = 21;
        unsigned int it = 0;
        double r = 0.0;
        do {
          // Precompute P_row squared
          const double P_row2 = P_row_estimate * P_row_estimate;
          const double NnP_row2 = Nn * P_row2;

          // Compute the value of f(P_row_estimate) and df(P_row_estimate)
          const double f = NnP_row2 * ( 2.0 * P_row2 - P_row_estimate)
              + PMm * ( P_row_estimate - two_P);
          const double df = NnP_row2 * ( 8.0 * P_row_estimate - 3.0) + PMm;

          // Compute a new guess for P_row
          const double P_row_n1 = P_row_estimate - (f / df);

          // Compute the residule for this iteration
          r = std::abs(P_row_n1 - P_row_estimate);

          // Update the guess
          P_row_estimate = P_row_n1;

        } while((r > 0.1) && ((++it) < max_it));

        return P_row_estimate + 0.5;
      }

      /// Search for optimal values of x and y

      /// This function will search for values of x and y such that minimize the
      /// number of unused processes, subject to the constraint that
      /// <tt>x*y <= nprocs</tt>.
      void minimize_unused_procs(size_type& x, size_type& y,
          const size_type nprocs, const size_type min_x, const size_type max_x)
      {
        size_type unused = x * y;
        if(unused == 0u)
          return;

        // Compute the range of values for x to be tested.
        const size_type delta = std::max<size_type>(1ul, TiledArray::math::log2(nprocs));

        const size_type min_test_x = std::max<int_fast32_t>(min_x, int_fast32_t(x) - delta);
        size_type test_x = std::min(x + delta, max_x);

        for(; test_x >= min_test_x; --test_x) {
          const size_type test_y = nprocs / test_x;
          const size_type test_unused = nprocs - test_x * test_y;

          if(test_unused < unused) {
            x = test_x;
            y = test_y;
            unused = test_unused;

            if(unused == 0u)
              return;
          }

        }
      }

      /// Member variable initialization

      /// This function initializes the member variables with with the optimal
      /// sizes.
      void init(const size_type rank, const size_type nprocs,
          const std::size_t row_size, const std::size_t col_size)
      {
        // Check for the simple cases first ...
        if(nprocs == 1ul) { // Only one process

          // Set process grid sizes
          proc_rows_ = 1ul;
          proc_cols_ = 1ul;
          proc_size_ = 1ul;

          // Set this process rank
          rank_row_ = 0;
          rank_col_ = 0;

          // Set local counts
          local_rows_ = rows_;
          local_cols_ = cols_;
          local_size_ = size_;

        } else if(size_ <= nprocs) { // Max one tile per process

          // Set process grid sizes
          proc_rows_ = rows_;
          proc_cols_ = cols_;
          proc_size_ = size_;

          if(rank < proc_size_) {
            // Set this process rank
            rank_row_ = rank / proc_cols_;
            rank_col_ = rank % proc_cols_;

            // Set local counts
            local_rows_ = 1ul;
            local_cols_ = 1ul;
            local_size_ = 1ul;
          }

        } else { // The not so simple case

          // Compute the limits for process rows
          const size_type min_proc_rows =
              std::max<size_type>(((nprocs + cols_ - 1ul) / cols_), 1ul);
          const size_type max_proc_rows = std::min<size_type>(nprocs, rows_);

          // Compute optimal the number of process rows and columns in terms of
          // communication time.
          proc_rows_ = std::max<size_type>(min_proc_rows,
              std::min<size_type>(optimal_proc_row(nprocs, row_size, col_size),
                  max_proc_rows));
          proc_cols_ = nprocs / proc_rows_;

          if((proc_rows_ > min_proc_rows) && (proc_rows_ < max_proc_rows)) {
            // Search for the values of proc_rows_ and proc_cols_ that minimizes
            // the number of unused processes in the process grid.
            minimize_unused_procs(proc_rows_, proc_cols_, nprocs,
                min_proc_rows, max_proc_rows);
          }

          proc_size_ = proc_rows_ * proc_cols_;

          if(rank < proc_size_) {
            // Set this process rank
            rank_row_ = rank / proc_cols_;
            rank_col_ = rank % proc_cols_;

            // Set local counts
            local_rows_ = (rows_ / proc_rows_) + (rank_row_ < (rows_ % proc_rows_) ? 1ul : 0ul);
            local_cols_ = (cols_ / proc_cols_) + (rank_col_ < (cols_ % proc_cols_) ? 1ul : 0ul);
            local_size_ = local_rows_ * local_cols_;
          }
        }
      }

    public:

      /// Construct a process grid

      // This constructor makes a rough estimate of the optimal process
      // dimensions. The goal is for the ratios of proc_rows/proc_cols and
      // rows/cols to be approximately equal.
      /// \param world The world where the process grid will live
      /// \param rows The number of tile rows
      /// \param cols The number of tile columns
      /// \param row_size The number of element rows
      /// \param col_size The number of element columns
      ProcGrid(madness::World& world, const size_type rows, const size_type cols,
          const std::size_t row_size, const std::size_t col_size) :
        world_(world), rows_(rows), cols_(cols), size_(rows_ * cols_),
        proc_rows_(0ul), proc_cols_(0ul), proc_size_(0ul),
        rank_row_(-1), rank_col_(-1),
        local_rows_(0ul), local_cols_(0ul), local_size_(0ul)
      {
        // Check for non-zero sizes
        TA_ASSERT(rows_ >= 1ul);
        TA_ASSERT(cols_ >= 1ul);
        TA_ASSERT(row_size >= 1ul);
        TA_ASSERT(col_size >= 1ul);

        init(world_.rank(), world_.size(), row_size, col_size);
      }

#ifdef TILEDARRAY_ENABLE_TEST_PROC_GRID
      // Note: The following function is here for testing purposes only. It
      // has the same functionality as the default constructor above, except the
      // rank and number of processes can be specified.


      /// Construct a process grid

      // This constructor makes a rough estimate of the optimal process
      // dimensions. The goal is for the ratios of proc_rows/proc_cols and
      // rows/cols to be approximately equal.
      /// \param world The world where the process grid will live
      /// \param test_rank Test rank
      /// \param test_nprocs Test number of procs
      /// \param rows The number of tile rows
      /// \param cols The number of tile columns
      /// \param row_size The number of element rows
      /// \param col_size The number of element columns
      ProcGrid(madness::World& world, const size_type test_rank, size_type test_nprocs,
          const size_type rows, const size_type cols,
          const std::size_t row_size, const std::size_t col_size) :
        world_(world), rows_(rows), cols_(cols), size_(rows_ * cols_),
        proc_rows_(0ul), proc_cols_(0ul), proc_size_(0ul), rank_row_(-1),
        rank_col_(-1), local_rows_(0ul), local_cols_(0ul), local_size_(0ul)
      {
        // Check for non-zero sizes
        TA_ASSERT(rows >= 1ul);
        TA_ASSERT(cols >= 1ul);
        TA_ASSERT(row_size >= 1ul);
        TA_ASSERT(col_size >= 1ul);
        TA_ASSERT(test_rank < test_nprocs);

        init(test_rank, test_nprocs, row_size, col_size);
      }
#endif // TILEDARRAY_ENABLE_TEST_PROC_GRID

      /// Copy constructor

      // This constructor makes a rough estimate of the optimal process
      // dimensions. The goal is for the ratios of proc_rows/proc_cols and
      // rows/cols to be approximately equal.
      /// \param other The other process grid to be copied
      ProcGrid(const ProcGrid& other) :
        world_(other.world_), rows_(other.rows_), cols_(other.cols_),
        size_(other.size_), proc_rows_(other.proc_rows_),
        proc_cols_(other.proc_cols_), proc_size_(other.proc_size_),
        rank_row_(other.rank_row_), rank_col_(other.rank_col_),
        local_rows_(other.local_rows_), local_cols_(other.local_cols_),
        local_size_(other.local_size_)
      { }

      /// Element row count accessor

      /// \return The number of element rows
      size_type rows() const { return rows_; }

      /// Element column count accessor

      /// \return The number of element columns
      size_type cols() const { return cols_; }

      /// Element count accessor

      /// \return The number of element
      size_type size() const { return size_; }

      /// Local element row count accessor

      /// \return The number of element rows
      size_type local_rows() const { return local_rows_; }

      /// Local element column count accessor

      /// \return The number of element columns
      size_type local_cols() const { return local_cols_; }

      /// Local element count accessor

      /// \return The number of element
      size_type local_size() const { return local_size_; }

      /// Rank row accessor

      /// \return The row of this process in the process grid
      ProcessID rank_row() const { return rank_row_; }

      /// Rank row accessor

      /// \return The column of this process in the process grid
      ProcessID rank_col() const { return rank_col_; }

      /// Process row count accessor

      /// \return The number of rows in the process grid
      size_type proc_rows() const { return proc_rows_; }

      /// Process column count accessor

      /// \return The number of columns in the process grid
      size_type proc_cols() const { return proc_cols_; }

      /// Process grid size accessor

      /// \return The number of processes included in the process grid (may be
      /// less than the number of process in world).
      size_type proc_size() const { return proc_size_; }

      /// Construct a row group

      /// Construct a row group that is filtered by a shape
      /// \param Shape The shape type
      /// \param did The distributed id for the result group
      /// \param shape The shape that will be used to filter the group
      /// \param row The column of shape to be
      /// \param size The number of elements in the shape
      /// \return A \c Group object that includes all processes in \c rank_col
      template <typename Shape>
      madness::Group make_row_group(const madness::DistributedID& did,
          const Shape& shape, const size_type row, const size_type size) const
      {
        // Check that row and size are compatible with this process grid.
        TA_ASSERT((size % cols_) == 0ul);
        TA_ASSERT(row < (size / cols_));

        madness::Group group;

        if(local_size_ != 0) {
          // Generate the list of processes in rank_row
          std::vector<ProcessID> proc_list(proc_cols_, 0);

          // Flag all process that have non-zero tiles
          size_type i_start = row * cols_;
          const size_type i_end = i_start + cols_;
          for(size_type i = i_start; i < i_end; ++i)
            if(!shape.is_zero(i))
              proc_list[(i - i_start) % proc_cols_] = 1;

          // Convert flags into processes
          size_type x = 0u;
          size_type p_start = rank_row_ * proc_cols_;
          for(size_type p = 0u; p < proc_cols_; ++p) {
            if(proc_list[p]) {
              proc_list[x] = p_start + p;
              ++x;
            }
          }

          // Truncate the process list
          proc_list.resize(x);

          if(proc_list.size() != 0)
            group = madness::Group(world_, proc_list, did);
        }

        return group;
      }

      /// Construct a column group

      /// \param Shape The shape type
      /// \param did The distributed id for the result group group.
      /// \param shape The shape that will be used to filter the group
      /// \param col The column of shape to be
      /// \return A \c Group object that includes all processes in \c rank_col
      template <typename Shape>
      madness::Group make_col_group(const madness::DistributedID& did,
          const Shape& shape, const size_type col, const size_type size) const
      {
        // Check that col and size are compatible with this process grid.
        TA_ASSERT((size % rows_) == 0u);
        TA_ASSERT(col < (size / rows_));

        madness::Group group;

        if(local_size_ != 0u) {
          std::vector<ProcessID> proc_list;
          proc_list.reserve(proc_rows_);

          // Populate the process column group
          // Note: This algorithm is slow due to the non-unit stride memory
          // access pattern, but there is nothing that can be done to change it.
          size_type i_start = col;
          const size_type i_start_step = size / rows_;
          const size_type i_step = i_start_step * proc_rows_;
          for(size_type p = rank_col_; p < proc_size_; p += proc_cols_) {
            for(size_type i = i_start; i < size; i += i_step) {
              if(! shape.is_zero(i)) {
                proc_list.push_back(p);
                break;
              }
            }

            i_start += i_start_step;
          }

          // Construct the group if the process list size
          if(proc_list.size() != 0u)
            group = madness::Group(world_, proc_list, did);
        }

        return group;
      }

      /// Construct a row group

      /// \param did The distributed id for the result group
      /// \param pred A predicate that selects processes to be included in the
      /// group.
      /// \return A \c Group object that includes all processes in \c rank_row
      madness::Group make_row_group(const madness::DistributedID& did) const {
        madness::Group group;

        if(local_size_ != 0u) {
          // Construct a vector to hold the
          std::vector<ProcessID> proc_list;
          proc_list.reserve(proc_cols_);

          // Populate the row process list
          size_type p = rank_row_ * proc_cols_;
          const size_type row_end = p + proc_cols_;
          for(; p < row_end; ++p)
            proc_list.push_back(p);

          // Construct the group
          group = madness::Group(world_, proc_list, did);
        }

        return group;
      }

      /// Construct a column group

      /// \param did The distributed id for the result group
      /// \param pred A predicate that selects processes to be included in the
      /// group.
      /// \return A \c Group object that includes all processes in \c rank_col
      madness::Group make_col_group(const madness::DistributedID& did) const {
        madness::Group group;

        if(local_size_ != 0u) {
          // Generate the list of processes in rank_row
          std::vector<ProcessID> proc_list;
          proc_list.reserve(proc_rows_);

          // Populate the column process list
          for(size_type p = rank_col_; p < proc_size_; p += proc_cols_)
            proc_list.push_back(p);

          // Construct the group
          if(proc_list.size() != 0)
            group = madness::Group(world_, proc_list, did);
        }

        return group;
      }


      /// Construct column phased a cyclic process

      /// Construct a cyclic process map where the column phase of the process
      /// matches that of this process grid.
      /// \param rows The number of rows in the process map
      /// \return Cyclic process map with matching column phase
      std::shared_ptr<Pmap> make_col_phase_pmap(const size_type rows) const {
        return std::shared_ptr<Pmap>(new CyclicPmap(world_, rows, cols_, proc_rows_, proc_cols_));
      }

      /// Construct row phased a cyclic process

      /// Construct a cyclic process map where the column phase of the process
      /// matches that of this process grid.
      /// \param cols The number of columns in the process map
      /// \return Cyclic process map with matching column phase
      std::shared_ptr<Pmap> make_row_phase_pmap(const size_type cols) const {
        return std::shared_ptr<Pmap>(new CyclicPmap(world_, rows_, cols, proc_rows_, proc_cols_));
      }
    }; // class Grid

  } // namespace detail
} // namespace TiledArray

#endif // TILEDARRAY_GRID_H__INCLUDED