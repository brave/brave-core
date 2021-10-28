// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

struct PasswordValidation {
  /// Whether or not a password is valid to use when creating or restoring a wallet
  ///
  /// Currently a strong password is defined by having at least 7 characters, and including at least
  /// 1 number and 1 special character.
  ///
  /// The available special characters are:
  ///
  ///      \` ~ ! @ # $ % ^ & * ( ) + = ? ; : | < > , . ' " { } / [ ] \ - _
  static func isValid(_ password: String) -> Bool {
    let specialCharactersSet = CharacterSet(charactersIn: #"`~!@#$%^&*()+=?;:|<>,.'"{}/[]\-_"#)
    let numberSet = CharacterSet(charactersIn: "0123456789")
    let passwordSet = CharacterSet(charactersIn: password)
    return password.count > 7 &&
      !specialCharactersSet.intersection(passwordSet).isEmpty &&
      !numberSet.intersection(passwordSet).isEmpty
  }
}
