// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import AVFoundation

class PlaylistAssetFetcher {
  let itemId: String
  private let asset: AVURLAsset
  
  init(itemId: String, asset: AVURLAsset) {
    self.itemId = itemId
    self.asset = asset
  }
  
  func cancelLoading() {
    asset.cancelLoading()
  }
}
