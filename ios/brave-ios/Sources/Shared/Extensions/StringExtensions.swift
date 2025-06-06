// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit
import os.log

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

  private var stringWithAdditionalEscaping: String {
    return self.replacingOccurrences(of: "|", with: "%7C")
  }

  public var asURL: URL? {
    // Firefox and NSURL disagree about the valid contents of a URL.
    // Let's escape | for them.
    // We'd love to use one of the more sophisticated CFURL* or NSString.* functions, but
    // none seem to be quite suitable.
    return URL(string: self) ?? URL(string: self.stringWithAdditionalEscaping)
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

  // Minimize trimming effort for characterset based on string
  public func trim(_ charactersInString: String) -> String {
    return self.trimmingCharacters(in: CharacterSet(charactersIn: charactersInString))
  }

  public func separatedBy(_ string: String) -> [String] {
    let cleaned = self.replacingOccurrences(of: "\n", with: " ")
    return cleaned.trimmingCharacters(in: .whitespacesAndNewlines).components(separatedBy: string)
  }

  /// Truncates the string to the specified length number of characters and appends an optional trailing string if longer.
  /// - Parameter length: Desired maximum lengths of a string
  /// - Parameter trailing: A 'String' that will be appended after the truncation.
  ///
  /// - Returns: 'String' object.
  public func truncate(length: Int, trailing: String = "…") -> String {
    return (self.count > length) ? self.prefix(length) + trailing : self
  }

  public var capitalizeFirstLetter: String {
    return prefix(1).capitalized + dropFirst()
  }

  public var withNonBreakingSpace: String {
    self.replacingOccurrences(of: " ", with: "\u{00a0}")
  }

  public var withSecureUrlScheme: String {
    var textEntered = self

    if !textEntered.hasPrefix("https://") {
      if textEntered.hasPrefix("http://") {
        textEntered = String(textEntered.dropFirst(7))
      }

      return "https://\(textEntered)"
    } else {
      let substringWithoutHttps = String(textEntered.dropFirst(8))

      if substringWithoutHttps.hasPrefix("https://") {
        return substringWithoutHttps
      } else if substringWithoutHttps.hasPrefix("http://") {
        let substringWithoutHttp = String(substringWithoutHttps.dropFirst(7))
        return "https://\(substringWithoutHttp)"
      }
    }

    return textEntered
  }

  // Taken from https://gist.github.com/pwightman/64c57076b89c5d7f8e8c
  // Returns nil if the string cannot be escaped
  public var javaScriptEscapedString: String? {
    // Because JSON is not a subset of JavaScript, the LINE_SEPARATOR and PARAGRAPH_SEPARATOR unicode
    // characters embedded in (valid) JSON will cause the webview's JavaScript parser to error. So we
    // must encode them first. See here: http://timelessrepo.com/json-isnt-a-javascript-subset
    // Also here: http://media.giphy.com/media/wloGlwOXKijy8/giphy.gif
    let str = self.replacingOccurrences(of: "\u{2028}", with: "\\u2028")
      .replacingOccurrences(of: "\u{2029}", with: "\\u2029")
    // Because escaping JavaScript is a non-trivial task (https://github.com/johnezang/JSONKit/blob/master/JSONKit.m#L1423)
    // we proceed to hax instead:
    do {
      let encoder = JSONEncoder()
      let data = try encoder.encode([str])
      let encodedString = String(decoding: data, as: UTF8.self)
      return String(encodedString.dropLast().dropFirst())
    } catch {
      Logger.module.error(
        "Failed to escape a string containing javascript: \(error.localizedDescription)"
      )
      return nil
    }
  }

  /// Encode HTMLStrings
  /// Also used for Strings which are not sanitized for displaying
  /// - Returns: Encoded String
  public var htmlEntityEncodedString: String {
    return
      self
      .replacingOccurrences(of: "&", with: "&amp;", options: .literal)
      .replacingOccurrences(of: "\"", with: "&quot;", options: .literal)
      .replacingOccurrences(of: "'", with: "&#39;", options: .literal)
      .replacingOccurrences(of: "<", with: "&lt;", options: .literal)
      .replacingOccurrences(of: ">", with: "&gt;", options: .literal)
      .replacingOccurrences(of: "`", with: "&lsquo;", options: .literal)
  }
}
