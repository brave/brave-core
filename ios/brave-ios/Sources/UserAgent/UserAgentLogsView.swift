// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import OSLog
import SwiftUI
import os.log

public struct UserAgentLogsView: View {
  @State private var logs: String = ""
  @State private var isLoading: Bool = true
  @State private var showShareSheet: Bool = false
  @State private var fileURL: URL?

  public init() {}

  public var body: some View {
    VStack {
      if isLoading {
        ProgressView()
      } else {
        ScrollView {
          Text(logs.isEmpty ? "No logs" : logs)
            .padding()
        }
      }
    }
    .toolbar {
      ToolbarItemGroup(placement: .topBarTrailing) {
        ShareLink(item: fileURL ?? URL(string: "disabled")!)
          .disabled(fileURL == nil || logs.isEmpty)
      }
    }
    .task {
      logs = await getLogs()
      fileURL = createTemporaryLogFile()
      isLoading = false
    }
  }

  private func createTemporaryLogFile() -> URL {
    let temporaryDirectoryURL = FileManager.default.temporaryDirectory
    let logFileURL = temporaryDirectoryURL.appendingPathComponent("Logs.txt")

    try? logs.write(to: logFileURL, atomically: true, encoding: .utf8)
    return logFileURL
  }

  private func getLogs() async -> String {
    return await Task.detached(priority: .background) {
      do {
        let store = try OSLogStore(scope: .currentProcessIdentifier)

        return
          try store
          .getEntries()
          .compactMap { $0 as? OSLogEntryLog }
          .filter { $0.category == "UserAgent" && $0.subsystem == Bundle.main.bundleIdentifier }
          .map { "[\($0.date.formatted())] \($0.composedMessage)" }
          .joined(separator: "\n")
      } catch {
        await MainActor.run {
          Logger.module.error("\(error.localizedDescription, privacy: .public)")
        }
      }
      return ""
    }.value
  }
}
