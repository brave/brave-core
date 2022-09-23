// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import PackagePlugin
import Foundation

/// Creates a bundle accessor that functions the same as `Bundle.module`, however works when running within
/// a SwiftUI Preview
///
/// Immediately creates a `current_bundle_accessor.swift` file in the GeneratedSources folder
@main
struct CurrentBundleGenPlugin: BuildToolPlugin {
  func createBuildCommands(context: PluginContext, target: Target) async throws -> [Command] {
    let outputDirectory = context.pluginWorkDirectory.appending("GeneratedSources")
    guard let target = target as? SourceModuleTarget,
          !target.sourceFiles.filter({ $0.type == .resource }).isEmpty else {
      Diagnostics.error("Attempted to use `CurrentBundleGenPlugin` on an unsupported module target")
      return []
    }
    try FileManager.default.createDirectory(atPath: outputDirectory.string, withIntermediateDirectories: true)
    let source = """
    import Foundation
    
    extension Bundle {
      /// Returns the resource bundle associated with the current Swift module.
      static var current: Bundle {
    #if DEBUG
        if ProcessInfo.processInfo.environment[\"XCODE_RUNNING_FOR_PREVIEWS\"] != \"1\" {
          return .module
        }
        
        let bundleName = \"\(context.package.displayName)_\(target.name)\"
        let candidates = [
          // Bundle should be present here when the package is linked into an App.
          Bundle.main.resourceURL,
          // Bundle should be present here when the package is linked into a framework.
          Bundle(for: BundleFinder.self).resourceURL,
          // For command-line tools.
          Bundle.main.bundleURL,
          // Bundle should be present in the build artifacts directory when linked into a SwiftUI Preview
          Bundle(for: BundleFinder.self).resourceURL?.deletingLastPathComponent(),
          Bundle(for: BundleFinder.self).resourceURL?.deletingLastPathComponent().deletingLastPathComponent()
        ]
        
        for candidate in candidates {
        let bundlePath = candidate?.appendingPathComponent(\"\\(bundleName).bundle\")
          if let bundle = bundlePath.flatMap(Bundle.init(url:)) {
            return bundle
          }
        }
        fatalError(\"unable to find bundle named \\(bundleName)\")
    #else
        return .module
    #endif
      }
      
      private class BundleFinder {}
    }
    """
    let filePath = outputDirectory.appending("current_bundle_accessor.swift")
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
        displayName: "Generate bundle accessor",
        executable: Path("/bin/zsh"),
        arguments: [],
        inputFiles: [],
        outputFiles: [filePath]
      )
    ]
  }
}
