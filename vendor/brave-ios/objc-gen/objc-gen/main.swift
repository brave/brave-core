/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Clang

/*
 **objc-gen**
 Use libclang (https://clang.llvm.org/doxygen/group__CINDEX.html) to parse the AST of header
 files and output Obj-C (which can then be imported by Swift)
 
 objc-gen requires the following dependencies:
  - Xcode (Defaults to being required at /Applications/Xcode.app, but can be changed in the Config)
  - LLVM (can be installed via `brew install llvm`)

 Input:
   A directory or single header file to read supported `structs` & `enums`
 Output:
   - `Enums.h`: Contains all the enums used within inputted header files
   - `Records.h`: Public Obj-C interface for all parsed C++ structs
   - `Records+Private.h`: A private interface including init methods which take the mirrored C++
      struct as an argument. Used to create Obj-C structs from C++, and in the future, vice-versa
   - `Records.mm`: The Obj-C++ implementations
 
 Currently only used for bat-native-ledger, but could be used for bat-native-ads as well
 
 Note: `CppTransformations.h` contains helper methods between supported C++ classes/structs
 and an Obj-C type. I.e. `std::string` <-> `NSString`, `std::vector` <-> `NSArray`, and
 `std::map` <-> `NSDictionary` and is required to be included by the project importing the
 outputted files
*/

final class Config {
  /// The Obj-C class prefixes
  static let classPrefix = "BAT"
  /// Includes which are to be included when parsing the AST (both are needed for some reason as of
  /// llvm 8.0)
  static let systemIncludes = [
    "/usr/local/opt/llvm/include/c++/v1",
    "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include",
    // Temporary hack:
    "/Users/kyle/git/brave/brave-browser/src/",
    "/Users/kyle/git/brave/brave-browser/src/out/sim-release/gen/brave/vendor/bat-native-ledger/include",
    "/Users/kyle/git/brave/brave-browser/src/out/sim-release/gen/",
  ]
}

func generate(from files: [String], includePaths: [String], outputDirectory: String) {
  var enums: Set<Enum> = []
  var interfaces: Set<Interface> = []
  
  func _traverse(nodes: [Cursor]) {
    for node in nodes where !node.children.isEmpty && node.isFromMainFile {
      switch node.kind {
      case CXCursor_Namespace:
        _traverse(nodes: node.children)
      case CXCursor_StructDecl:
        do {
          interfaces.insert(try Interface(cursor: node))
        } catch {
          print("Skipping \(node.name) due to error: \(error)")
        }
      case CXCursor_EnumDecl:
        enums.insert(Enum(cursor: node))
      default:
        continue
      }
    }
  }
  
  for file in files {
    let idx = clang_createIndex(0, 1)
    defer { clang_disposeIndex(idx) }
    // Have to define "LEDGER_EXPORT" so we don't get parsing errors.
    // I assume its because we are parsing headers and not source files
    let args: [String] = ["-x", "c++", "-std=c++14", "-DLEDGER_EXPORT= ", "-iframework", "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks"] +
      (Config.systemIncludes.flatMap { ["-isystem", $0] }) +
      (includePaths.flatMap { ["-I", $0] })
    var unit: CXTranslationUnit!
    let errorCode = clang_parseTranslationUnit2(idx, file, args.map { ($0 as NSString).utf8String }, Int32(args.count), nil, 0, 0, &unit)
    if errorCode.rawValue != 0 {
      print("Couldn't parse \(file)")
      continue
    }
    
    _traverse(nodes: Cursor(clang_getTranslationUnitCursor(unit)).children)
  }
  
  let sortedInterfaces = interfaces.sorted()
  
  // We have to cheat a bit and force the export.h files to be at the top since a lot of files
  // in the ledger includes use `LEDGER_EXPORT` but not `#include "bat/ledger/export.h"
  let fudgedSortedFiles = files.sorted(by: { $0.contains("export.h") ? true : $0 < $1 })
  let cppIncludes: [String] = fudgedSortedFiles.map { filename in
    var updatedPath = filename
    includePaths.forEach {
      updatedPath = updatedPath.replacingOccurrences(of: $0, with: "")
    }
    if updatedPath.hasPrefix("/") {
      updatedPath = String(updatedPath.dropFirst())
    }
    return updatedPath
  }
  
  let outputedFiles: [TemplateOutput] = [
    EnumHeaderOutput(enums: enums.sorted()),
    RecordsHeaderOutput(interfaces: sortedInterfaces),
    PrivateRecordsHeaderOutput(interfaces: sortedInterfaces, cppIncludes: cppIncludes),
    ImplementationSourceOutput(interfaces: sortedInterfaces)
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

guard let libraryPath = ProcessInfo.processInfo.environment["BATLibraryPath"],
      let ledgerPath = ProcessInfo.processInfo.environment["BATLedgerPath"],
      let adsPath = ProcessInfo.processInfo.environment["BATAdsPath"] else {
  fatalError("Missing `BATLibraryPath` & `BATLedgerPath` from environment variables")
}

guard FileManager.default.fileExists(atPath: "/usr/local/opt/llvm/include/c++/v1") else {
  fatalError("This tool requires LLVM/Clang be downloaded at `/usr/local/opt/llvm`")
}

// Generate ledger files
do {
  let includePath = libraryPath.appending("/bat-native-ledger/include")
  let headersPath = includePath.appending("/bat/ledger")
  let filePaths = try! FileManager.default.contentsOfDirectory(atPath: headersPath)
    .filter { $0.hasSuffix(".h") }
    .map { return "\(headersPath)/\($0)" }
  let outputPath = ledgerPath.appending("/Generated")
  
  generate(from: filePaths, includePaths: [includePath], outputDirectory: outputPath)
  createBridge(from: "\(headersPath)/ledger_client.h", className: "LedgerClient", includePaths: [includePath], outputDirectory: outputPath)
}

// Generate ads bridge
do {
  let includePath = libraryPath.appending("/bat-native-ads/include")
  let headersPath = includePath.appending("/bat/ads")
//  let filePaths = try! FileManager.default.contentsOfDirectory(atPath: headersPath)
//    .filter { $0.hasSuffix(".h") }
//    .map { return "\(headersPath)/\($0)" }
  let outputPath = adsPath.appending("/Generated")
  
//  generate(from: filePaths, includePaths: [includePath], outputDirectory: outputPath)
  createBridge(from: "\(headersPath)/ads_client.h", className: "AdsClient", includePaths: [includePath], outputDirectory: outputPath)
}
