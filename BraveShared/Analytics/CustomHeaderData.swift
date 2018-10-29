/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftyJSON

class CustomHeaderData: NSObject {
    let domainList: [String]
    let headerField: String
    let headerValue: String

    init(domainList: [String], headerKey: String, headerValue: String) {
        self.domainList = domainList
        self.headerField = headerKey
        self.headerValue = headerValue
    }

    // Initializer can't be placed in extension.
    required init?(coder aDecoder: NSCoder) {
        guard let domainList = aDecoder.decodeObject(forKey: CodingKeys.domain) as? [String],
            let headerKey = aDecoder.decodeObject(forKey: CodingKeys.headerKey) as? String,
            let headerValue = aDecoder.decodeObject(forKey: CodingKeys.headerValue) as? String
            else { return nil }

        self.domainList = domainList
        self.headerField = headerKey
        self.headerValue = headerValue
    }

    static func customHeaders(from json: JSON) -> [CustomHeaderData] {
        var customHeaders: [CustomHeaderData] = []

        for (_, object) in json {
            guard let header = object["headers"].first else { continue }

            var domains = [String]()

            for domain in object["domains"] {
                domains.append(domain.1.stringValue)
            }

            customHeaders.append(CustomHeaderData(domainList: domains, headerKey: header.0, headerValue: header.1.stringValue))

        }

        return customHeaders
    }
}

extension CustomHeaderData: NSCoding {
    struct CodingKeys {
        static let domain = "customHeaderDataDomain"
        static let headerKey = "customHeaderDataHeaderKey"
        static let headerValue = "customHeaderDataHeaderValue"
    }

    func encode(with aCoder: NSCoder) {
        aCoder.encode(domainList, forKey: CodingKeys.domain)
        aCoder.encode(headerField, forKey: CodingKeys.headerKey)
        aCoder.encode(headerValue, forKey: CodingKeys.headerValue)
    }
}
