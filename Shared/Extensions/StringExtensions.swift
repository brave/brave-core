/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

extension String {
    public func escape() -> String? {
        // We can't guaruntee that strings have a valid string encoding, as this is an entry point for tainted data,
        // we should be very careful about forcefully dereferencing optional types.
        // https://stackoverflow.com/questions/33558933/why-is-the-return-value-of-string-addingpercentencoding-optional#33558934
        let queryItemDividers = CharacterSet(charactersIn: "?=&")
        let allowedEscapes = CharacterSet.urlQueryAllowed.symmetricDifference(queryItemDividers)
        return self.addingPercentEncoding(withAllowedCharacters: allowedEscapes)
    }

    public func unescape() -> String? {
        return self.removingPercentEncoding
    }

    /**
    Ellipsizes a String only if it's longer than `maxLength`

      "ABCDEF".ellipsize(4)
      // "AB…EF"

    :param: maxLength The maximum length of the String.

    :returns: A String with `maxLength` characters or less
    */
    public func ellipsize(maxLength: Int) -> String {
        if (maxLength >= 2) && (self.count > maxLength) {
            let index1 = self.index(self.startIndex, offsetBy: (maxLength + 1) / 2) // `+ 1` has the same effect as an int ceil
            let index2 = self.index(self.endIndex, offsetBy: maxLength / -2)

            return String(self[..<index1]) + "…\u{2060}" + String(self[index2...])
        }
        return self
    }

    private var stringWithAdditionalEscaping: String {
        return self.replacingOccurrences(of: "|", with: "%7C")
    }

    public var asURL: URL? {
        // Firefox and NSURL disagree about the valid contents of a URL.
        // Let's escape | for them.
        // We'd love to use one of the more sophisticated CFURL* or NSString.* functions, but
        // none seem to be quite suitable.
        return URL(string: self) ??
               URL(string: self.stringWithAdditionalEscaping)
    }

    /// Returns a new string made by removing the leading String characters contained
    /// in a given character set.
    public func stringByTrimmingLeadingCharactersInSet(_ set: CharacterSet) -> String {
        var trimmed = self
        while trimmed.rangeOfCharacter(from: set)?.lowerBound == trimmed.startIndex {
            trimmed.remove(at: trimmed.startIndex)
        }
        return trimmed
    }
    
    //Minimize trimming effort for characterset based on string
    public func trim(_ charactersInString: String) -> String {
        return self.trimmingCharacters(in: CharacterSet(charactersIn: charactersInString))
    }

    /// Adds a newline at the closest space from the middle of a string.
    /// Example turning "Mark as Read" into "Mark as\n Read"
    public func stringSplitWithNewline() -> String {
        let mid = self.count / 2

        let arr: [Int] = self.indices.compactMap {
            if self[$0] == " " {
                return self.distance(from: startIndex, to: $0)
            }

            return nil
        }
        guard let closest = arr.enumerated().min(by: { abs($0.1 - mid) < abs($1.1 - mid) }) else {
            return self
        }
        var newString = self
        newString.insert("\n", at: newString.index(newString.startIndex, offsetBy: closest.element))
        return newString
    }

    public static func contentsOfFileWithResourceName(_ name: String, ofType type: String, fromBundle bundle: Bundle, encoding: String.Encoding, error: NSErrorPointer) -> String? {
        return bundle.path(forResource: name, ofType: type).flatMap {
            try? String(contentsOfFile: $0, encoding: encoding)
        }
    }
    
    public func regexReplacePattern(_ pattern: String, with: String) throws -> String {
        let regex = try NSRegularExpression(pattern: pattern, options: [])
        return regex.stringByReplacingMatches(in: self, options: [], range: NSRange(location: 0, length: self.count), withTemplate: with)
    }
    
    public func separatedBy(_ string: String) -> [String] {
        let cleaned = self.replacingOccurrences(of: "\n", with: " ")
        return cleaned.trimmingCharacters(in: .whitespacesAndNewlines).components(separatedBy: string)
    }
    
    /// Makes a part of the string bold and returns a NSAttributedString.
    /// Text size must be provided for bold system font and to make font size the same as rest of the string.
    public func makePartiallyBoldAttributedString(stringToBold text: String, boldTextSize: CGFloat) -> NSAttributedString? {
        let addWordsDescriptionBolded = NSMutableAttributedString(string: self)
        guard let rangeOfBoldedText = self.range(of: text) else { return nil }
        // NSMutableAttributedString still uses NSRange, a conversion from Swift's range is required.
        let nsRangeOfBoldedText = NSRange(rangeOfBoldedText, in: self)
        // Make sure we use the same font size for the bolded text.
        let attributes: [NSAttributedString.Key: Any] = [.font: UIFont.boldSystemFont(ofSize: boldTextSize)]
        addWordsDescriptionBolded.setAttributes(attributes, range: nsRangeOfBoldedText)
        return addWordsDescriptionBolded
    }
    
    /*
     Truncates the string to the specified length number of characters and appends an optional trailing string if longer.
     - Parameter length: Desired maximum lengths of a string
     - Parameter trailing: A 'String' that will be appended after the truncation.
     
     - Returns: 'String' object.
     */
    public func truncate(length: Int, trailing: String = "…") -> String {
        return (self.count > length) ? self.prefix(length) + trailing : self
    }
}
