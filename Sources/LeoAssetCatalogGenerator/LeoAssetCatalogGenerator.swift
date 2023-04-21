// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// Generates an asset catalog for the Leo design system resources
///
/// For now only SF Symbols are added to the asset catalog based on `symbolsets` found in asset catalogs
/// belonging to the target
@main
struct LeoAssetCatalogGenerator {
  // Arguments: ./LeoAssetCatalogGenerator asset_catalog1[, asset_catalog2, ...] leo_symbols_directory output_directory
  static func main() throws {
    var arguments = ProcessInfo.processInfo.arguments
    if arguments.count < 4 {
      exit(EXIT_FAILURE)
    }
    let outputDirectory = URL(fileURLWithPath: arguments.popLast()!)
    let leoSymbolsDirectory = URL(fileURLWithPath: arguments.popLast()!)
    let assetCatalogs = arguments.dropFirst().map { URL(fileURLWithPath: $0) }
    
    let generator = LeoAssetCatalogGenerator(
      assetCatalogs: assetCatalogs,
      leoSymbolsDirectory: leoSymbolsDirectory,
      outputDirectory: outputDirectory
    )
    
    try generator.createAssetCatalog()
  }
  
  var assetCatalogs: [URL]
  var leoSymbolsDirectory: URL
  var outputDirectory: URL
  let fileManager = FileManager.default
  
  init(
    assetCatalogs: [URL],
    leoSymbolsDirectory: URL,
    outputDirectory: URL
  ) {
    self.assetCatalogs = assetCatalogs
    self.leoSymbolsDirectory = leoSymbolsDirectory
    self.outputDirectory = outputDirectory
  }
  
  func createAssetCatalog() throws {
    try fileManager.createDirectory(at: outputDirectory, withIntermediateDirectories: true)
    try assetCatalogContentsJSON.write(
      to: outputDirectory.appendingPathComponent("Contents.json"), atomically: true, encoding: .utf8
    )
    try createSymbolSets()
  }
  
  private var assetCatalogContentsJSON: String {
    """
    {
      "info" : {
        "author" : "xcode",
        "version" : 1
      }
    }
    """
  }
  
  // MARK: - SF Symbols
  
  func createSymbolSets() throws {
    for catalog in assetCatalogs {
      for symbol in symbolSets(in: catalog) {
        let symbolName = symbol.deletingPathExtension().lastPathComponent
        if try fileManager.contentsOfDirectory(atPath: symbol.path).contains(where: { $0.hasSuffix(".svg") }) {
          // We've specifically overridden this icon for some reason, so skipping for now
          print("Skipped copying Leo SF Symbol \"\(symbolName)\" from leo-sf-symbols. Using local version")
          continue
        }
        let symbolSetOutputDirectory = outputDirectory.appendingPathComponent("\(symbolName).symbolset")
        try fileManager.createDirectory(at: symbolSetOutputDirectory, withIntermediateDirectories: true)
        let leoSymbolSVGPath = leoSymbolsDirectory.appendingPathComponent("symbols/\(symbolName).svg").path
        print("leo symbol svg path: \(leoSymbolSVGPath)")
        if !fileManager.fileExists(atPath: leoSymbolSVGPath) {
          print("Couldn't find a Leo icon named \(symbolName).svg")
          exit(EXIT_FAILURE)
        }
        try symbolSetContentsJSON(filename: symbolName).write(
          to: symbolSetOutputDirectory.appendingPathComponent("Contents.json"), atomically: true, encoding: .utf8
        )
        let svgOutputDirectory = symbolSetOutputDirectory.appendingPathComponent("\(symbolName).svg")
        if fileManager.fileExists(atPath: svgOutputDirectory.path) {
          try fileManager.removeItem(at: svgOutputDirectory)
        }
        try fileManager.copyItem(at: URL(fileURLWithPath: leoSymbolSVGPath), to: svgOutputDirectory)
      }
    }
  }
  
  private func symbolSets(in catalog: URL) -> [URL] {
    var symbols: [URL] = []
    guard let enumerator = fileManager.enumerator(
      at: catalog,
      includingPropertiesForKeys: [.isDirectoryKey, .nameKey],
      options: [.skipsHiddenFiles, .skipsSubdirectoryDescendants]
    ) else { return [] }
    while let fileURL = enumerator.nextObject() as? URL {
      guard
        let values = try? fileURL.resourceValues(forKeys: [.isDirectoryKey, .nameKey]),
        let isDirectory = values.isDirectory,
        let name = values.name,
        isDirectory,
        name.hasPrefix("leo"),
        name.hasSuffix(".symbolset") else {
        continue
      }
      symbols.append(fileURL)
    }
    return symbols
  }
  
  private func symbolSetContentsJSON(filename: String) -> String {
    """
    {
      "info" : {
        "author" : "xcode",
        "version" : 1
      },
      "symbols" : [
        {
          "filename" : "\(filename).svg",
          "idiom" : "universal"
        }
      ]
    }
    """
  }
}
