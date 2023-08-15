// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Shared
import BraveShared
import BraveUI
import os.log

struct BraveSearchDebugMenuDetail: View {
  let logEntry: BraveSearchLogEntry.FallbackLogEntry

  @State private var showingSheet = false

  var body: some View {
    Form {
      ValueRow(title: "URL", value: logEntry.url.absoluteString)

      Section(header: Text(verbatim: "/can/answer")) {
        ValueRow(title: "Cookies", value: cookieNames)
        ValueRow(title: "Time taken(s)", value: logEntry.canAnswerTime ?? "-")
        ValueRow(title: "Response", value: logEntry.backupQuery ?? "-")
      }

      Section(header: Text(verbatim: "Search Fallback")) {
        ValueRow(title: "Time taken(s)", value: logEntry.fallbackTime ?? "-")

        Button("Export response") {
          showingSheet.toggle()
        }
        .disabled(logEntry.fallbackData == nil)
        .background(
          ActivityView(isPresented: $showingSheet, activityItems: [dataAsUrl].compactMap { $0 })
        )
      }
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
  }

  private var cookieNames: String {
    logEntry.cookies.map {
      "\($0.name): \($0.value)"
    }
    .joined(separator: ", ")
  }

  private var dataAsUrl: URL? {
    guard let data = logEntry.fallbackData else { return nil }
    let tempUrl = FileManager.default.temporaryDirectory.appendingPathComponent("output.html")

    do {
      try data.write(to: tempUrl)
      return tempUrl
    } catch {
      Logger.module.error("data write-to error")
      return nil
    }
  }

  private struct ValueRow: View {
    var title: LocalizedStringKey
    var value: String

    var body: some View {
      HStack {
        Text(title)
        Spacer()
        Text(value)
      }
    }
  }
}

#if DEBUG
struct BraveSearchDebugMenuDetail_Previews: PreviewProvider {
  static var previews: some View {
    BraveSearchDebugMenuDetail(
      logEntry: BraveSearchDebugMenuFixture.sample)
  }
}
#endif
