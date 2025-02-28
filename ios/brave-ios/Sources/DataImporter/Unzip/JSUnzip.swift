// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import WebKit

class JSUnzip {
  static func unzip(_ zipPath: String, toDirectory destinationDirectory: String) async -> Bool {
    guard let jsPath = Unzip.jszipScriptPath else {
      return false
    }

    if let zipData = await AsyncFileManager.default.contents(atPath: zipPath)?.base64EncodedString,
      let jsZipData = await AsyncFileManager.default.contents(atPath: jsPath),
      let jsZipString = String(data: jsZipData, encoding: .utf8)
    {
      let script = """
        var p = JSZip.loadAsync("\(zipData)", { base64: true }).then(function (zip) {
            var promises = [];
            
            zip.forEach(function (relativePath, zipEntry) {
                promises.push(zipEntry.async("base64").then(data => ({ [relativePath]: data })));
            });

            return Promise.all(promises).then(entries => {
                return Object.assign({}, ...entries);
            });
        });

        await p;
        return p;
        """

      return await withTaskGroup(of: Bool.self, returning: Bool.self) { group in
        let webView = await MainActor.run {
          WKWebView(frame: .init(width: 0, height: 0), configuration: .init())
        }

        let result: Any?
        do {
          result = try await webView.callAsyncJavaScript(
            "\(jsZipString)\n\n\(script)",
            contentWorld: .defaultClient
          )
        } catch {
          return false
        }

        guard let result = result as? [String: String] else {
          return false
        }

        for (relativePath, base64Data) in result {
          group.addTask {
            let destinationURL = URL(fileURLWithPath: destinationDirectory)
              .appendingPathComponent(JSUnzip.escapePath(relativePath: relativePath))

            if base64Data.isEmpty {
              // Usually, if there's no base64Data, it's a DIRECTORY, so we'd create it
              // But the code below, will already create "Intermediate" directories
              // So we can ignore it and return true here
              return true
            }

            if let data = Data(base64Encoded: base64Data) {
              do {
                try await AsyncFileManager.default.createDirectory(
                  at: destinationURL.deletingLastPathComponent(),
                  withIntermediateDirectories: true,
                  attributes: nil
                )

                return await AsyncFileManager.default.createFile(
                  atPath: destinationURL.path,
                  contents: data,
                  attributes: nil
                )
              } catch {
                return false
              }
            }
            return false
          }
        }

        return await group.reduce(true) { $0 && $1 }
      }
    }

    return false
  }

  // Escapes the path with the same rules as in Chromium:
  // https://source.chromium.org/chromium/chromium/src/+/main:third_party/zlib/google/zip_reader.h;l=109
  private static func escapePath(relativePath: String) -> String {
    let components = relativePath.split(separator: "/").map { String($0) }
    var escapedComponents: [String] = []

    for component in components {
      if component == ".." {
        escapedComponents.append("UP")
      } else if component == "." {
        escapedComponents.append("DOT")
      } else if component.isEmpty {
        continue
      } else {
        escapedComponents.append(component)
      }
    }

    return escapedComponents.joined(separator: "/")
  }
}
