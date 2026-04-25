// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_CSV_SAFARI_PASSWORD_H_
#define BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_CSV_SAFARI_PASSWORD_H_

#include <stddef.h>

#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "base/types/expected.h"
#include "url/gurl.h"

namespace password_manager {

class CSVSafariPassword {
 public:
  enum class Label { kTitle, kURL, kUsername, kPassword, kNotes, kOTPAuthURL };
  using ColumnMap = base::flat_map<size_t, Label>;

  // Status describes parsing errors.
  enum class Status {
    kOK = 0,
    kSyntaxError = 1,
    kSemanticError = 2,
  };

  CSVSafariPassword();
  explicit CSVSafariPassword(const ColumnMap& map, std::string_view csv_row);
  explicit CSVSafariPassword(std::string title,
                             GURL url,
                             std::string username,
                             std::string password,
                             std::string notes,
                             GURL otp_auth_url,
                             Status status);
  // This constructor creates a valid CSVPassword but with an invalid_url, i.e.
  // the url is not a valid GURL.
  explicit CSVSafariPassword(std::string title,
                             std::string invalid_url,
                             std::string username,
                             std::string password,
                             std::string note,
                             GURL otp_auth_url,
                             Status status);
  CSVSafariPassword(const CSVSafariPassword&);
  CSVSafariPassword(CSVSafariPassword&&);
  CSVSafariPassword& operator=(const CSVSafariPassword&);
  CSVSafariPassword& operator=(CSVSafariPassword&&);
  ~CSVSafariPassword();

  friend bool operator==(const CSVSafariPassword&,
                         const CSVSafariPassword&) = default;

  Status GetParseStatus() const;

  const std::string& GetTitle() const;

  const std::string& GetPassword() const;

  const std::string& GetUsername() const;

  const std::string& GetNotes() const;

  const base::expected<GURL, std::string>& GetURL() const;

  const base::expected<GURL, std::string>& GetOTPAuthURL() const;

 private:
  std::string title_;
  base::expected<GURL, std::string> url_ = base::unexpected("");
  std::string username_;
  std::string password_;
  std::string notes_;
  base::expected<GURL, std::string> otp_auth_url_ = base::unexpected("");

  Status status_;
};

}  // namespace password_manager

#endif  // BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_CSV_SAFARI_PASSWORD_H_
