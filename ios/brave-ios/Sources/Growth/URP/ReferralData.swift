// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import Shared
import os.log

struct ReferralData: Decodable {

  let downloadId: String
  let referralCode: String
  let offerPage: String?

  func isExtendedUrp() -> Bool {
    return offerPage != nil
  }

  init(downloadId: String, code: String, offerPage: String? = nil) {
    self.downloadId = downloadId
    self.referralCode = code

    self.offerPage = offerPage
  }

  enum CodingKeys: String, CodingKey {
    case downloadId = "download_id"
    case referralCode = "referral_code"
    case offerPage = "offer_page_url"
  }
}
