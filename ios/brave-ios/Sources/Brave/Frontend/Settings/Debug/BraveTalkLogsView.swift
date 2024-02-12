// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import os.log
import OSLog

struct BraveTalkLogsView: View {
  @State private var logs: String = ""
  @State private var isLoading: Bool = true
  @State private var showShareSheet: Bool = false
  @State private var fileURL: URL?
  
  var body: some View {
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
    .navigationBarItems(trailing: ShareButton())
    .task {
      logs = await getLogs()
      fileURL = createTemporaryLogFile()
      isLoading = false
    }
  }
  
  @ViewBuilder private func ShareButton() -> some View {
    if #available(iOS 16, *) {
      ShareLink(item: fileURL ?? URL(string: "disabled")!)
        .disabled(fileURL == nil || logs.isEmpty)
    } else {
      EmptyView()
    }
  }
  
  private func createTemporaryLogFile() -> URL {
    let temporaryDirectoryURL = FileManager.default.temporaryDirectory
    let logFileURL = temporaryDirectoryURL.appendingPathComponent("Logs.txt")
    
    try? logs.write(to: logFileURL, atomically: true, encoding: .utf8)
    return logFileURL
  }
  
  private func getLogs() async -> String {
    do {
      let store = try OSLogStore(scope: .currentProcessIdentifier)
      
      return try store
        .getEntries()
        .compactMap { $0 as? OSLogEntryLog }
        .filter { $0.category == "BraveTalk" && $0.subsystem == Bundle.main.bundleIdentifier }
        .map { "[\($0.date.formatted())] \($0.composedMessage)" }
        .joined(separator: "\n")
    } catch {
      Logger.module.error("\(error.localizedDescription, privacy: .public)")
    }
    
    return ""
  }
}

#if DEBUG
struct BraveTalkLogsView_Previews: PreviewProvider {
  static var previews: some View {
    BraveTalkLogsView()
  }
}
#endif
