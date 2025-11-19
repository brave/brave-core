// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AIChat
import BraveCore
import BraveUI
import Foundation
import PhotosUI
import SpeechRecognition
import UIKit
import Web

extension BrowserViewController {
  func handleAIChatWebUIPageAction(_ tab: any TabState, action: AIChatWebUIPageAction) {
    switch action {
    case .handleVoiceRecognitionRequest(let completion):
      handleVoiceRecognitionRequest(completion)
    case .handleFileUploadRequest(_, let completion):
      completion(nil)
    case .presentSettings:
      break
    case .presentPremiumPaywall:
      break
    case .presentManagePremium:
      break
    case .closeTab:
      break
    case .openURL:
      break
    }
  }

  private func handleVoiceRecognitionRequest(_ completion: @escaping (String?) -> Void) {
    if !speechRecognizer.isVoiceSearchAvailable {
      completion(nil)
      return
    }

    // These are the same properties used for the standard search by voice feature on the toolbar
    onPendingRequestUpdatedCancellable = speechRecognizer.$finalizedRecognition.sink {
      [weak self] finalizedRecognition in
      guard let self else {
        completion(finalizedRecognition)
        return
      }

      if let finalizedRecognition {
        // Feedback indicating recognition is finalized
        AudioServicesPlayAlertSound(SystemSoundID(kSystemSoundID_Vibrate))
        UIImpactFeedbackGenerator(style: .medium).vibrate()

        voiceSearchViewController?.dismiss(animated: true) {
          self.speechRecognizer.clearSearch()
          completion(finalizedRecognition)
        }
      }
    }

    Task { @MainActor in
      if await SpeechRecognizer.requestPermission() {
        // Pause active playing in PiP when Audio Search is enabled
        if PlaylistCoordinator.shared.isPictureInPictureActive {
          PlaylistCoordinator.shared.pauseAllPlayback()
        }

        voiceSearchViewController = PopupViewController(
          rootView: SpeechToTextInputView(
            speechModel: speechRecognizer,
            disclaimer: Strings.VoiceSearch.screenDisclaimer
          )
        )

        if let voiceSearchController = voiceSearchViewController {
          voiceSearchController.modalTransitionStyle = .crossDissolve
          voiceSearchController.modalPresentationStyle = .overFullScreen
          present(voiceSearchController, animated: true)
        }
      } else {
        let alertController = UIAlertController(
          title: Strings.VoiceSearch.microphoneAccessRequiredWarningTitle,
          message: Strings.VoiceSearch.microphoneAccessRequiredWarningDescription,
          preferredStyle: .alert
        )

        let settingsAction = UIAlertAction(
          title: Strings.settings,
          style: .default
        ) { _ in
          completion(nil)
          let url = URL(string: UIApplication.openSettingsURLString)!
          UIApplication.shared.open(url, options: [:], completionHandler: nil)
        }

        let cancelAction = UIAlertAction(
          title: Strings.CancelString,
          style: .cancel,
          handler: { _ in
            completion(nil)
          }
        )

        alertController.addAction(settingsAction)
        alertController.addAction(cancelAction)

        present(alertController, animated: true)
      }
    }
  }
}
