/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Clang

extension Cursor {
  var isConstCXXMethod: Bool {
    return clang_CXXMethod_isConst(cursor) == 1
  }
  
  var isPureVirtualCXXMethod: Bool {
    return clang_CXXMethod_isPureVirtual(cursor) == 1
  }
  
  var resultType: CXType {
    return clang_getResultType(type)
  }
}

struct Method: Hashable, Comparable {
  struct Argument: Hashable {
    let type: String
    let typeIsUniquePtr: Bool
    let name: String
    var objcProtocolFormattedName: String {
      return Interface.Property.formatted(name: name)
    }
  }
  
  let resultType: String
  let resultIsVoid: Bool
  let name: String
  let isConst: Bool
  let arguments: [Argument]
  
  private static func typeStringFixingLackingNamespace(_ type: CXType) -> String {
    let numberOfTemplateArgs = clang_Type_getNumTemplateArguments(type)
    let typeString = clang_getTypeSpelling(type).stringAndDisposeAfter
    if numberOfTemplateArgs > 0 {
      if typeString.hasPrefix("std::unique_ptr") {
        return "std::unique_ptr<\(clang_Type_getTemplateArgumentAsType(type, 0))>"
      }
    }
    return typeString
  }
  
  init(cursor: Cursor) {
    self.resultType = Method.typeStringFixingLackingNamespace(cursor.resultType)
    self.resultIsVoid = cursor.resultType.kind == CXType_Void
    self.name = clang_getCursorSpelling(cursor.cursor).stringAndDisposeAfter
    self.isConst = clang_CXXMethod_isConst(cursor.cursor) == 1
    self.arguments = (0..<clang_Cursor_getNumArguments(cursor.cursor)).map {
      let argCursor = Cursor(clang_Cursor_getArgument(cursor.cursor, UInt32($0)))
      let typeString = Method.typeStringFixingLackingNamespace(argCursor.type)
      let mojoSpelling = clang_getTypeSpelling(clang_getCanonicalType(argCursor.type)).stringAndDisposeAfter
      // Dirty hack for now:
      let isMojo = mojoSpelling.hasPrefix("mojo::StructPtr<") || mojoSpelling.hasPrefix("std::__1::vector<mojo::StructPtr<")
      return Argument(
        type: typeString,
        typeIsUniquePtr: isMojo || clang_getTypeSpelling(argCursor.type).stringAndDisposeAfter.contains("unique_ptr"),
        // Make sure to give an argument name
        name: argCursor.name.isEmpty ? "arg\($0)" : argCursor.name
      )
    }
  }
  
  var generatedPublicDecleration: String {
    var s = "\(resultType) \(name)(\(arguments.map { "\($0.type) \($0.name)" }.joined(separator: ", ")))"
    if isConst {
      s.append(" const")
    }
    s.append(" override;")
    return s
  }
  
  func generatedSourceImplementation(parentClass: String) -> String {
    var s = "\(resultType) \(parentClass)::\(name)(\(arguments.map { "\($0.type) \($0.name)" }.joined(separator: ", ")))"
    if isConst {
      s.append(" const")
    }
    return s
  }
  
  var generatedProtocolMethodCall: String {
    if name.isEmpty { return "" }
    var s: String
    if name.hasPrefix("URL") || name.hasPrefix("URI") {
      s = name
    } else {
      s = "\(name.first!.lowercased() + String(name.dropFirst()))"
    }
    if !arguments.isEmpty {
      let args = arguments.map {
        let value = $0.typeIsUniquePtr ? "std::move(\($0.name))" : $0.name
        return $0 == arguments.first ? value : "\($0.objcProtocolFormattedName):\(value)"
      }.joined(separator: " ")
      s.append(":\(args)")
    }
    return s
  }
  
