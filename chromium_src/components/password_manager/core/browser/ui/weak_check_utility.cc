/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/password_manager/core/browser/ui/weak_check_utility.cc"

#include "base/strings/utf_string_conversions.h"

namespace password_manager {

int GetPasswordStrength(const std::string& password) {
  if (password.empty()) {
    return 0;
  }

  // The score returned by PasswordWeakCheck() is an integer between 0 and 4
  // (https://github.com/dropbox/zxcvbn). 0 is weakest.
  return (PasswordWeakCheck(base::UTF8ToUTF16(password)) + 1) / 5.0 * 100;
}

}  // namespace password_manager
