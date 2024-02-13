// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

/// Displays the contents of the Chromium `Local State` file
struct LocalStateInspectorView: View {
  @State private var localState: String = ""
  var body: some View {
    TextEditor(text: .constant(localState))
      .navigationTitle("Local State")
      .font(.system(.footnote, design: .monospaced))
      .onAppear {
        loadLocalState()
      }
      .toolbar {
        ToolbarItemGroup(placement: .navigationBarTrailing) {
          Button {
            loadLocalState()
          } label: {
            Label("Refresh", braveSystemImage: "leo.browser.refresh")
              .imageScale(.medium)
          }
        }
      }
  }
  
  private func loadLocalState() {
    do {
      let jsonContents = try Data(
        contentsOf: FileManager.default.url(
          for: .applicationSupportDirectory,
          in: .userDomainMask,
          appropriateFor: nil,
          create: true
        ).appendingPathComponent("Chromium/Local State")
      )
      let jsonObject = try JSONSerialization.jsonObject(with: jsonContents)
      let prettyPrintedData = try JSONSerialization.data(
        withJSONObject: jsonObject,
        options: [.prettyPrinted]
      )
      localState = String(data: prettyPrintedData, encoding: .utf8) ?? ""
    } catch {
      localState = "Failed to load Local State"
    }
  }
}
