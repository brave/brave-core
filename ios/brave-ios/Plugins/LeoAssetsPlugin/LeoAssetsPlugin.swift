// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import PackagePlugin
import Foundation

/// Creates an asset catalog filled with Brave's Leo SF Symbols
@main
struct LeoAssetsPlugin: BuildToolPlugin {
  
  func createBuildCommands(context: PluginContext, target: Target) async throws -> [Command] {
    // Check to make sure we have pulled down the icons correctly
    let fileManager = FileManager.default
    let braveCoreRootDirectory = context.package.directory.removingLastComponent().removingLastComponent()
    let leoSymbolsDirectory = braveCoreRootDirectory.appending("node_modules/@brave/leo-sf-symbols")
    let leoColorsDirectory = braveCoreRootDirectory.appending("node_modules/@brave/leo")

    if !fileManager.fileExists(atPath: leoSymbolsDirectory.string) ||
        !fileManager.fileExists(atPath: leoColorsDirectory.string) {
      Diagnostics.error("Required Leo assets not found: \(FileManager.default.currentDirectoryPath)")
      return []
    }
    
    // Check to make sure the plugin is being used correctly in SPM
    guard let target = target as? SourceModuleTarget else {
      Diagnostics.error("Attempted to use `LeoAssetsPlugin` on an unsupported module target")
      return []
    }
    
    let assetCatalogs = Array(target.sourceFiles(withSuffix: "xcassets").map(\.path))
    if assetCatalogs.isEmpty {
      Diagnostics.error("No asset catalogs found in the target")
      return []
    }
    
    // The command that will create an asset catalog full of leo sf symbols
    let copySFSymbolsCommand: Command = try {
      let scriptPath = context.package.directory.appending("Plugins/LeoAssetsPlugin/make_asset_catalog.sh")
      let outputDirectory = context.pluginWorkDirectory.appending("LeoAssets.xcassets")
      
      // Running a macOS tool while archiving a iOS build is unfortunately broken in Xcode 14. It attempts to
      // run the macOS tool from the wrong directory and fails to find it. One way around this is to use a
      // precompiled version of this tool instead, but it will mean building and uploading them somewhere
      // so for now the tool will be replaced by a bash script. We can uncomment this and add back the dep on
      // `LeoAssetCatalogGenerator` once this is fixed in Xcode.
      //    return [
      //      .buildCommand(
      //        displayName: "Create Asset Catalog",
      //        executable: try context.tool(named: "LeoAssetCatalogGenerator").path,
      //        arguments: assetCatalogs + [leoSymbolsDirectory, outputDirectory.string],
      //        inputFiles: assetCatalogs + [leoSymbolsDirectory.appending("package.json")],
      //        outputFiles: [outputDirectory]
      //      ),
      //    ]
      let icons = try assetCatalogs.flatMap {
        try symbolSets(in: URL(fileURLWithPath: $0.string))
      }.joined(separator: ",")
      return .buildCommand(
        displayName: "Create Asset Catalog",
        executable: Path("/bin/zsh"),
        arguments: [
          scriptPath.string,
          "-l", leoSymbolsDirectory.string,
          "-i", icons,
          "-o", context.pluginWorkDirectory.string
        ],
        inputFiles: assetCatalogs + [leoSymbolsDirectory.appending("package.json"),
                                     scriptPath],
        outputFiles: [outputDirectory]
      )
    }()
    
    let copyColorsCommand: Command = {
      let tokensPath = leoColorsDirectory.appending("tokens/ios-swift")
      let outputDirectory = context.pluginWorkDirectory.appending("LeoColors")
      return .buildCommand(
        displayName: "Copy Leo Colors",
        executable: Path("/bin/zsh"),
        arguments: [
          "-c", "cp -R \"\(tokensPath.string)/.\" \"\(outputDirectory)\""
        ],
        inputFiles: [
          tokensPath.appending("Colors.xcassets"),
          tokensPath.appending("Gradients.swift"),
          tokensPath.appending("ColorSetAccessors.swift"),
        ],
        outputFiles: [
          outputDirectory.appending("Colors.xcassets"),
          outputDirectory.appending("Gradients.swift"),
          outputDirectory.appending("ColorSetAccessors.swift"),
        ]
      )
    }()
    
    return [
      copySFSymbolsCommand,
      copyColorsCommand
    ]
  }
}

extension LeoAssetsPlugin {
  fileprivate func symbolSets(in catalog: URL) throws -> [String] {
    let fileManager = FileManager.default
    var symbols: [String] = []
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
        name.hasSuffix(".symbolset"),
        !(try fileManager.contentsOfDirectory(atPath: fileURL.path)
          .contains(where: { $0.hasSuffix("svg") }))
      else {
        continue
      }
      symbols.append(fileURL.deletingPathExtension().lastPathComponent)
    }
    return symbols
  }
}
