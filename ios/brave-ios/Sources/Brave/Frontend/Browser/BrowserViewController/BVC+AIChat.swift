// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AIChat
import BraveCore
import BraveStore
import BraveUI
import Foundation
import PhotosUI
import SpeechRecognition
import SwiftUI
import UIKit
import Web

extension BrowserViewController {
  func handleAIChatWebUIPageAction(_ tab: any TabState, action: AIChatWebUIPageAction) {
    switch action {
    case .handleVoiceRecognitionRequest(let completion):
      handleVoiceRecognitionRequest(completion)
    case .handleFileUploadRequest(let mode, let completion):
      switch mode {
      case .camera:
        self.presentCamera(completion)
      case .photos:
        self.presentPhotoPicker(completion)
      }
    case .presentSettings:
      presentAIChatSettings()
    case .presentPremiumPaywall:
      presentAIChatPremiumPaywall()
    case .presentManagePremium:
      Task {
        let store = BraveStoreSDK(skusService: nil)
        let leoMonthly = await store.currentTransaction(for: BraveStoreProduct.leoMonthly)
        let leoYearly = await store.currentTransaction(for: BraveStoreProduct.leoYearly)
        let isSubbedViaAppStore = leoMonthly != nil || leoYearly != nil
        if isSubbedViaAppStore, let url = URL.apple.manageSubscriptions,
          UIApplication.shared.canOpenURL(url)
        {
          await UIApplication.shared.open(url, options: [:])
        } else {
          tabManager.addTabAndSelect(
            URLRequest(url: .brave.account),
            isPrivate: privateBrowsingManager.isPrivateBrowsing
          )
        }
      }
    case .closeTab:
      tabManager.removeTab(tab)
    case .openURL(let url):
      tabManager.addTabAndSelect(
        URLRequest(url: url),
        isPrivate: privateBrowsingManager.isPrivateBrowsing
      )
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

  private func presentCamera(_ completion: @escaping ([AiChat.UploadedFile]?) -> Void) {
    class CameraDelegate: NSObject, UIImagePickerControllerDelegate, UINavigationControllerDelegate
    {
      var continuation: CheckedContinuation<UIImage?, Never>?
      func imagePickerController(
        _ picker: UIImagePickerController,
        didFinishPickingMediaWithInfo info: [UIImagePickerController.InfoKey: Any]
      ) {
        let image = info[.originalImage] as? UIImage
        continuation?.resume(returning: image)
        picker.dismiss(animated: true)
      }
      func imagePickerControllerDidCancel(_ picker: UIImagePickerController) {
        continuation?.resume(returning: nil)
        picker.dismiss(animated: true)
      }
    }

    guard UIImagePickerController.isSourceTypeAvailable(.camera) else {
      completion(nil)
      return
    }

    let controller = UIImagePickerController()
    controller.sourceType = .camera
    controller.cameraCaptureMode = .photo
    controller.mediaTypes = ["public.image"]

    Task { @MainActor in
      let delegate = CameraDelegate()
      controller.delegate = delegate
      let image: UIImage? = await withCheckedContinuation { continuation in
        delegate.continuation = continuation
        present(controller, animated: true)
      }

      let data = await image?.imageDataForLeo
      guard let data else {
        completion(nil)
        return
      }

      let filename = "image.png"
      let filesize = UInt32(data.count)
      let dataArray = [UInt8](data).map { NSNumber(value: $0) }

      let uploadedFile = AiChat.UploadedFile(
        filename: filename,
        filesize: filesize,
        data: dataArray,
        type: .image
      )

      completion([uploadedFile])
    }
  }

  private func presentPhotoPicker(_ completion: @escaping ([AiChat.UploadedFile]?) -> Void) {
    class ImageUploadDelegate: NSObject, PHPickerViewControllerDelegate {
      var continuation: CheckedContinuation<[PHPickerResult], Never>?
      func picker(_ picker: PHPickerViewController, didFinishPicking results: [PHPickerResult]) {
        continuation?.resume(returning: results)
        picker.dismiss(animated: true)
      }
    }
    var configuration = PHPickerConfiguration()
    configuration.preferredAssetRepresentationMode = .compatible
    let controller = PHPickerViewController(configuration: configuration)
    Task { @MainActor in
      let delegate = ImageUploadDelegate()
      controller.delegate = delegate
      let results: [PHPickerResult] = await withCheckedContinuation { continuation in
        delegate.continuation = continuation
        present(controller, animated: true)
      }

      if results.isEmpty {
        completion(nil)
        return
      }

      let uploadedFiles = await withTaskGroup(of: AiChat.UploadedFile?.self) { group in
        for result in results {
          group.addTask {
            return await AiChat.UploadedFile(provider: result.itemProvider)
          }
        }

        var files: [AiChat.UploadedFile] = []
        for await file in group {
          if let file = file {
            files.append(file)
          }
        }
        return files
      }

      completion(uploadedFiles.isEmpty ? nil : uploadedFiles)
    }
  }

  private func presentAIChatSettings() {
    struct StandaloneAIChatSettingsView: View {
      @Environment(\.dismiss) private var dismiss
      var model: AIChatSettingsViewModel
      var openURL: (URL) -> Void
      var body: some View {
        NavigationStack {
          AIChatSettingsView(viewModel: model)
            .toolbar {
              ToolbarItemGroup(placement: .confirmationAction) {
                Button {
                  dismiss()
                } label: {
                  Text(Strings.done)
                }
              }
            }
        }
        .environment(
          \.openURL,
          OpenURLAction { url in
            openURL(url)
            dismiss()
            return .handled
          }
        )
      }
    }
    let model = AIChatSettingsViewModel(
      helper: AIChatSettingsHelperImpl(profile: profileController.profile),
      skusService: Skus.SkusServiceFactory.get(profile: profileController.profile)
    )
    let controller = UIHostingController(
      rootView: StandaloneAIChatSettingsView(model: model) { [weak self] url in
        guard let self else { return }
        tabManager.addTabAndSelect(
          URLRequest(url: url),
          isPrivate: privateBrowsingManager.isPrivateBrowsing
        )
      }
    )
    present(controller, animated: true)
  }

  private func presentAIChatPremiumPaywall() {
    struct StandaloneAIChatPaywallView: View {
      @Environment(\.dismiss) private var dismiss
      var openURL: (URL) -> Void

      var body: some View {
        AIChatPaywallView(
          refreshCredentials: {
            openURL(.brave.braveLeoRefreshCredentials)
            dismiss()
          },
          openDirectCheckout: {
            openURL(.brave.braveLeoCheckoutURL)
            dismiss()
          }
        )
      }
    }
    let controller = UIHostingController(
      rootView: StandaloneAIChatPaywallView(openURL: { [weak self] url in
        guard let self else { return }
        tabManager.addTabAndSelect(
          URLRequest(url: url),
          isPrivate: privateBrowsingManager.isPrivateBrowsing
        )
      })
    )
    present(controller, animated: true)
  }
}

extension AiChat.UploadedFile {
  convenience init?(provider: NSItemProvider) async {
    guard provider.hasItemConformingToTypeIdentifier(UTType.image.identifier) else {
      return nil
    }

    do {
      let data = try await withCheckedThrowingContinuation { continuation in
        _ = provider.loadDataRepresentation(for: .image) { data, error in
          if let data {
            continuation.resume(returning: data)
          } else if let error {
            continuation.resume(throwing: error)
          }
        }
      }
      let filename = provider.suggestedName ?? "image"
      let filesize = UInt32(data.count)
      let dataArray = [UInt8](data).map { NSNumber(value: $0) }

      self.init(
        filename: filename,
        filesize: filesize,
        data: dataArray,
        type: .image
      )
    } catch {
      return nil
    }
  }
}

extension UIImage {
  fileprivate var imageDataForLeo: Data? {
    get async {
      return self.pngData()
    }
  }
}
