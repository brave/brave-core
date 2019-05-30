/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Clang

/// An Obj-C enum to generate
struct Enum: Hashable, Comparable {
  /// Convert a snake case enum name into an Obj-C capitalized one (i.e. "LOG_LEVEL" returns
  /// "LogLevel")
  static func formatted(name: String) -> String {
    let items = name.components(separatedBy: "_")
    if items.count > 1 {
      // Probably all-caps
      return items.map { $0.lowercased() == "url" ? $0.uppercased() : $0.capitalized }.joined()
    }
    return name // Likely already fine
  }
  /// Converts a snake enum case name into an Obj-C capitalized one (i.e. "LEDGER_OK" returns
  /// "LedgerOk")
  func formatted(caseName: String) -> String {
    let c = caseName.components(separatedBy: "_").map { $0.capitalized }.joined()
    return "\(Enum.formatted(name: name))\(c)"
  }
  /// The enum name (unformatted)
  var name: String
  /// The list of cases and their values (unformatted)
  var cases: [(String, Int64)]
  /// The generated Obj-C enum output
  var generated: String {
    let formattedName = Enum.formatted(name: name)
    return """
    typedef NS_ENUM(NSInteger, \(Config.classPrefix)\(formattedName)) {
    \(cases.map { "  \(Config.classPrefix)\(formatted(caseName: $0.0)) = \($0.1)" }.joined(separator: ",\n"))
    } NS_SWIFT_NAME(\(formattedName));
    """
  }
  static func == (lhs: Enum, rhs: Enum) -> Bool {
    return lhs.name == rhs.name && lhs.cases.count == rhs.cases.count
  }
  func hash(into hasher: inout Hasher) {
    hasher.combine(name)
  }
  static func < (lhs: Enum, rhs: Enum) -> Bool {
    return lhs.name < rhs.name
  }
}

extension Enum {
  /// Create an enum based on a given Cursor
  init(cursor: Cursor) {
    assert(cursor.kind == CXCursor_EnumDecl)
    let cases = cursor.children
      .filter { $0.kind == CXCursor_EnumConstantDecl }
      .map { ($0.name, clang_getEnumConstantDeclValue($0.cursor)) }
    self.init(name: cursor.name, cases: cases)
  }
}
