/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PUBLISHER_PREFIX_ITERATOR_H_
#define BRAVELEDGER_PUBLISHER_PREFIX_ITERATOR_H_

#include <iterator>

#include "base/strings/string_piece.h"

namespace braveledger_publisher {

using PrefixIteratorBase =
  std::iterator<std::random_access_iterator_tag, base::StringPiece>;

// A random-access iterator over prefixes stored in an
// uncompressed prefix list, suitable for binary search
class PrefixIterator : public PrefixIteratorBase {
 public:
  using difference_type = PrefixIteratorBase::difference_type;

  PrefixIterator(const char* data, size_t index, size_t size)
      : data_(data), index_(index), size_(size) {}

  PrefixIterator(const PrefixIterator& rhs)
      : data_(rhs.data_), index_(rhs.index_), size_(rhs.size_) {}

  base::StringPiece operator*() const {
    size_t offset = index_ * size_;
    return base::StringPiece(data_ + offset, size_);
  }

  base::StringPiece operator[](const int& rhs) const {
    size_t offset = (index_ + rhs) * size_;
    return base::StringPiece(data_ + offset, size_);
  }

  PrefixIterator& operator=(const PrefixIterator& rhs) {
    index_ = rhs.index_;
    return *this;
  }

  PrefixIterator& operator+=(const int& rhs) {
    index_ += rhs;
    return *this;
  }

  PrefixIterator& operator-=(const int& rhs) {
    index_ -= rhs;
    return *this;
  }

  PrefixIterator& operator++() {
    index_++;
    return *this;
  }

  PrefixIterator& operator--() {
    index_--;
    return *this;
  }

  PrefixIterator operator+(const PrefixIterator& rhs) const {
    return PrefixIterator(data_, index_ + rhs.index_, size_);
  }

  difference_type operator-(const PrefixIterator& rhs) const {
    return index_ - rhs.index_;
  }

  PrefixIterator operator+(const int& rhs) const {
    return PrefixIterator(data_, index_ + rhs, size_);
  }

  PrefixIterator operator-(const int& rhs) const {
    return PrefixIterator(data_, index_ - rhs, size_);
  }

  bool operator==(const PrefixIterator& rhs) const {
    return index_ == rhs.index_;
  }

  bool operator!=(const PrefixIterator& rhs) const {
    return index_ != rhs.index_;
  }

  bool operator>(const PrefixIterator& rhs) const {
    return index_ > rhs.index_;
  }

  bool operator<(const PrefixIterator& rhs) const {
    return index_ < rhs.index_;
  }

  bool operator>=(const PrefixIterator& rhs) const {
    return index_ >= rhs.index_;
  }

  bool operator<=(const PrefixIterator& rhs) const {
    return index_ <= rhs.index_;
  }

 private:
  const char* data_;
  size_t index_;
  size_t size_;
};

}  // namespace braveledger_publisher

#endif  // BRAVELEDGER_PUBLISHER_PREFIX_ITERATOR_H_
