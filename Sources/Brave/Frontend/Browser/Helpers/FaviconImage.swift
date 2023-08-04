// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Shared
import BraveShared
import Data
import BraveUI
import Favicon

private class FaviconHelper: ObservableObject {
  @Published var image: UIImage?
  private var faviconTask: Task<Void, Error>?

  func load(url: URL, isPrivateBrowsing: Bool) {
    faviconTask?.cancel()
    faviconTask = Task { @MainActor in
      let favicon = try await FaviconFetcher.loadIcon(url: url, persistent: !isPrivateBrowsing)
      self.image = favicon.image
    }
  }
}

struct FaviconImage: View {
  let url: URL?
  private let isPrivateBrowsing: Bool
  @StateObject private var faviconLoader = FaviconHelper()
  
  init(url: String?, isPrivateBrowsing: Bool) {
    self.isPrivateBrowsing = isPrivateBrowsing
    
    if let url = url {
      self.url = URL(string: url)
    } else {
      self.url = nil
    }
  }
  
  var body: some View {
    Image(uiImage: faviconLoader.image ?? .init(named: "defaultFavicon", in: .module, compatibleWith: nil)!)
      .resizable()
      .aspectRatio(contentMode: .fit)
      .frame(width: 30, height: 30)
      .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
      .onAppear {
        if let url = url {
          faviconLoader.load(url: url, isPrivateBrowsing: isPrivateBrowsing)
        }
      }
  }
}
