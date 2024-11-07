// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import PackagePlugin

/// Creates an asset catalog filled with Brave's Leo SF Symbols
@main
struct LeoAssetsPlugin: BuildToolPlugin {

  func createBuildCommands(context: PluginContext, target: Target) async throws -> [Command] {
    // Check to make sure we have pulled down the icons correctly
    let fileManager = FileManager.default
    let braveCoreRootDirectory = context.package.directory.removingLastComponent()
      .removingLastComponent()
    let leoColorsDirectory = braveCoreRootDirectory.appending("node_modules/@brave/leo")

    if !fileManager.fileExists(atPath: leoColorsDirectory.string) {
      Diagnostics.error(
        "Required Leo assets not found: \(FileManager.default.currentDirectoryPath)"
      )
      return []
    }

    // Check to make sure the plugin is being used correctly in SPM
    guard let target = target as? SourceModuleTarget else {
      Diagnostics.error("Attempted to use `LeoAssetsPlugin` on an unsupported module target")
      return []
    }

    let copyColorsCommand: Command = {
      let tokensPath = leoColorsDirectory.appending("tokens/ios-swift")
      let outputDirectory = context.pluginWorkDirectory.appending("LeoColors")
      return .buildCommand(
        displayName: "Copy Leo Colors",
        executable: Path("/bin/zsh"),
        arguments: [
          "-c", "find \"\(tokensPath)\" -name \\*.swift -exec cp {} \"\(outputDirectory)\" \\;",
        ],
        inputFiles: [
          tokensPath.appending("Gradients.swift"),
          tokensPath.appending("ColorSetAccessors.swift"),
        ],
        outputFiles: [
          outputDirectory.appending("Gradients.swift"),
          outputDirectory.appending("ColorSetAccessors.swift"),
        ]
      )
    }()

    return [copyColorsCommand]
  }
}
