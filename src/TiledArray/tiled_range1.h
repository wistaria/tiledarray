#ifndef RANGE1_H__INCLUDED
#define RANGE1_H__INCLUDED

#include <TiledArray/range.h>
//#include <TiledArray/coordinates.h>
//#include <cassert>
#include <vector>
//#include <cstddef>
//#include <iosfwd>

namespace TiledArray {

  template <typename I>
  class TiledRange1;
  template <typename I>
  void swap(TiledRange1<I>&, TiledRange1<I>&);
  template<typename I>
  Range<I, 1, LevelTag<1>, CoordinateSystem<1> > make_range1(const I&, const I&);
  template<typename I>
  Range<I, 1, LevelTag<0>, CoordinateSystem<1> > make_tile_range1(const I&, const I&);
  template <typename I>
  bool operator ==(const TiledRange1<I>&, const TiledRange1<I>&);
  template <typename I>
  bool operator !=(const TiledRange1<I>&, const TiledRange1<I>&);
  template<typename I>
  Range<I, 1, LevelTag<1>, CoordinateSystem<1> > make_range1(const I&, const I&);
  template<typename I>
  Range<I, 1, LevelTag<0>, CoordinateSystem<1> > make_tile_range1(const I&, const I&);
  template <typename I>
  std::ostream& operator <<(std::ostream&, const TiledRange1<I>&);

  /// TiledRange1 class defines a non-uniformly-tiled, continuous, one-dimensional
  /// range. The tiling data is constructed with and stored in an array with
  /// the format {a0, a1, a2, ...}, where 0 <= a0 < a1 < a2 < ... Each tile is
  /// defined as [a0,a1), [a1,a2), ... The number of tiles in the range will be
  /// equal to one less than the number of elements in the array.
  template <typename I>
  class TiledRange1 {
  public:
    typedef Range<I, 1, LevelTag<1>, CoordinateSystem<1> > range_type;
    typedef Range<I, 1, LevelTag<0>,  CoordinateSystem<1> > tile_range_type;
    typedef I index_type;
    typedef I tile_index_type;
    typedef typename std::vector<tile_range_type>::const_iterator const_iterator;

  private:
    typedef typename range_type::index_type range_index;
    typedef typename tile_range_type::index_type tile_range_index;

  public:

    /// Default constructor, range of 0 tiles and elements.
    TiledRange1() : range_(make_range1<index_type>(0,0)), element_range_(make_tile_range1<index_type>(0,0)),
        tile_ranges_(1, make_tile_range1<index_type>(0,0)), elem2tile_(1, 0)
    {
      init_map_();
    }

    /// Constructs a range with the boundaries provided by [first, last).
    /// Start_tile_index is the index of the first tile.
    template <typename RandIter>
    TiledRange1(RandIter first, RandIter last, const index_type start_tile_index = 0) :
        range_(), element_range_(), tile_ranges_(), elem2tile_()
    {
      BOOST_STATIC_ASSERT(detail::is_random_iterator<RandIter>::value);
      init_tiles_(first, last, start_tile_index);
      init_map_();
    }

    /// Copy constructor
    TiledRange1(const TiledRange1& rng) : range_(rng.range_), element_range_(rng.element_range_),
      tile_ranges_(rng.tile_ranges_), elem2tile_(rng.elem2tile_)
    { }

    /// Assignment operator
    TiledRange1& operator =(const TiledRange1& rng) {
      TiledRange1 temp(rng);
      swap(*this, temp);
      return *this;
    }

    template <typename RandIter>
    TiledRange1& resize(RandIter first, RandIter last, const index_type start_tile_index = 0) {
      BOOST_STATIC_ASSERT(detail::is_random_iterator<RandIter>::value);
      TiledRange1 temp(first, last, start_tile_index);
      swap(*this, temp);
      return *this;
    }

    /// Returns an iterator to the first tile in the range.
    const_iterator begin() const { return tile_ranges_.begin(); }

    /// Returns an iterator to the end of the range.
    const_iterator end() const { return tile_ranges_.end(); }

    /// Return tile iterator associated with tile_index_type
    const_iterator find(const tile_index_type& e) const{
      if(! element_range_.includes(tile_range_index(e)))
        return tile_ranges_.end();
      const_iterator result = tile_ranges_.begin();
      result += element2tile(e);
      return result;
    }

    const range_type& tiles() const { return range_; }
    const tile_range_type& elements() const { return element_range_; }
    const tile_range_type& tile(const index_type& i) {
      return tile_ranges_.at(i - range_.start()[0]);
    }

