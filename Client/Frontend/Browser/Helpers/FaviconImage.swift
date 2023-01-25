// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Shared
import BraveShared
import Data
import BraveUI

struct FaviconImage: View {
  let url: URL?
  
  // FIXME: Generalize the playlist favicon loader.
  @StateObject private var faviconLoader = PlaylistFolderImageLoader()
  
  init(url: String?) {
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
          faviconLoader.load(domainUrl: url)
        }
      }
  }
}
