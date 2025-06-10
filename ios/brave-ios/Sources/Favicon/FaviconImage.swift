// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Shared
import SwiftUI

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

public struct FaviconImage: View {
  var url: URL
  var isPrivateBrowsing: Bool
  @StateObject private var faviconLoader = FaviconHelper()

  public init(url: URL, isPrivateBrowsing: Bool) {
    self.isPrivateBrowsing = isPrivateBrowsing
    self.url = url
  }

  public var body: some View {
    Image(uiImage: faviconLoader.image ?? Favicon.defaultImage)
      .resizable()
      .aspectRatio(contentMode: .fit)
      .task(id: url) {
        if ProcessInfo.processInfo.environment["XCODE_RUNNING_FOR_PREVIEWS"] != nil {
          return
        }
        faviconLoader.load(url: url, isPrivateBrowsing: isPrivateBrowsing)
      }
  }
}

public struct StyledFaviconImage: View {
  var url: String?
  var isPrivateBrowsing: Bool

  public init(url: String?, isPrivateBrowsing: Bool) {
    self.url = url
    self.isPrivateBrowsing = isPrivateBrowsing
  }

  public var body: some View {
    Group {
      if let urlString = url, let url = URL(string: urlString) {
        FaviconImage(url: url, isPrivateBrowsing: isPrivateBrowsing)
      } else {
        Color.clear
      }
    }
    .frame(width: 30, height: 30)
    .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
  }
}
