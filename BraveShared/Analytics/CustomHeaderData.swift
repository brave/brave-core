/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftyJSON

class CustomHeaderData: NSObject {
    let domainList: [String]
    let headerField: String
    let headerValue: String
    static let bravePartnerKey = "X-Brave-Partner"
    private static let securedBravePartnerKey = "__Secure-\(CustomHeaderData.bravePartnerKey)"

    init(domainList: [String], headerKey: String, headerValue: String) {
        self.domainList = domainList
        self.headerField = headerKey
        self.headerValue = headerValue
    }

    // Initializer can't be placed in extension.
    required init?(coder aDecoder: NSCoder) {
        guard let domainList = aDecoder.decodeObject(of: [NSString.self], forKey: CodingKeys.domain) as? [String],
            let headerKey = aDecoder.decodeObject(of: NSString.self, forKey: CodingKeys.headerKey) as String?,
            let headerValue = aDecoder.decodeObject(of: NSString.self, forKey: CodingKeys.headerValue) as String?
            
            else { return nil }

        self.domainList = domainList
        self.headerField = headerKey
        self.headerValue = headerValue
    }
    
    func cookies() -> [HTTPCookie] {
        let domains = domainList.compactMap { URL(string: $0)?.absoluteString }
        return domains.compactMap {
            let cookie = HTTPCookie(properties: [
                // Must include `.` prefix to be included in subdomains
                .domain: ".\($0)",
                .path: "/",
                .name: "__Secure-\(headerField)",
                .value: headerValue,
                .secure: "TRUE",
                .expires: NSDate(timeIntervalSinceNow: 7.days)
                ])
            if cookie?.name == CustomHeaderData.securedBravePartnerKey {
                return cookie
            }
            assertionFailure("Invalid partner cookie name: \(cookie?.name ?? "Cookie is nil")")
            return nil
        }
    }

    static func customHeaders(from json: JSON) -> [CustomHeaderData] {
        var customHeaders: [CustomHeaderData] = []

        for (_, object) in json {
            guard let header: (String, JSON) = object["headers"].first, header.0 == CustomHeaderData.bravePartnerKey else { continue }

            var domains = [String]()

            for domain in object["domains"] {
                domains.append(domain.1.stringValue)
            }

            customHeaders.append(CustomHeaderData(domainList: domains, headerKey: header.0, headerValue: header.1.stringValue))

        }

        return customHeaders
    }
}

extension CustomHeaderData: NSSecureCoding {
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
    
    static var supportsSecureCoding: Bool {
        return true
    }
}
