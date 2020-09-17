/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Clang

/// A C++ struct which will generate an Obj-C @interface
struct Interface: Hashable, Comparable {
  /// An interface @property
  struct Property {
    /// The name of the property (i.e. "isEnabled") (unformatted from C++)
    let name: String
    /// The property decleration (This will be the @property string placed inside a @interface)
    let decleration: String
    /// The assignment string (This will be the r-value string placed inside a @implementaion)
    let assignmentString: String
  
    /// Convert a snake case property name into an Obj-C camcelCase'd one (i.e. "opening_balance_"
    /// returns "openingBalance")
    static func formatted(name: String) -> String {
      let pieces = name.components(separatedBy: "_")
      if pieces.isEmpty { return name }
      if pieces.count == 1 { return pieces.first! }
      return ([pieces.first!.lowercased()] + pieces.dropFirst().map { $0.capitalized }).joined()
    }
  }
  /// The @interface's name
  let name: String
  /// The C++ structs name (including the namespace)
  let cppTypeName: String
  /// The name with the class prefix used from Config
  var prefixedName: String {
    return "\(Config.classPrefix)\(name)"
  }
  /// The properties that will mirror a C++ structs fields
  let properties: [Property]
  /// The generated output of a this @interface
  var generatedPublicInterface: String {
    return """
    OBJC_EXPORT
    NS_SWIFT_NAME(\(name))
    @interface \(prefixedName) : NSObject
    \(properties.map { $0.decleration }.joined(separator: "\n"))
    @end
    """
  }
  /// The private generated output of a this @interface (containing C++ to Obj-C inits)
  var generatedPrivateInterface: String {
    return """
    @interface \(prefixedName) (Private)
    - (instancetype)initWith\(name):(const \(cppTypeName)&)obj;
    @end
    """
  }
  /// The generated implementation of this @interface (containing the Obj-C init methods found in
  /// `generatedPrivateInterface`)
  var generatedImplementation: String {
    let assignments = properties.map({ return "self.\(Property.formatted(name: $0.name)) = \($0.assignmentString);"})
    return """
    @implementation \(Config.classPrefix)\(name)
    - (instancetype)initWith\(name):(const \(cppTypeName)&)obj {
      if ((self = [super init])) {
    \(assignments.map { "    \($0)" }.joined(separator: "\n"))
      }
      return self;
    }
    @end
    """
  }
  static func == (lhs: Interface, rhs: Interface) -> Bool {
    return lhs.name == rhs.name
  }
  func hash(into hasher: inout Hasher) {
    hasher.combine(name)
  }
  static func < (lhs: Interface, rhs: Interface) -> Bool {
    return lhs.name < rhs.name
  }
}

extension Interface {
  init(cursor: Cursor) throws {
    // Currently only supporting creating an Obj-C class from a C++ struct
    assert(cursor.kind == CXCursor_StructDecl)
    self.init(
      name: cursor.name,
      cppTypeName: clang_getTypeSpelling(cursor.type).stringAndDisposeAfter,
      properties: try cursor.children.filter({ $0.kind == CXCursor_FieldDecl }).map({ try Property(cursor: $0) })
    )
  }
}

extension Interface.Property {
  init(cursor: Cursor) throws {
    assert(cursor.kind == CXCursor_FieldDecl)
    self.init(
      name: cursor.name,
      decleration: "@property (nonatomic) \(try cursor.objCType()) \(Interface.Property.formatted(name: cursor.name));",
      assignmentString: try cursor.objCAssignmentRValueString("obj")
    )
  }
}
