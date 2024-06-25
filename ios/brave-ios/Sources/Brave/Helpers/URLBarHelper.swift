// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

class URLBarHelper {

  static let shared = URLBarHelper()

  func shouldShowSearchSuggestions(using lastReplacement: String) async -> Bool {
    // Check if last entry to url textfield needs to be checked as suspicious.
    // The reason of checking count is bigger than 1 is the single character
    // entries will always be safe and only way to achieve multi character entry is
    // using paste board.
    // This check also allow us to handle paste permission case
    guard lastReplacement.count > 1 else {
      return true
    }

    // Check if paste board has any text to guarantee the case
    guard UIPasteboard.general.hasStrings || UIPasteboard.general.hasURLs else {
      return true
    }

    // Perform check on pasted text
    if let pasteboardContents = UIPasteboard.general.string {
      let isSuspicious = await isSuspiciousQuery(pasteboardContents)
      return !isSuspicious
    }

    return true
  }

  /// Whether  the desired text should allow search suggestions to appear when it is copied
  /// - Parameter query: Search query copied
  /// - Returns: the result if it is suspicious
  func isSuspiciousQuery(_ query: String) async -> Bool {
    // Remove the msg if the query is too long
    if query.count > 50 {
      return true
    }

    // Remove the msg if the query contains more than 7 words
    if query.components(separatedBy: " ").count > 7 {
      return true
    }

    // Remove the msg if the query contains a number longer than 7 digits
    if let _ = checkForLongNumber(query, 7) {
      return true
    }

    // Remove if email (exact), even if not totally well formed
    if checkForEmail(query) {
      return true
    }

    // Remove if query looks like an http pass
    if query.range(of: "[^:]+:[^@]+@", options: .regularExpression) != nil {
      return true
    }

    for word in query.components(separatedBy: " ") {
      if word.range(of: "[^:]+:[^@]+@", options: .regularExpression) != nil {
        return true
      }
    }

    if query.count > 12 {
      let literalsPattern = "[^A-Za-z0-9]"

      guard
        let literalsRegex = try? NSRegularExpression(
          pattern: literalsPattern,
          options: .caseInsensitive
        )
      else {
        return true
      }

      let range = NSRange(location: 0, length: query.utf16.count)

      let cquery = literalsRegex.stringByReplacingMatches(
        in: query,
        options: [],
        range: range,
        withTemplate: ""
      )

      if cquery.count > 12 {
        let pp = isHashProb(cquery)
        // we are a bit more strict here because the query
        // can have parts well formed
        if pp < URLBarHelperConstants.probHashThreshold * 1.5 {
          return true
        }
      }
    }

    return false
  }

  private func checkForLongNumber(_ str: String, _ maxNumberLength: Int) -> String? {
    let controlString = str.replacingOccurrences(
      of: "[^A-Za-z0-9]",
      with: "",
      options: .regularExpression
    )

    var location = 0
    var maxLocation = 0
    var maxLocationPosition: String.Index? = nil

    for i in controlString.indices {
      if controlString[i] >= "0" && controlString[i] <= "9" {
        location += 1
      } else {
        if location > maxLocation {
          maxLocation = location
          maxLocationPosition = i
        }

        location = 0
      }
    }

    if location > maxLocation {
      maxLocation = location
      maxLocationPosition = controlString.endIndex
    }

    if let maxLocationPosition = maxLocationPosition, maxLocation > maxNumberLength {
      let start = controlString.index(maxLocationPosition, offsetBy: -maxLocation)
      let end = maxLocationPosition

      return String(controlString[start..<end])
    } else {
      return nil
    }
  }

  private func checkForEmail(_ str: String) -> Bool {
    let emailPattern = "[a-z0-9\\-_@]+(@|%40|%(25)+40)[a-z0-9\\-_]+\\.[a-z0-9\\-_]"

    guard
      let emailRegex = try? NSRegularExpression(pattern: emailPattern, options: .caseInsensitive)
    else {
      return false
    }

    let range = NSRange(location: 0, length: str.utf16.count)
    return emailRegex.firstMatch(in: str, options: [], range: range) != nil
  }

  private func isHashProb(_ str: String) -> Double {
    var logProb = 0.0
    var transC = 0
    let filteredStr = str.replacingOccurrences(
      of: "[^A-Za-z0-9]",
      with: "",
      options: .regularExpression
    )

    let characters = Array(filteredStr)
    for i in 0..<(characters.count - 1) {
      if let pos1 = URLBarHelperConstants.probHashChars[characters[i]],
        let pos2 = URLBarHelperConstants.probHashChars[characters[i + 1]]
      {
        logProb += URLBarHelperConstants.probHashLogM[pos1][pos2]
        transC += 1
      }
    }

    if transC > 0 {
      return exp(logProb / Double(transC))
    } else {
      return exp(logProb)
    }
  }

}
