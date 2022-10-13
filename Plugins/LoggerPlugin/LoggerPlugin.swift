// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import PackagePlugin
import Foundation

@main
struct LoggerPlugin: BuildToolPlugin {
  func createBuildCommands(context: PluginContext, target: Target) async throws -> [Command] {
    let outputDirectory = context.pluginWorkDirectory.appending("GeneratedSources")
    guard let target = target as? SourceModuleTarget else {
      Diagnostics.error("Attempted to use `LoggerPlugin` on an unsupported module target")
      return []
    }
    
    try FileManager.default.createDirectory(atPath: outputDirectory.string, withIntermediateDirectories: true)
    let source = """
    import Foundation
    import os.log
    
    extension Logger {
      static var module: Logger {
        .init(subsystem: "\\(Bundle.main.bundleIdentifier ?? "com.brave.ios")", category: "\(target.moduleName)")
      }
    }
    """
    
    let filePath = outputDirectory.appending("logger.swift")
    if !FileManager.default.fileExists(atPath: filePath.string) {
      try source.write(
        toFile: filePath.string,
        atomically: true,
        encoding: .utf8
      )
    }
    // TODO: Generate the above file in an `executableTarget` when SPM supports building Mac tools while targetting iOS
    return [
      .buildCommand(
        displayName: "Generate logger",
        executable: Path("/bin/zsh"),
        arguments: [],
        outputFiles: [outputDirectory.appending("logger.swift")]
      )
    ]
  }
}
