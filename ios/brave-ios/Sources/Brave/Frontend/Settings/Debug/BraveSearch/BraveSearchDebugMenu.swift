// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import SwiftUI
import WebKit

struct BraveSearchDebugMenu: View {

  @ObservedObject var logging: BraveSearchLogEntry

  @State private var cookies: [HTTPCookie] = []
  @State private var storageTypes: [String] = []

  var body: some View {
    Form {
      Section {
        Toggle("Enable callback logging", isOn: $logging.isEnabled)
          .toggleStyle(SwitchToggleStyle(tint: .accentColor))
      }

      Section(header: Text(verbatim: "Logs")) {
        ForEach(logging.logs) { logEntry in
          NavigationLink(destination: BraveSearchDebugMenuDetail(logEntry: logEntry)) {
            VStack(alignment: .leading) {
              Text(formattedDate(logEntry.date))
                .font(.caption)
              Text(logEntry.query)
                .font(.body)
            }
          }
        }
      }

      Section(header: Text(verbatim: "cookies")) {
        ForEach(cookies, id: \.name) { cookie in
          Text(String(describing: cookie))
        }
      }

      Section(header: Text(verbatim: "storage found for brave.com")) {
        Text(String(describing: storageTypes))
      }
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .onAppear(perform: loadRecords)
  }

  private func formattedDate(_ date: Date) -> String {
    let dateFormatter = DateFormatter()
    dateFormatter.dateStyle = .short
    dateFormatter.timeStyle = .short
    return dateFormatter.string(from: date)
  }

  private func loadRecords() {
    let eligibleDomains =
      [
        "search.brave.com", "search.brave.software", "search.bravesoftware.com",
        "safesearch.brave.com", "safesearch.brave.software",
        "safesearch.bravesoftware.com", "search-dev-local.brave.com",
      ]
    WKWebsiteDataStore.default().httpCookieStore.getAllCookies { cookies in
      self.cookies = cookies.filter {
        eligibleDomains.contains($0.domain)
      }

    }

    let eligibleStorageDomains =
      ["brave.com", "bravesoftware.com", "brave.software"]
    WKWebsiteDataStore.default()
      .fetchDataRecords(
        ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(),
        completionHandler: { records in
          storageTypes =
            records
            .filter { eligibleStorageDomains.contains($0.displayName) }
            .flatMap { $0.dataTypes }
        }
      )
  }
}

#if DEBUG
struct BraveSearchDebugMenu_Previews: PreviewProvider {
  static var previews: some View {
    BraveSearchDebugMenu(logging: BraveSearchDebugMenuFixture.loggingSample)
  }
}
#endif
