// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PASSWORD_MANAGER_SERVICES_CSV_PASSWORD_PUBLIC_MOJOM_CSV_SAFARI_PASSWORD_PARSER_TRAITS_H_
#define BRAVE_COMPONENTS_PASSWORD_MANAGER_SERVICES_CSV_PASSWORD_PUBLIC_MOJOM_CSV_SAFARI_PASSWORD_PARSER_TRAITS_H_

#include <optional>
#include <string>

#include "base/types/expected.h"
#include "base/types/expected_macros.h"
#include "brave/components/password_manager/core/browser/import/csv_safari_password.h"
#include "brave/components/password_manager/services/csv_password/public/mojom/csv_safari_password_parser.mojom.h"
#include "mojo/public/cpp/bindings/struct_traits.h"

namespace mojo {

template <>
struct EnumTraits<password_manager::mojom::CSVSafariPassword_Status,
                  password_manager::CSVSafariPassword::Status> {
  static password_manager::mojom::CSVSafariPassword_Status ToMojom(
      password_manager::CSVSafariPassword::Status status);
  static bool FromMojom(
      password_manager::mojom::CSVSafariPassword_Status status,
      password_manager::CSVSafariPassword::Status* output);
};

template <>
struct StructTraits<password_manager::mojom::CSVSafariPasswordDataView,
                    password_manager::CSVSafariPassword> {
  static password_manager::CSVSafariPassword::Status status(
      const password_manager::CSVSafariPassword& r) {
    return r.GetParseStatus();
  }
  static const GURL url(const password_manager::CSVSafariPassword& r) {
    return r.GetURL().value_or(GURL());
  }
  static const GURL otp_auth_url(const password_manager::CSVSafariPassword& r) {
    return r.GetOTPAuthURL().value_or(GURL());
  }
  static std::optional<std::string> invalid_url(
      const password_manager::CSVSafariPassword& r) {
    RETURN_IF_ERROR(r.GetURL());
    return std::nullopt;
  }
  static const std::string& title(
      const password_manager::CSVSafariPassword& r) {
    return r.GetTitle();
  }
  static const std::string& username(
      const password_manager::CSVSafariPassword& r) {
    return r.GetUsername();
  }
  static const std::string& password(
      const password_manager::CSVSafariPassword& r) {
    return r.GetPassword();
  }
  static const std::string& notes(
      const password_manager::CSVSafariPassword& r) {
    return r.GetNotes();
  }
  static bool Read(password_manager::mojom::CSVSafariPasswordDataView data,
                   password_manager::CSVSafariPassword* out);
};

}  // namespace mojo

#endif  // BRAVE_COMPONENTS_PASSWORD_MANAGER_SERVICES_CSV_PASSWORD_PUBLIC_MOJOM_CSV_SAFARI_PASSWORD_PARSER_TRAITS_H_