    const index_type& element2tile(const tile_index_type& e) const {
      TA_ASSERT( element_range_.includes(tile_range_index(e)) ,
          std::out_of_range, "Element index is out of range.");
      std::size_t i = e - element_range_.start()[0];
      return elem2tile_[i];
    }

  private:

    /// Validates tile_boundaries
    template <typename RandIter>
    static bool valid_(RandIter first, RandIter last) {
      // Verify at least 2 elements are present if the vector is not empty.
      if((last - first) == 2)
        return false;
      // Verify the requirement that a0 < a1 < a2 < ...
      for (; first != (last - 1); ++first)
        if(*first >= *(first + 1))
          return false;
      return true;
    }

    /// Initialize tiles use a set of tile offsets
    template <typename RandIter>
    void init_tiles_(RandIter first, RandIter last, index_type start_tile_index) {
      TA_ASSERT( valid_(first, last) , std::runtime_error,
          "Tile boundaries do not have the expected structure.");
      range_.resize(range_index(start_tile_index), range_index(start_tile_index + last - first - 1));
      element_range_.resize(tile_range_index(*first), tile_range_index(*(last - 1)));
      for (; first != (last - 1); ++first)
        tile_ranges_.push_back(make_tile_range1<I>(*first, *(first + 1)));
    }

    /// Initialize secondary data
    void init_map_() {
      // check for 0 size range.
      if(element_range_.size()[0] == 0)
        return;

      // initialize elem2tile map
      elem2tile_.resize(element_range_.size()[0]);
      for(index_type t = 0; t < range_.size()[0]; ++t)
        for(tile_index_type e = tile_ranges_[t].start()[0]; e < tile_ranges_[t].finish()[0]; ++e)
          elem2tile_[e - element_range_.start()[0]] = t + range_.start()[0];
    }

    friend void swap<>(TiledRange1<I>&, TiledRange1<I>&);
    friend std::ostream& operator << <>(std::ostream&, const TiledRange1&);

    // TiledRange1 data
    range_type range_; ///< stores the overall dimensions of the tiles.
    tile_range_type element_range_; ///< stores overall element dimensions.
    std::vector<tile_range_type> tile_ranges_; ///< stores the dimensions of each tile.
    std::vector<index_type> elem2tile_; ///< maps element index to tile index (secondary data).

  }; // class TiledRange1

  /// Exchange the data of the two given ranges.
  template <typename I>
  void swap(TiledRange1<I>& r0, TiledRange1<I>& r1) { // no throw
    TiledArray::swap(r0.range_, r1.range_);
    TiledArray::swap(r0.element_range_, r1.element_range_);
    std::swap(r0.tile_ranges_, r1.tile_ranges_);
    std::swap(r0.elem2tile_, r1.elem2tile_);
  }

  /// Equality operator
  template <typename I>
  bool operator ==(const TiledRange1<I>& r1, const TiledRange1<I>& r2) {
    return std::equal(r1.begin(), r1.end(), r2.begin()) &&
        (r1.tiles() == r2.tiles()) && (r1.elements() == r2.elements());
  }

  /// Inequality operator
  template <typename I>
  bool operator !=(const TiledRange1<I>& r1, const TiledRange1<I>& r2){
    return ! operator ==(r1, r2);
  }

  template<typename I>
  Range<I, 1, LevelTag<1>, CoordinateSystem<1> > make_range1(const I& s, const I& f) {
    typedef Range<I, 1, LevelTag<1>, CoordinateSystem<1> > range_type;
    typedef typename range_type::index_type index_type;

    return range_type(index_type(s), index_type(f));
  }

  template<typename I>
  Range<I, 1, LevelTag<0>, CoordinateSystem<1> > make_tile_range1(const I& s, const I& f) {
    typedef Range<I, 1, LevelTag<0>, CoordinateSystem<1> > range_type;
    typedef typename range_type::index_type index_type;

    return range_type(index_type(s), index_type(f));
  }

  /// TiledRange1 ostream operator
  template <typename I>
  std::ostream& operator <<(std::ostream& out, const TiledRange1<I>& rng) {
    out << "( tiles = [ " << rng.tiles().start()[0] << ", " << rng.tiles().finish()[0]
        << " ), elements = [ " << rng.elements().start()[0] << ", " << rng.elements().finish()[0] << " ) )";
    return out;
  }

} // namespace TiledArray

#endif // RANGE1_H__INCLUDED