/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveCore

extension BraveWallet.AccountInfo {
  static var previewAccount: BraveWallet.AccountInfo {
    let account = BraveWallet.AccountInfo()
    account.name = "Account 1"
    account.address = "0x879240B2D6179E9EC40BC2AFFF"
    return account
  }
}
