// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// Finds and removes the `host##selector` cosmetic filters created by the element picker.
///
/// Scriptlets and procedural cosmetic filters are never touched since they can't be
/// created by the element picker and are assumed to be written by the user.
public enum BlockedElementFilters {
  private static let patternsToSkip = [
    // Scriptlets
    "+js(",
    // Procedural cosmetic filters
    ":has(", ":has-text(", ":matches-attr(", ":matches-css(",
    ":matches-css-before(", ":matches-css-after(", ":matches-media(",
    ":matches-path(", ":matches-prop(", ":min-text-length(", ":not(",
    ":others(", ":upward(", ":watch-attr(", ":xpath(",
  ]

  private static func isBlockedElementFilter(_ line: String, host: String) -> Bool {
    line.hasPrefix("\(host)##") && !patternsToSkip.contains(where: { line.contains($0) })
  }

  private static func trimmedLines(_ rules: String) -> [String] {
    rules.split(separator: "\n").map { $0.trimmingCharacters(in: .whitespaces) }
  }

  /// Whether the rules contain any element picker filters for the given host.
  public static func hasFilters(for host: String, in rules: String) -> Bool {
    guard !host.isEmpty, !rules.isEmpty else { return false }
    return trimmedLines(rules).contains { isBlockedElementFilter($0, host: host) }
  }

  /// Returns the rules with all element picker filters for the given host removed, or `nil` if
  /// the host or rules are empty.
  public static func removingFilters(for host: String, from rules: String) -> String? {
    guard !host.isEmpty, !rules.isEmpty else { return nil }
    return trimmedLines(rules)
      .filter { !isBlockedElementFilter($0, host: host) }
      .map { "\($0)\n" }
      .joined()
  }
}
