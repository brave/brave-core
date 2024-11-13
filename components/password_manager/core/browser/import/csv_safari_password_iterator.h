// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_CSV_SAFARI_PASSWORD_ITERATOR_H_
#define BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_CSV_SAFARI_PASSWORD_ITERATOR_H_

#include <stddef.h>

#include <iterator>
#include <optional>
#include <string_view>

#include "base/memory/raw_ptr.h"
#include "brave/components/password_manager/core/browser/import/csv_safari_password.h"

namespace password_manager {

class CSVSafariPasswordIterator {
 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = CSVSafariPassword;
  using difference_type = std::ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;

  CSVSafariPasswordIterator();
  explicit CSVSafariPasswordIterator(const CSVSafariPassword::ColumnMap& map,
                                     std::string_view csv);
  CSVSafariPasswordIterator(const CSVSafariPasswordIterator&);
  CSVSafariPasswordIterator& operator=(const CSVSafariPasswordIterator&);
  ~CSVSafariPasswordIterator();

  reference operator*() const {
    DCHECK(password_);
    return *password_;
  }
  pointer operator->() const { return &**this; }

  CSVSafariPasswordIterator& operator++();
  CSVSafariPasswordIterator operator++(int);
  bool operator==(const CSVSafariPasswordIterator& other) const;

 private:
  void SeekToNextValidRow();

  raw_ptr<const CSVSafariPassword::ColumnMap> map_ = nullptr;
  std::string_view csv_rest_;
  std::string_view csv_row_;
  std::optional<CSVSafariPassword> password_;
};

}  // namespace password_manager

#endif  // BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_CSV_SAFARI_PASSWORD_ITERATOR_H_
