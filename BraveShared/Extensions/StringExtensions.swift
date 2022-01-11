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
    
    /// Obtain a list of words in a given string
    public var words: [String] {
        var words: [String] = []
        enumerateSubstrings(
            in: startIndex..<endIndex,
            options: .byWords
        ) { (word, _, _, _) in
            if let word = word {
                words.append(word)
            }
        }
        return words
    }
    
    /// Encode a String to Base64
    public func toBase64() -> String {
        return Data(self.utf8).base64EncodedString()
    }
}
