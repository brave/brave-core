// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/password_manager/core/browser/import/csv_safari_password_iterator.h"

#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "components/password_manager/core/browser/import/csv_password_iterator.h"

namespace password_manager {

namespace {

// Takes the |rest| of the CSV lines, returns the first one and stores the
// remaining ones back in |rest|.
std::string_view ExtractFirstRow(std::string_view* rest) {
  DCHECK(rest);
  if (!rest->empty()) {
    return ConsumeCSVLine(rest);
  }
  return std::string_view();
}

}  // namespace

CSVSafariPasswordIterator::CSVSafariPasswordIterator() = default;

CSVSafariPasswordIterator::CSVSafariPasswordIterator(
    const CSVSafariPassword::ColumnMap& map,
    std::string_view csv)
    : map_(&map), csv_rest_(csv) {
  SeekToNextValidRow();
}

CSVSafariPasswordIterator::CSVSafariPasswordIterator(
    const CSVSafariPasswordIterator& other) {
  *this = other;
}

CSVSafariPasswordIterator& CSVSafariPasswordIterator::operator=(
    const CSVSafariPasswordIterator& other) {
  map_ = other.map_;
  csv_rest_ = other.csv_rest_;
  csv_row_ = other.csv_row_;
  if (map_) {
    password_.emplace(*map_, csv_row_);
  } else {
    password_.reset();
  }
  return *this;
}

CSVSafariPasswordIterator::~CSVSafariPasswordIterator() = default;

CSVSafariPasswordIterator& CSVSafariPasswordIterator::operator++() {
  SeekToNextValidRow();
  return *this;
}

CSVSafariPasswordIterator CSVSafariPasswordIterator::operator++(int) {
  CSVSafariPasswordIterator old = *this;
  ++*this;
  return old;
}

bool CSVSafariPasswordIterator::operator==(
    const CSVSafariPasswordIterator& other) const {
  // There is no need to compare |password_|, because it is determined by |map_|
  // and |csv_row_|.

  return csv_row_ == other.csv_row_ && csv_rest_ == other.csv_rest_ &&
         // The column map should reference the same map if the iterators come
         // from the same sequence, and iterators from different sequences are
         // not considered equal. Therefore the maps' addresses are checked
         // instead of their contents.
         map_ == other.map_;
}

void CSVSafariPasswordIterator::SeekToNextValidRow() {
  DCHECK(map_);
  do {
    csv_row_ = base::TrimString(ExtractFirstRow(&csv_rest_), "\r \t",
                                base::TRIM_LEADING);
  } while (
      // Skip over empty lines.
      csv_row_.empty() && !csv_rest_.empty());
  password_.emplace(*map_, csv_row_);
}

}  // namespace password_manager
