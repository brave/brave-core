// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AudioToolbox
import BraveCore
import BraveStrings
import BraveUI
import Foundation
import SpeechRecognition

extension BrowserViewController: AIChatCommunicationProtocol {

  public func handleVoiceRecognition(
    _ controller: AIChatCommunicationController,
    withConversationId conversationId: String,
    completion: @escaping (String?) -> Void
  ) {
    Task { @MainActor in
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

      if await speechRecognizer.askForUserPermission() {
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

  public func fetchImage(forChatUpload controller: AIChatCommunicationController) async -> URL? {
    class ImagePickerDelegate: NSObject, UIImagePickerControllerDelegate,
      UINavigationControllerDelegate
    {
      var continuation: CheckedContinuation<URL?, Never>?

      func imagePickerController(
        _ picker: UIImagePickerController,
        didFinishPickingMediaWithInfo info: [UIImagePickerController.InfoKey: Any]
      ) {
        picker.dismiss(animated: true, completion: nil)

        if let imageURL = info[UIImagePickerController.InfoKey.imageURL] as? URL {
          continuation?.resume(returning: imageURL)
          continuation = nil
        } else {
          continuation?.resume(returning: nil)
          continuation = nil
        }
      }

      func imagePickerControllerDidCancel(_ picker: UIImagePickerController) {
        picker.dismiss(animated: true, completion: nil)
        continuation?.resume(returning: nil)
        continuation = nil
      }
    }

    let delegate = ImagePickerDelegate()
    let imagePicker = UIImagePickerController()
    imagePicker.delegate = delegate
    imagePicker.sourceType = .photoLibrary

    return await withCheckedContinuation { continuation in
      delegate.continuation = continuation
      present(imagePicker, animated: true, completion: nil)
    }
  }

  public func openSettings(_ controller: AIChatCommunicationController) {
    print("[AIChat] - Open Settings")
  }

  public func openConversationFullPage(
    _ controller: AIChatCommunicationController,
    conversationId: String
  ) {
    print("[AIChat] - Open Conversation Full Screen")
  }

  public func openURL(_ controller: AIChatCommunicationController, url: URL) {
    openURLInNewTab(url, isPrivileged: false)
  }

  public func goPremium(_ controller: AIChatCommunicationController) {
    print("[AIChat] - Go Premium")
  }

  public func managePremium(_ controller: AIChatCommunicationController) {
    print("[AIChat] - Manage Premium")
  }
}