  var generatedProtocolMethodDecleration: String {
    if name.isEmpty { return "" }
    var s = "- (\(resultType))"
    if name.hasPrefix("URL") || name.hasPrefix("URI") {
      s.append(name)
    } else {
      s.append("\(name.first!.lowercased() + String(name.dropFirst()))")
    }
    if !arguments.isEmpty {
      s.append(":\(arguments.map { $0 == arguments.first ? "(\($0.type))\($0.name)" : "\($0.objcProtocolFormattedName):(\($0.type))\($0.name)" }.joined(separator: " "))")
    }
    s.append(";")
    return s
  }
  
  static func < (lhs: Method, rhs: Method) -> Bool {
    return lhs.name < rhs.name
  }
}

/// Create a generated Obj-C bridge from the a abstract client class (i.e. `ledger_client` or
/// `ads_client`)
///
/// Outputs:
/// _Assuming that className was "LedgerClient"_
///   - `NativeLedgerClient.h`: The C++ header class, redefines all methods as non-virtual overrides
///   - `NativeLedgerClient.mm`: The C++ class which will accept a `Native[ClassName]Bridge` and
///      redirect all callbacks to said bridge
///   - `NativeLedgerClientBridge.h`: An Obj-C++ header defining a protocol with all
///      `NativeLedgerClient` callbacks
func createBridge(from clientFile: String, className: String, includePaths: [String], outputDirectory: String) {
  let idx = clang_createIndex(0, 1)
  defer { clang_disposeIndex(idx) }
  // Have to define "LEDGER_EXPORT" so we don't get parsing errors.
  // I assume its because we are parsing headers and not source files
  let args: [String] = ["-x", "c++", "-std=c++14", "-DLEDGER_EXPORT= ", "-iframework", "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks"] +
    (Config.systemIncludes.flatMap { ["-isystem", $0] }) +
    (includePaths.flatMap { ["-I", $0] })
  var unit: CXTranslationUnit!
  let errorCode = clang_parseTranslationUnit2(idx, clientFile, args.map { ($0 as NSString).utf8String }, Int32(args.count), nil, 0, 0, &unit)
  if errorCode.rawValue != 0 {
    print("Couldn't parse \(clientFile)")
  }
  
  var methods: Set<Method> = []
  var namespace: String = ""
  
  func _traverse(nodes: [Cursor]) {
    for node in nodes where !node.children.isEmpty && node.isFromMainFile {
      switch node.kind {
      case CXCursor_Namespace:
        _traverse(nodes: node.children)
      case CXCursor_ClassDecl where node.name == className:
        namespace = Cursor(clang_getCursorLexicalParent(node.cursor)).name
        methods.formUnion(
          node.children
            .filter { $0.kind == CXCursor_CXXMethod && $0.isPureVirtualCXXMethod }
            .map(Method.init)
        )
      default:
        continue
      }
    }
  }
  
  _traverse(nodes: Cursor(clang_getTranslationUnitCursor(unit)).children)
  
  var updatedPath = clientFile
  includePaths.forEach {
    updatedPath = updatedPath.replacingOccurrences(of: $0, with: "")
  }
  if updatedPath.hasPrefix("/") {
    updatedPath = String(updatedPath.dropFirst())
  }
  
  let sortedMethods = methods.sorted()
  let outputedFiles: [TemplateOutput] = [
    NativeClientHeaderOutput(namespace: namespace, className: className, includeHeader: updatedPath, methods: sortedMethods),
    NativeClientSourceOutput(className: className, methods: sortedMethods),
    NativeClientBridgeProtocolOutput(className: className, includeHeader: updatedPath, methods: sortedMethods)
  ]
  
  do {
    try FileManager.default.createDirectory(atPath: outputDirectory, withIntermediateDirectories: true, attributes: nil)
    try outputedFiles.forEach {
      try $0.generated.write(toFile: "\(outputDirectory)/\($0.filename)", atomically: true, encoding: .utf8)
    }
  } catch {
    print("Failed to write generated files to output directory: \(String(describing: error))")
  }
}

