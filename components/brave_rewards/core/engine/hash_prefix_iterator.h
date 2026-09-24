/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_HASH_PREFIX_ITERATOR_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_HASH_PREFIX_ITERATOR_H_

#include <iterator>
#include <string_view>

namespace brave_rewards::internal {

// A random-access iterator over prefixes stored in an uncompressed prefix list,
// suitable for binary search.
class HashPrefixIterator {
 public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::string_view;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;

  HashPrefixIterator(std::string_view data, size_t index, size_t size)
      : data_(data), index_(index), size_(size) {}

  HashPrefixIterator(const HashPrefixIterator& rhs)
      : data_(rhs.data_), index_(rhs.index_), size_(rhs.size_) {}

  std::string_view operator*() const { return PrefixAt(index_); }

  std::string_view operator[](const int& rhs) const {
    return PrefixAt(index_ + rhs);
  }

  HashPrefixIterator& operator=(const HashPrefixIterator& rhs) {
    index_ = rhs.index_;
    return *this;
  }

  HashPrefixIterator& operator+=(const int& rhs) {
    index_ += rhs;
    return *this;
  }

  HashPrefixIterator& operator-=(const int& rhs) {
    index_ -= rhs;
    return *this;
  }

  HashPrefixIterator& operator++() {
    index_++;
    return *this;
  }

  HashPrefixIterator& operator--() {
    index_--;
    return *this;
  }

  HashPrefixIterator operator+(const HashPrefixIterator& rhs) const {
    return HashPrefixIterator(data_, index_ + rhs.index_, size_);
  }

  difference_type operator-(const HashPrefixIterator& rhs) const {
    return index_ - rhs.index_;
  }

  HashPrefixIterator operator+(const int& rhs) const {
    return HashPrefixIterator(data_, index_ + rhs, size_);
  }

  HashPrefixIterator operator-(const int& rhs) const {
    return HashPrefixIterator(data_, index_ - rhs, size_);
  }

  bool operator==(const HashPrefixIterator& rhs) const {
    return index_ == rhs.index_;
  }

  bool operator>(const HashPrefixIterator& rhs) const {
    return index_ > rhs.index_;
  }

  bool operator<(const HashPrefixIterator& rhs) const {
    return index_ < rhs.index_;
  }

  bool operator>=(const HashPrefixIterator& rhs) const {
    return index_ >= rhs.index_;
  }

  bool operator<=(const HashPrefixIterator& rhs) const {
    return index_ <= rhs.index_;
  }

 private:
  std::string_view PrefixAt(size_t index) const {
    return data_.substr(index_ * size_, size_);
  }

  const std::string_view data_;
  size_t index_;
  const size_t size_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_HASH_PREFIX_ITERATOR_H_
