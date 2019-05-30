/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Clang

/// Simple wrapper around CXCursor
final class Cursor {
  /// The underlying cursor
  let cursor: CXCursor
  init(_ cursor: CXCursor) {
    self.cursor = cursor
  }
  /// The display name for this cursor
  var name: String {
    return clang_getCursorDisplayName(cursor).stringAndDisposeAfter
  }
  /// The cursor kind (struct, enum, etc.)
  var kind: CXCursorKind {
    return clang_getCursorKind(cursor)
  }
  /// The cursor's type (void, bool, int, etc.)
  var type: CXType {
    return clang_getCursorType(cursor)
  }
  /// Whether or not this cursor belongs to the main file being parsed
  var isFromMainFile: Bool {
    return clang_Location_isFromMainFile(clang_getCursorLocation(cursor)) == 1
  }
  /// The cursor's children
  lazy private(set) var children: [Cursor] = {
    var cursors: [Cursor] = []
    clang_visitChildrenWithBlock(cursor, { (cursor, parent) -> CXChildVisitResult in
      cursors.append(Cursor(cursor))
      return CXChildVisit_Continue
    })
    return cursors
  }()
}

extension CXString {
  /// Obtain a Swift string and dispose the underlying CXString
  var stringAndDisposeAfter: String {
    defer { clang_disposeString(self) }
    if data == nil { return "" }
    return String(cString: clang_getCString(self))
  }
}

// MARK: - Debug Descriptions

extension CXString: CustomDebugStringConvertible {
  public var debugDescription: String {
    return self.stringAndDisposeAfter
  }
}

extension Cursor: CustomDebugStringConvertible {
  var debugDescription: String {
    let prettyPrinted = clang_getCursorPrettyPrinted(cursor, nil).stringAndDisposeAfter
    if prettyPrinted.count > 0 {
      return prettyPrinted
    }
    return "\(clang_getCursorKindSpelling(kind).stringAndDisposeAfter) \(clang_getCursorDisplayName(cursor).stringAndDisposeAfter)"
  }
}

extension CXType: CustomDebugStringConvertible {
  public var debugDescription: String {
    return clang_getTypeSpelling(self).stringAndDisposeAfter
  }
}

extension CXCursorKind: CustomDebugStringConvertible {
  public var debugDescription: String {
    return clang_getCursorKindSpelling(self).stringAndDisposeAfter
  }
}

