// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

extension String {
    /// The first URL found within this String, or nil if no URL is found
    public var firstURL: URL? {
        if let detector = try? NSDataDetector(types: NSTextCheckingResult.CheckingType.link.rawValue),
            let match = detector.firstMatch(in: self, options: [], range: NSRange(location: 0, length: self.count)),
            let range = Range(match.range, in: self) {
            return URL(string: String(self[range]))
        }
        return nil
    }
}
