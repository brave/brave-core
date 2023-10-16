// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import DesignSystem

@available(iOS 16.0, *)
struct ResourcesToolsView: View {
  let resourcesInfo: CachedAdBlockEngine.ResourcesInfo
  @State private var isCopied = false
  
  var body: some View {
    List {
      Section("File") {
        VStack(alignment: .leading) {
          HStack {
            Label("URL", systemImage: "doc.plaintext")
            HStack {
              Text("v") + Text(resourcesInfo.version)
            }.foregroundStyle(.secondary)
            
            Spacer()
            ShareOrCopyURLView(url: resourcesInfo.localFileURL)
          }
          
          Text(resourcesInfo.localFileURL.path(percentEncoded: false))
            .foregroundStyle(.secondary)
            .font(.caption)
        }
      }.listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle("Resources")
  }
}
