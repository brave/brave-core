// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UIKit

extension BrowserViewController: DownloadQueueDelegate {
  func downloadQueue(_ downloadQueue: DownloadQueue, didStartDownload download: Download) {
    // If no other download toast is shown, create a new download toast and show it.
    guard let downloadToast = self.downloadToast else {
      let downloadToast = DownloadToast(
        download: download,
        completion: { buttonPressed in
          // When this toast is dismissed, be sure to clear this so that any
          // subsequent downloads cause a new toast to be created.
          self.downloadToast = nil

          // Handle download cancellation
          if buttonPressed, !downloadQueue.isEmpty {
            downloadQueue.cancelAll()

            let downloadCancelledToast = ButtonToast(
              labelText: Strings.downloadCancelledToastLabelText,
              backgroundColor: UIColor.braveLabel,
              textAlignment: .center
            )

            self.show(toast: downloadCancelledToast)
          }
        }
      )

      show(toast: downloadToast, duration: nil)
      return
    }

    // Otherwise, just add this download to the existing download toast.
    downloadToast.addDownload(download)
  }

  func downloadQueue(
    _ downloadQueue: DownloadQueue,
    didDownloadCombinedBytes combinedBytesDownloaded: Int64,
    combinedTotalBytesExpected: Int64?
  ) {
    downloadToast?.combinedBytesDownloaded = combinedBytesDownloaded
    downloadToast?.combinedTotalBytesExpected = combinedTotalBytesExpected
  }

  func downloadQueue(
    _ downloadQueue: DownloadQueue,
    download: Download,
    didFinishDownloadingTo location: URL
  ) {
    print("didFinishDownloadingTo(): \(location)")
  }

  func downloadQueue(_ downloadQueue: DownloadQueue, didCompleteWithError error: Error?) {
    guard let downloadToast = self.downloadToast, let download = downloadToast.downloads.first
    else {
      return
    }

    DispatchQueue.main.async {
      downloadToast.dismiss(false)

      if error == nil {
        let downloadCompleteToast = ButtonToast(
          labelText: download.filename,
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
