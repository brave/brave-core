// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_CSV_SAFARI_PASSWORD_SEQUENCE_H_
#define BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_CSV_SAFARI_PASSWORD_SEQUENCE_H_

#include <string>
#include <string_view>

#include "brave/components/password_manager/core/browser/import/csv_safari_password.h"
#include "brave/components/password_manager/core/browser/import/csv_safari_password_iterator.h"

namespace password_manager {

class CSVSafariPasswordSequence {
 public:
  explicit CSVSafariPasswordSequence(std::string csv);
  CSVSafariPasswordSequence(const CSVSafariPasswordSequence&) = delete;
  CSVSafariPasswordSequence& operator=(const CSVSafariPasswordSequence&) =
      delete;

  CSVSafariPasswordSequence(CSVSafariPasswordSequence&&);
  CSVSafariPasswordSequence& operator=(CSVSafariPasswordSequence&&);

  ~CSVSafariPasswordSequence();

  CSVSafariPasswordIterator begin() const;
  CSVSafariPasswordIterator end() const;

  CSVSafariPassword::Status result() const { return result_; }

 private:
  std::string csv_;
  CSVSafariPassword::ColumnMap map_;
  std::string_view data_rows_;
  CSVSafariPassword::Status result_ = CSVSafariPassword::Status::kOK;
};

}  // namespace password_manager

#endif  // BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_CSV_SAFARI_PASSWORD_SEQUENCE_H_
