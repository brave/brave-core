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
    let braveCoreRootDirectory = context.package.directoryURL.deletingLastPathComponent()
      .deletingLastPathComponent()
    let leoColorsDirectory = braveCoreRootDirectory.appendingPathComponent("node_modules/@brave/leo")

    if !fileManager.fileExists(atPath: leoColorsDirectory.path) {
      Diagnostics.error(
        "Required Leo assets not found: \(FileManager.default.currentDirectoryPath)"
      )
      return []
    }

    // Check to make sure the plugin is being used correctly in SPM
    guard target is SourceModuleTarget else {
      Diagnostics.error("Attempted to use `LeoAssetsPlugin` on an unsupported module target")
      return []
    }

    let copyColorsCommand: Command = {
      let tokensPath = leoColorsDirectory.appendingPathComponent("tokens/ios-swift")
      let outputDirectory = context.pluginWorkDirectoryURL.appendingPathComponent("LeoColors")
      return .buildCommand(
        displayName: "Copy Leo Colors",
        executable: URL(fileURLWithPath: "/bin/zsh"),
        arguments: [
          "-c", "find \"\(tokensPath.path)\" -name \\*.swift -exec cp {} \"\(outputDirectory.path)\" \\;",
        ],
        inputFiles: [
          tokensPath.appendingPathComponent("Gradients.swift"),
          tokensPath.appendingPathComponent("ColorSetAccessors.swift"),
        ],
        outputFiles: [
          outputDirectory.appendingPathComponent("Gradients.swift"),
          outputDirectory.appendingPathComponent("ColorSetAccessors.swift"),
        ]
      )
    }()

    return [copyColorsCommand]
  }
}
