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
    let leoSymbolsDirectory = context.package.directory.appending("node_modules/leo-sf-symbols")
    
    if !fileManager.fileExists(atPath: leoSymbolsDirectory.string) {
      Diagnostics.error("Leo SF Symbols not found: \(FileManager.default.currentDirectoryPath)")
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
    
    let outputDirectory = context.pluginWorkDirectory.appending("LeoAssets.xcassets")
    
    return [
      .buildCommand(
        displayName: "Create Asset Catalog",
        executable: try context.tool(named: "LeoAssetCatalogGenerator").path,
        arguments: assetCatalogs + [leoSymbolsDirectory, outputDirectory.string],
        inputFiles: assetCatalogs + [leoSymbolsDirectory.appending("package.json")],
        outputFiles: [outputDirectory]
      ),
    ]
  }
}

#if canImport(XcodeProjectPlugin)
import XcodeProjectPlugin

extension LeoAssetsPlugin: XcodeBuildToolPlugin {
  // Entry point for creating build commands for targets in Xcode projects.
  func createBuildCommands(context: XcodePluginContext, target: XcodeTarget) throws -> [Command] {
    // Check to make sure we have pulled down the icons correctly
    let fileManager = FileManager.default
    // Xcode project is inside App folder so we have to go backwards a level to get the leo-sf-symbols
    let leoSymbolsDirectory = context.xcodeProject.directory.removingLastComponent()
      .appending("node_modules/leo-sf-symbols")
    if !fileManager.fileExists(atPath: leoSymbolsDirectory.string) {
      Diagnostics.error("Leo SF Symbols not found: \(FileManager.default.currentDirectoryPath)")
      return []
    }
    
    let assetCatalogs = target.inputFiles
      .filter { $0.type == .resource && $0.path.extension == "xcassets" }
      .map(\.path)
    if assetCatalogs.isEmpty {
      Diagnostics.error("No asset catalogs found in the target")
      return []
    }
    
    let outputDirectory = context.pluginWorkDirectory.appending("LeoAssets.xcassets")
    
    return [
      .buildCommand(
        displayName: "Create Asset Catalog",
        executable: try context.tool(named: "LeoAssetCatalogGenerator").path,
        arguments: assetCatalogs + [leoSymbolsDirectory, outputDirectory.string],
        inputFiles: assetCatalogs + [leoSymbolsDirectory.appending("package.json")],
        outputFiles: [outputDirectory]
      ),
    ]
  }
}
#endif
