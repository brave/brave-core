/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import SwiftyJSON

private let log = Logger.browserLogger

struct ReferralData {

    let downloadId: String
    let referralCode: String
    let offerPage: String?

    let customHeaders: [CustomHeaderData]?

    func isExtendedUrp() -> Bool {
        return offerPage != nil
    }

    init(downloadId: String, code: String, offerPage: String? = nil, customHeaders: [CustomHeaderData]? = nil) {
        self.downloadId = downloadId
        self.referralCode = code

        self.offerPage = offerPage
        self.customHeaders = customHeaders
    }

    init?(json: JSON) {
        guard let downloadId = json["download_id"].string, let code = json["referral_code"].string else {
            log.error("Failed to unwrap json to Referral struct.")
            return nil
        }

        self.downloadId = downloadId
        self.referralCode = code
        self.offerPage = json["offer_page_url"].string

        var headers = [CustomHeaderData]()
        headers.append(contentsOf: CustomHeaderData.customHeaders(from: json["headers"]))

        self.customHeaders = headers.count > 0 ? headers : nil
    }
}
