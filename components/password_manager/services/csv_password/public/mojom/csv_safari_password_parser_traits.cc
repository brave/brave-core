// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/password_manager/services/csv_password/public/mojom/csv_safari_password_parser_traits.h"

#include "url/mojom/url_gurl_mojom_traits.h"

namespace mojo {

password_manager::mojom::CSVSafariPassword_Status
EnumTraits<password_manager::mojom::CSVSafariPassword_Status,
           password_manager::CSVSafariPassword::Status>::
    ToMojom(password_manager::CSVSafariPassword::Status status) {
  switch (status) {
    case password_manager::CSVSafariPassword::Status::kOK:
      return password_manager::mojom::CSVSafariPassword_Status::kOK;
    case password_manager::CSVSafariPassword::Status::kSyntaxError:
      return password_manager::mojom::CSVSafariPassword_Status::kSyntaxError;
    case password_manager::CSVSafariPassword::Status::kSemanticError:
      return password_manager::mojom::CSVSafariPassword_Status::kSemanticError;
  }
  NOTREACHED();
}

bool EnumTraits<password_manager::mojom::CSVSafariPassword_Status,
                password_manager::CSVSafariPassword::Status>::
    FromMojom(password_manager::mojom::CSVSafariPassword_Status status,
              password_manager::CSVSafariPassword::Status* out) {
  switch (status) {
    case password_manager::mojom::CSVSafariPassword_Status::kOK:
      *out = password_manager::CSVSafariPassword::Status::kOK;
      return true;
    case password_manager::mojom::CSVSafariPassword_Status::kSyntaxError:
      *out = password_manager::CSVSafariPassword::Status::kSyntaxError;
      return true;
    case password_manager::mojom::CSVSafariPassword_Status::kSemanticError:
      *out = password_manager::CSVSafariPassword::Status::kSemanticError;
      return true;
  }
  return false;
}

// static
bool StructTraits<password_manager::mojom::CSVSafariPasswordDataView,
                  password_manager::CSVSafariPassword>::
    Read(password_manager::mojom::CSVSafariPasswordDataView data,
         password_manager::CSVSafariPassword* out) {
  password_manager::CSVSafariPassword::Status status;
  GURL url;
  std::string title;
  std::string username;
  std::string password;
  GURL otp_auth_url;
  std::string notes;

  if (!data.ReadStatus(&status)) {
    return false;
  }
  if (!data.ReadTitle(&title)) {
    return false;
  }
  if (!data.ReadUrl(&url)) {
    return false;
  }
  if (!data.ReadUsername(&username)) {
    return false;
  }
  if (!data.ReadPassword(&password)) {
    return false;
  }
  if (!data.ReadNotes(&notes)) {
    return false;
  }
  if (!data.ReadOtpAuthUrl(&otp_auth_url)) {
    return false;
  }
  if (url.is_valid()) {
    *out = password_manager::CSVSafariPassword(title, url, username, password,
                                               notes, otp_auth_url, status);
    return true;
  }
  std::optional<std::string> invalid_url;
  if (!data.ReadInvalidUrl(&invalid_url)) {
    return false;
  }
  DCHECK(invalid_url.has_value());
  *out = password_manager::CSVSafariPassword(title, invalid_url.value(),
                                             username, password, notes,
                                             otp_auth_url, status);
  return true;
}

}  // namespace mojo
