// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

@available(iOS 16.0, *)
struct ShareOrCopyURLView: View {
  let url: URL
  
  var body: some View {
    HStack {
      ShareLink(item: url)
        .labelStyle(.iconOnly)
        .buttonStyle(.borderedProminent)
        .foregroundStyle(.white)
        .font(.caption)
      
      #if targetEnvironment(simulator)
      Button {
        UIPasteboard.general.string = url.path(percentEncoded: false)
      } label: {
        Label("Copy", systemImage: "doc.on.clipboard")
      }
      .labelStyle(.iconOnly)
      .buttonStyle(.borderedProminent)
      .foregroundStyle(.white)
      .font(.caption)
      #endif
    }
  }
}

#if swift(>=5.9)
@available(iOS 16.0, *)
#Preview {
  ShareOrCopyURLView(url: URL(string: "https://example.com")!)
}
#endif
