// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_CSV_SAFARI_PASSWORD_SEQUENCE_H_
#define BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_CSV_SAFARI_PASSWORD_SEQUENCE_H_

#include <optional>
#include <string_view>

#include "base/compiler_specific.h"
#include "base/types/expected.h"
#include "brave/components/password_manager/core/browser/import/csv_safari_password.h"

namespace password_manager {

class CSVSafariPasswordSequence {
 public:
  // Constructs a CSVSafariPasswordSequence over |csv|, or returns the header
  // parse error status.
  static base::expected<CSVSafariPasswordSequence, CSVSafariPassword::Status>
  Create(std::string_view csv LIFETIME_BOUND);

  CSVSafariPasswordSequence(const CSVSafariPasswordSequence&) = delete;
  CSVSafariPasswordSequence& operator=(const CSVSafariPasswordSequence&) =
      delete;

  CSVSafariPasswordSequence(CSVSafariPasswordSequence&&);
  CSVSafariPasswordSequence& operator=(CSVSafariPasswordSequence&&);

  ~CSVSafariPasswordSequence();

  // Parses and returns the next non-empty credential row, or std::nullopt when
  // the end of the CSV data is reached.
  std::optional<CSVSafariPassword> NextPassword();

 private:
  CSVSafariPasswordSequence(CSVSafariPassword::ColumnMap column_map,
                            std::string_view remaining_rows LIFETIME_BOUND);

  CSVSafariPassword::ColumnMap column_map_;
  std::string_view remaining_rows_;
};

}  // namespace password_manager

#endif  // BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_CSV_SAFARI_PASSWORD_SEQUENCE_H_
