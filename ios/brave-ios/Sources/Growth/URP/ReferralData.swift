// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import Shared
import SwiftyJSON
import os.log

struct ReferralData {

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

  init?(json: JSON) {
    guard let downloadId = json["download_id"].string, let code = json["referral_code"].string
    else {
      Logger.module.error("Failed to unwrap json to Referral struct.")
      return nil
    }

    self.downloadId = downloadId
    self.referralCode = code
    self.offerPage = json["offer_page_url"].string
  }
}
