// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// Receiver-side file name sanitization (PROTOCOL.md §7). The result is
/// never empty, never more than 255 UTF-8 bytes, and contains no path
/// separators.
public enum FilenameSanitizer {
  public static let maxNameBytes = 255
  public static let maxExtensionBytes = 32
  public static let fallbackName = "file"

  private static let replaced = Set("/\\<>:\"|?*".unicodeScalars)
  private static let reserved: Set<String> = {
    var names: Set<String> = ["CON", "PRN", "AUX", "NUL"]
    for i in 1...9 {
      names.insert("COM\(i)")
      names.insert("LPT\(i)")
    }
    return names
  }()

  private static func isRemoved(_ v: UInt32) -> Bool {
    v <= 0x1f || (0x7f...0x9f).contains(v) || v == 0x061c || (0x200b...0x200f).contains(v)
      || (0x202a...0x202e).contains(v) || (0x2060...0x2069).contains(v) || v == 0xfeff
  }

  private static func utf8Count(_ scalars: ArraySlice<Unicode.Scalar>) -> Int {
    scalars.reduce(0) { $0 + UTF8.width($1) }
  }

  private static func trim(
    _ scalars: ArraySlice<Unicode.Scalar>, leading: Bool, trailing: Bool
  ) -> ArraySlice<Unicode.Scalar> {
    var s = scalars
    let strip: (Unicode.Scalar) -> Bool = { $0 == " " || $0 == "." }
    while leading, let first = s.first, strip(first) { s = s.dropFirst() }
    while trailing, let last = s.last, strip(last) { s = s.dropLast() }
    return s
  }

  private static func asciiUppercased(_ scalars: ArraySlice<Unicode.Scalar>) -> String {
    var out = String.UnicodeScalarView()
    for s in scalars {
      out.append(("a"..."z").contains(s) ? Unicode.Scalar(s.value - 32)! : s)
    }
    return String(out)
  }

  public static func sanitize(_ name: String) -> String {
    var scalars: [Unicode.Scalar] = []
    for s in name.unicodeScalars where !isRemoved(s.value) {
      scalars.append(replaced.contains(s) ? "_" : s)
    }
    var result = Array(trim(scalars[...], leading: true, trailing: true))
    if result.isEmpty {
      return fallbackName
    }
    // Windows ignores trailing spaces, so "aux .txt" is reserved too.
    var stem = result.prefix { $0 != "." }
    while stem.last == " " { stem = stem.dropLast() }
    if reserved.contains(asciiUppercased(stem)) {
      result.insert("_", at: 0)
    }
    if utf8Count(result[...]) > maxNameBytes {
      var ext: ArraySlice<Unicode.Scalar> = []
      if let dot = result.lastIndex(of: "."), dot > 0 {
        ext = result[dot...]
      }
      if utf8Count(ext) > maxExtensionBytes {
        ext = []
      }
      let budget = maxNameBytes - utf8Count(ext)
      var kept: ArraySlice<Unicode.Scalar> = []
      var used = 0
      for s in result[..<(result.count - ext.count)] {
        let width = UTF8.width(s)
        if used + width > budget { break }
        used += width
        kept = result[..<(kept.count + 1)]
      }
      result = Array(trim(kept, leading: false, trailing: true)) + Array(ext)
    }
    var view = String.UnicodeScalarView()
    view.append(contentsOf: result)
    return String(view)
  }
}
