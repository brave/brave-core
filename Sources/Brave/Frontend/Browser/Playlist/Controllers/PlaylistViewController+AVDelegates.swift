// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import AVKit
import AVFoundation
import Shared

// MARK: - AVPictureInPictureControllerDelegate

extension PlaylistViewController: AVPictureInPictureControllerDelegate {

  func pictureInPictureControllerWillStartPictureInPicture(_ pictureInPictureController: AVPictureInPictureController) {

    PlaylistCarplayManager.shared.playlistController = self

    if UIDevice.isIpad {
      self.dismiss(animated: true, completion: nil)
    }
  }

  func pictureInPictureControllerDidStartPictureInPicture(_ pictureInPictureController: AVPictureInPictureController) {
    if UIDevice.isPhone {
      DispatchQueue.main.async {
        self.dismiss(animated: true, completion: nil)
      }
    }
  }

  func pictureInPictureControllerDidStopPictureInPicture(_ pictureInPictureController: AVPictureInPictureController) {
    PlaylistCarplayManager.shared.playlistController = nil
  }

  func pictureInPictureController(_ pictureInPictureController: AVPictureInPictureController, failedToStartPictureInPictureWithError error: Error) {

    let alert = UIAlertController(
      title: Strings.PlayList.sorryAlertTitle,
      message: Strings.PlayList.pictureInPictureErrorTitle, preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: Strings.PlayList.okayButtonTitle, style: .default, handler: nil))
    self.present(alert, animated: true, completion: nil)
  }

  func pictureInPictureController(_ pictureInPictureController: AVPictureInPictureController, restoreUserInterfaceForPictureInPictureStopWithCompletionHandler completionHandler: @escaping (Bool) -> Void) {

    if let restorationController = PlaylistCarplayManager.shared.playlistController {
      restorationController.modalPresentationStyle = .fullScreen
      guard let browserViewController = view.window == nil ? PlaylistCarplayManager.shared.browserController : self.currentScene?.browserViewController else {
        completionHandler(true)
        return
      }

      // There is a case when the user can manually present the PlaylistController through 3-dot menu
      // or URL bar. In that case, attempting to present the same controller WILL crash the application
      // So we need to guard against it.
      if restorationController.presentingViewController != nil || restorationController.isMovingToParent || restorationController.isBeingPresented {
        DispatchQueue.main.async {
          PlaylistCarplayManager.shared.playlistController = nil
          completionHandler(true)
        }
        return
      }

      browserViewController.present(restorationController, animated: true) {
        DispatchQueue.main.async {
          PlaylistCarplayManager.shared.playlistController = nil
          completionHandler(true)
        }
      }
    } else {
      completionHandler(true)
    }
  }
}
