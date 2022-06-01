// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import PackagePlugin
import Foundation

@main
struct IntentBuilderPlugin: BuildToolPlugin {
  func createBuildCommands(context: PluginContext, target: Target) async throws -> [Command] {
    let outputDirectory = context.pluginWorkDirectory.appending("GeneratedSources")
    guard let target = target as? SourceModuleTarget else {
      Diagnostics.error("Attempted to use `IntentBuilderPlugin` on an unsupported module target")
      return []
    }
    return target.sourceFiles(withSuffix: "intentdefinition")
      .map { file in
        .prebuildCommand(
          displayName: "Generate intents sources",
          executable: Path("/usr/bin/xcrun"),
          arguments: [
            "intentbuilderc", "generate",
            "-input", file.path.string,
            "-output", outputDirectory,
            "-language", "Swift",
            "-swiftVersion", "5.6",
            "-classPrefix", ""
          ],
          outputFilesDirectory: outputDirectory
        )
      }
  }
}
