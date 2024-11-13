// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/password_manager/core/browser/import/csv_safari_password.h"

#include <string_view>
#include <utility>

#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "components/affiliations/core/browser/affiliation_utils.h"
#include "components/password_manager/core/browser/form_parsing/form_data_parser.h"
#include "components/password_manager/core/browser/import/csv_field_parser.h"
#include "url/gurl.h"

namespace password_manager {

namespace {

std::string ConvertUTF8(std::string_view str) {
  std::string str_copy(str);
  base::ReplaceSubstringsAfterOffset(&str_copy, 0, "\"\"", "\"");
  return str_copy;
}

}  // namespace

CSVSafariPassword::CSVSafariPassword() : status_(Status::kSemanticError) {}

CSVSafariPassword::CSVSafariPassword(std::string title,
                                     GURL url,
                                     std::string username,
                                     std::string password,
                                     std::string notes,
                                     GURL otp_auth_url,
                                     Status status)
    : title_(title),
      url_(std::move(url)),
      username_(std::move(username)),
      password_(std::move(password)),
      notes_(std::move(notes)),
      otp_auth_url_(otp_auth_url),
      status_(status) {}

CSVSafariPassword::CSVSafariPassword(std::string title,
                                     std::string invalid_url,
                                     std::string username,
                                     std::string password,
                                     std::string notes,
                                     GURL otp_auth_url,
                                     Status status)
    : title_(title),
      url_(base::unexpected(std::move(invalid_url))),
      username_(std::move(username)),
      password_(std::move(password)),
      notes_(std::move(notes)),
      otp_auth_url_(otp_auth_url),
      status_(status) {}

CSVSafariPassword::CSVSafariPassword(const ColumnMap& map,
                                     std::string_view row) {
  if (row.empty()) {
    status_ = Status::kSemanticError;
    return;
  }

  size_t field_idx = 0;
  CSVFieldParser parser(row);
  status_ = Status::kOK;

  while (parser.HasMoreFields()) {
    std::string_view field;
    if (!parser.NextField(&field)) {
      status_ = Status::kSyntaxError;
      return;
    }
    auto meaning_it = map.find(field_idx++);
    if (meaning_it == map.end()) {
      continue;
    }
    switch (meaning_it->second) {
      case Label::kTitle:
        title_ = ConvertUTF8(field);
        break;
      case Label::kURL: {
        GURL gurl = GURL(field);
        if (!gurl.is_valid()) {
          url_ = base::unexpected(ConvertUTF8(field));
        } else {
          url_ = gurl;
        }
        break;
      }
      case Label::kUsername:
        username_ = ConvertUTF8(field);
        break;
      case Label::kPassword:
        password_ = ConvertUTF8(field);
        break;
      case Label::kNotes:
        notes_ = ConvertUTF8(field);
        break;
      case Label::kOTPAuthURL: {
        GURL gurl = GURL(field);
        if (!gurl.is_valid()) {
          otp_auth_url_ = base::unexpected(ConvertUTF8(field));
        } else {
          otp_auth_url_ = gurl;
        }
        break;
      }
    }
  }
}

CSVSafariPassword::CSVSafariPassword(const CSVSafariPassword&) = default;
CSVSafariPassword::CSVSafariPassword(CSVSafariPassword&&) = default;
CSVSafariPassword& CSVSafariPassword::operator=(const CSVSafariPassword&) =
    default;
CSVSafariPassword& CSVSafariPassword::operator=(CSVSafariPassword&&) = default;
CSVSafariPassword::~CSVSafariPassword() = default;

const std::string& CSVSafariPassword::GetTitle() const {
  return title_;
}

CSVSafariPassword::Status CSVSafariPassword::GetParseStatus() const {
  return status_;
}

const std::string& CSVSafariPassword::GetPassword() const {
  return password_;
}

const std::string& CSVSafariPassword::GetUsername() const {
  return username_;
}

const std::string& CSVSafariPassword::GetNotes() const {
  return notes_;
}

const base::expected<GURL, std::string>& CSVSafariPassword::GetURL() const {
  return url_;
}

const base::expected<GURL, std::string>& CSVSafariPassword::GetOTPAuthURL()
    const {
  return otp_auth_url_;
}

}  // namespace password_manager
