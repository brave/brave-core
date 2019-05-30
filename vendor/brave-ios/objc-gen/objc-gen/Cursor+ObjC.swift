/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Clang

// MARK: - Obj-C
extension Cursor {
  /// Thrown when there is no Obj-C type that can be used in place of some C++ type
  struct NoObjCTypeError: Error, CustomStringConvertible {
    let cursor: CXCursor
    let type: CXType
    var description: String {
      return "No Obj-C type to use for the given type: \"\(String(describing: type))\""
    }
  }
  /// Thrown when there is no Obj-C transform available between certain C++ types and an Obj-C type
  /// for an r-value assignment.
  ///
  /// Example transforming between an `std::map` where we haven't written a supported
  /// transformer for its given key/value types
  struct NoObjCTransformError: Error {
    let cursor: Cursor
  }
  
  private func _objCType(for type: CXType, inObject: Bool) throws -> String {
    let typeString = clang_getTypeSpelling(type).stringAndDisposeAfter
    switch type.kind {
    case CXType_Record:
      // C++ struct
      let name = clang_getCursorDisplayName(clang_getTypeDeclaration(type)).stringAndDisposeAfter
      return "\(Config.classPrefix)\(name) *"
    case CXType_Enum:
      let name = clang_getCursorDisplayName(clang_getTypeDeclaration(type)).stringAndDisposeAfter
      return "\(Config.classPrefix)\(Enum.formatted(name: name))"
    case CXType_Typedef:
      return try _objCType(for: clang_getCanonicalType(type), inObject: inObject)
    case CXType_Bool,
         CXType_Char_U,
         CXType_UChar,
         CXType_Char16,
         CXType_Char32,
         CXType_UShort,
         CXType_UInt,
         CXType_ULong,
         CXType_ULongLong,
         CXType_UInt128,
         CXType_Char_S,
         CXType_SChar,
         CXType_WChar,
         CXType_Short,
         CXType_Int,
         CXType_Long,
         CXType_LongLong,
         CXType_Int128,
         CXType_Float,
         CXType_Double,
         CXType_LongDouble,
         CXType_Float128:
      return inObject ? "NSNumber *" : typeString
    case CXType_Elaborated:
      if typeString.hasPrefix("std::vector") {
        // Get the type
        let templateType = clang_Type_getTemplateArgumentAsType(type, 0)
        return "NSArray<\(try _objCType(for: templateType, inObject: true))> *"
      } else if typeString.hasPrefix("std::map") {
        // Get the types
        let keyType = clang_Type_getTemplateArgumentAsType(type, 0)
        let valueType = clang_Type_getTemplateArgumentAsType(type, 1)
        return "NSDictionary<\(try _objCType(for: keyType, inObject: true)), \(try _objCType(for: valueType, inObject: true))> *"
      } else if typeString.hasPrefix("std::string") {
        return "NSString *";
      }
    default:
      break
    }
    throw NoObjCTypeError(cursor: cursor, type: type)
  }
  
  /// Get the type to be used in an Obj-C interface. The type used may differ depending on if the
  /// type is used within an object (I.e. A property can be an `int`, but stored within an Obj-C
  /// storage type (i.e. `NSArray`), it must be an NSNumber
  /// - throws: NoObjCTypeError
  func objCType(inObject: Bool = false) throws -> String {
    let type = clang_getCursorType(cursor)
    return try _objCType(for: type, inObject: inObject)
  }
  
  /// Get an Obj-C implementation property assignment r-value string for this cursor.
  /// - throws: NoObjCTypeError
  func objCAssignmentRValueString(_ accessingObjName: String = "obj") throws -> String {
    switch type.kind {
    case CXType_Record:
      // C++ struct
      let name = clang_getCursorDisplayName(clang_getTypeDeclaration(type)).stringAndDisposeAfter
      return "[[\(Config.classPrefix)\(name) alloc] initWith\(name): \(accessingObjName).\(name)];"
    case CXType_Enum:
      let name = clang_getCursorDisplayName(clang_getTypeDeclaration(type)).stringAndDisposeAfter
      return "(\(Config.classPrefix)\(Enum.formatted(name: name)))\(accessingObjName).\(self.name)"
    case CXType_Typedef,
         CXType_Bool,
         CXType_Char_U,
         CXType_UChar,
         CXType_Char16,
         CXType_Char32,
         CXType_UShort,
         CXType_UInt,
         CXType_ULong,
         CXType_ULongLong,
         CXType_UInt128,
         CXType_Char_S,
         CXType_SChar,
         CXType_WChar,
         CXType_Short,
         CXType_Int,
         CXType_Long,
         CXType_LongLong,
         CXType_Int128,
         CXType_Float,
         CXType_Double,
         CXType_LongDouble,
         CXType_Float128:
      return "\(accessingObjName).\(name)"
    case CXType_Elaborated:
      let typeString = clang_getTypeSpelling(type).stringAndDisposeAfter
      if typeString.hasPrefix("std::vector") {
        // Get the type
        let templateType = clang_Type_getTemplateArgumentAsType(type, 0)
        switch templateType.kind {
        case CXType_Record:
          let objcType = try _objCType(for: templateType, inObject: true)
          let cppTypeSpelling = clang_getTypeSpelling(templateType).stringAndDisposeAfter
          let templateTypeString = clang_getCursorSpelling(clang_getTypeDeclaration(templateType)).stringAndDisposeAfter
          return "NSArrayFromVector(\(accessingObjName).\(name), ^\(objcType)(const \(cppTypeSpelling)& o){ return [[\(Config.classPrefix)\(templateTypeString) alloc] initWith\(templateTypeString): o]; })"
        default:
          return "NSArrayFromVector(\(accessingObjName).\(name))"
        }
      } else if typeString.hasPrefix("std::map") {
        // Get the types
        // At the moment we only support [String: AnyObject] dictionaries
        let valueType = clang_Type_getTemplateArgumentAsType(type, 1)
        if valueType.kind == CXType_Record {
          let cppTypeSpelling = clang_getTypeSpelling(valueType) // Includes namespace
          let valueTypeString = clang_getCursorSpelling(clang_getTypeDeclaration(valueType)).stringAndDisposeAfter
          return "NSDictionaryFromMap(\(accessingObjName).\(name), ^\(Config.classPrefix)\(valueTypeString) *(\(cppTypeSpelling) o){ return [[\(Config.classPrefix)\(valueTypeString) alloc] initWith\(valueTypeString):o]; })"
        }
        return "NSDictionaryFromMap(\(accessingObjName).\(name))"
      } else if typeString.hasPrefix("std::string") {
        return "[NSString stringWithUTF8String:\(accessingObjName).\(name).c_str()]";
      }
    default:
      break
    }
    throw NoObjCTransformError(cursor: self)
  }
}
