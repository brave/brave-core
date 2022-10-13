// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import MediaPlayer
import AVKit
import AVFoundation
import Shared
import Data
import os.log

class PlaylistPlayerStatusObserver: NSObject {
  private weak var player: AVPlayer?
  private var item: AVPlayerItem?
  private var onStatusChanged: (AVPlayerItem.Status) -> Void
  private var currentItemObserver: NSKeyValueObservation?

  init(player: AVPlayer, onStatusChanged: @escaping (AVPlayerItem.Status) -> Void) {
    self.onStatusChanged = onStatusChanged
    super.init()

    self.player = player
    currentItemObserver = player.observe(
      \AVPlayer.currentItem?.status, options: [.new],
      changeHandler: { [weak self] _, change in
        guard let self = self else { return }

        let status = change.newValue ?? .none
        switch status {
        case .readyToPlay:
          Logger.module.debug("Player Item Status: Ready")
          self.onStatusChanged(.readyToPlay)
        case .failed:
          Logger.module.debug("Player Item Status: Failed")
          self.onStatusChanged(.failed)
        case .unknown:
          Logger.module.debug("Player Item Status: Unknown")
          self.onStatusChanged(.unknown)
        case .none:
          Logger.module.debug("Player Item Status: None")
          self.onStatusChanged(.unknown)
        @unknown default:
          assertionFailure("Unknown Switch Case for AVPlayerItemStatus")
        }
      })
  }
}
