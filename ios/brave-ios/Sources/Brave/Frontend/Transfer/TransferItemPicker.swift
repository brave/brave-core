// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import BraveTransferUI
import Foundation
import PhotosUI
import Strings
import UIKit
import UniformTypeIdentifiers

/// The native picker behind the page's `PickAndSend` on iOS (design plan
/// §5.5): Photos (`PHPickerViewController`, which needs no library access)
/// or Files. Picked items are copied into a temporary folder the caller
/// owns.
@MainActor
final class TransferItemPicker: NSObject {
  private var completion: (([SharedItem]?) -> Void)?
  private let directory: URL
  private var retainedSelf: TransferItemPicker?

  private init(directory: URL, completion: @escaping ([SharedItem]?) -> Void) {
    self.directory = directory
    self.completion = completion
  }

  /// `completion` gets nil when the user cancels.
  static func present(
    from presenter: UIViewController,
    into directory: URL,
    completion: @escaping ([SharedItem]?) -> Void
  ) {
    let picker = TransferItemPicker(directory: directory, completion: completion)
    picker.retainedSelf = picker
    picker.chooseSource(from: presenter)
  }

  private func chooseSource(from presenter: UIViewController) {
    let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    alert.addAction(
      UIAlertAction(title: Strings.BraveTransfer.pickPhotos, style: .default) { [self] _ in
        presentPhotos(from: presenter)
      }
    )
    alert.addAction(
      UIAlertAction(title: Strings.BraveTransfer.pickFiles, style: .default) { [self] _ in
        presentFiles(from: presenter)
      }
    )
    alert.addAction(
      UIAlertAction(title: Strings.BraveTransfer.shareCancel, style: .cancel) { [self] _ in
        finish(nil)
      }
    )
    if let popover = alert.popoverPresentationController {
      popover.sourceView = presenter.view
      popover.sourceRect = CGRect(
        x: presenter.view.bounds.midX,
        y: presenter.view.bounds.midY,
        width: 0,
        height: 0
      )
      popover.permittedArrowDirections = []
    }
    presenter.present(alert, animated: true)
  }

  private func presentPhotos(from presenter: UIViewController) {
    var configuration = PHPickerConfiguration()
    configuration.filter = .any(of: [.images, .videos])
    configuration.selectionLimit = TransferLimits.maxItems
    configuration.preferredAssetRepresentationMode = .current
    let picker = PHPickerViewController(configuration: configuration)
    picker.delegate = self
    presenter.present(picker, animated: true)
  }

  private func presentFiles(from presenter: UIViewController) {
    let picker = UIDocumentPickerViewController(forOpeningContentTypes: [.item], asCopy: true)
    picker.allowsMultipleSelection = true
    picker.delegate = self
    presenter.present(picker, animated: true)
  }

  private func finish(_ items: [SharedItem]?) {
    completion?(items)
    completion = nil
    retainedSelf = nil
  }
}

extension TransferItemPicker: PHPickerViewControllerDelegate {
  nonisolated func picker(
    _ picker: PHPickerViewController,
    didFinishPicking results: [PHPickerResult]
  ) {
    MainActor.assumeIsolated {
      picker.dismiss(animated: true)
      guard !results.isEmpty else {
        finish(nil)
        return
      }
      let providers = results.map(\.itemProvider)
      let directory = self.directory
      Task {
        let items = await SharedItemLoader.load(providers, into: directory)
        finish(items)
      }
    }
  }
}

extension TransferItemPicker: UIDocumentPickerDelegate {
  nonisolated func documentPicker(
    _ controller: UIDocumentPickerViewController,
    didPickDocumentsAt urls: [URL]
  ) {
    MainActor.assumeIsolated {
      let directory = self.directory
      DispatchQueue.global(qos: .userInitiated).async {
        let items = urls.prefix(TransferLimits.maxItems).compactMap {
          SharedItemLoader.fileItem(copying: $0, into: directory)
        }
        // asCopy: the picker's copies are ours to delete.
        for url in urls {
          try? FileManager.default.removeItem(at: url)
        }
        DispatchQueue.main.async {
          MainActor.assumeIsolated { self.finish(items) }
        }
      }
    }
  }

  nonisolated func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
    MainActor.assumeIsolated {
      finish(nil)
    }
  }
}
