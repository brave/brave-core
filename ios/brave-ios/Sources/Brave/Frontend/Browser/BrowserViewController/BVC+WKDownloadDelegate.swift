// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import OSLog
import PassKit
import Shared
import WebKit

extension BrowserViewController: CWVDownloadTaskDelegate {
  fileprivate func uniqueDownloadPathForFilename(_ filename: String) async throws -> URL {
    let downloadsPath = try await AsyncFileManager.default.downloadsPath()
    let basePath = downloadsPath.appending(path: filename)
    let fileExtension = basePath.pathExtension
    let filenameWithoutExtension =
      !fileExtension.isEmpty ? String(filename.dropLast(fileExtension.count + 1)) : filename

    var proposedPath = basePath
    var count = 0

    while await AsyncFileManager.default.fileExists(atPath: proposedPath.path) {
      count += 1

      let proposedFilenameWithoutExtension = "\(filenameWithoutExtension) (\(count))"
      proposedPath = downloadsPath.appending(path: proposedFilenameWithoutExtension)
        .appending(path: fileExtension)
    }

    return proposedPath
  }

  public func downloadTask(
    _ downloadTask: CWVDownloadTask,
    didFinishWithError error: (any Error)?
  ) {
    defer {
      downloadToast?.dismiss(false)
    }
    if let error {
      // display an error
      let alertController = UIAlertController(
        // FIXME: This error needs to generalized to any download
        title: Strings.unableToAddPassErrorTitle,
        message: Strings.unableToAddPassErrorMessage,
        preferredStyle: .alert
      )
      alertController.addAction(
        UIAlertAction(title: Strings.unableToAddPassErrorDismiss, style: .cancel) { (action) in
          // Do nothing.
        }
      )
      present(alertController, animated: true, completion: nil)
      return
    }

    let fileURL = FileManager.default.temporaryDirectory.appending(
      path: downloadTask.suggestedFileName
    )

    if let passbookHelper = OpenPassBookHelper(
      mimeType: downloadTask.mimeType,
      url: fileURL,
      browserViewController: self
    ) {
      Task {
        await passbookHelper.open()
        try await AsyncFileManager.default.removeItem(at: fileURL)
      }
    } else {
      Task {
        if error == nil {
          do {
            let destination = try await self.uniqueDownloadPathForFilename(
              downloadTask.suggestedFileName
            )
            try await AsyncFileManager.default.moveItem(at: fileURL, to: destination)
          } catch {
            Logger.module.error("Failed to move downloaded file to downloads")
          }
          let downloadCompleteToast = ButtonToast(
            labelText: downloadTask.suggestedFileName,
            image: UIImage(named: "check", in: .module, compatibleWith: nil)?.template,
            buttonText: Strings.downloadsButtonTitle,
            completion: { buttonPressed in
              guard buttonPressed else { return }

              UIApplication.shared.openBraveDownloadsFolder { [weak self] success in
                if !success {
                  self?.displayOpenDownloadsError()
                }
              }
            }
          )

          self.show(toast: downloadCompleteToast, duration: DispatchTimeInterval.seconds(8))
        } else {
          let downloadFailedToast = ButtonToast(
            labelText: Strings.downloadFailedToastLabelText,
            backgroundColor: UIColor.braveLabel,
            textAlignment: .center
          )

          self.show(toast: downloadFailedToast, duration: nil)
        }
      }
    }
  }

  public func downloadTaskProgressDidChange(_ downloadTask: CWVDownloadTask) {
    downloadToast?.updatePercent()
  }

  func displayOpenDownloadsError() {
    let alert = UIAlertController(
      title: Strings.genericErrorTitle,
      message: Strings.openDownloadsFolderErrorDescription,
      preferredStyle: .alert
    )
    alert.addAction(
      UIAlertAction(title: Strings.PlayList.okayButtonTitle, style: .default, handler: nil)
    )

    present(alert, animated: true, completion: nil)
  }
}
