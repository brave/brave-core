// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import MobileCoreServices
import PassKit
import Shared
import UniformTypeIdentifiers
import WebKit

struct MIMEType {
  static let bitmap = "image/bmp"
  static let css = "text/css"
  static let gif = "image/gif"
  static let javaScript = "text/javascript"
  static let jpeg = "image/jpeg"
  static let html = "text/html"
  static let octetStream = "application/octet-stream"
  static let passbook = "application/vnd.apple.pkpass"
  static let pdf = "application/pdf"
  static let plainText = "text/plain"
  static let png = "image/png"
  static let webP = "image/webp"
  static let xHTML = "application/xhtml+xml"

  private static let webViewViewableTypes: [String] = [
    MIMEType.bitmap, MIMEType.gif, MIMEType.jpeg, MIMEType.html, MIMEType.pdf, MIMEType.plainText,
    MIMEType.png, MIMEType.webP, MIMEType.xHTML,
  ]

  static func canShowInWebView(_ mimeType: String) -> Bool {
    return webViewViewableTypes.contains(mimeType.lowercased())
  }

  static func mimeTypeFromFileExtension(_ fileExtension: String) -> String {
    guard let mimeType = UTType(filenameExtension: fileExtension)?.preferredMIMEType else {
      return MIMEType.octetStream
    }
    return mimeType
  }
}

extension String {
  var isKindOfHTML: Bool {
    return [MIMEType.html, MIMEType.xHTML].contains(self)
  }
}

class DownloadHelper: NSObject {
  fileprivate let task: CWVDownloadTask

  required init(task: CWVDownloadTask) {
    self.task = task
  }

  func downloadAlert(
    from view: UIView,
    okAction: @escaping () -> Void
  ) -> UIAlertController? {
    guard let host = task.originalURL.host else {
      return nil
    }

    let expectedSize =
      task.totalBytes != CWVDownloadSizeUnknown
      ? ByteCountFormatter.string(fromByteCount: task.totalBytes, countStyle: .file)
      : nil

    let title = "\(task.suggestedFileName) - \(host)"

    let downloadAlert = UIAlertController(title: title, message: nil, preferredStyle: .actionSheet)

    var downloadActionText = Strings.download
    // The download can be of undetermined size, adding expected size only if it's available.
    if let expectedSize = expectedSize {
      downloadActionText += " (\(expectedSize))"
    }

    let okAction = UIAlertAction(title: downloadActionText, style: .default) { _ in
      okAction()
    }

    let cancelAction = UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel)

    downloadAlert.addAction(okAction)
    downloadAlert.addAction(cancelAction)

    downloadAlert.popoverPresentationController?.do {
      $0.sourceView = view
      $0.sourceRect = CGRect(x: view.bounds.midX, y: view.bounds.maxY - 16, width: 0, height: 0)
      $0.permittedArrowDirections = []
    }

    return downloadAlert
  }
}

class OpenPassBookHelper: NSObject {
  fileprivate var url: URL

  fileprivate let browserViewController: BrowserViewController

  required init?(
    mimeType: String,
    url: URL,
    browserViewController: BrowserViewController
  ) {
    guard mimeType == MIMEType.passbook,
      PKAddPassesViewController.canAddPasses()
    else { return nil }
    self.url = url
    self.browserViewController = browserViewController
    super.init()
  }

  @MainActor func open() async {
    let passData = try? await Task.detached { [url] in
      try Data(contentsOf: url)
    }.value
    guard let passData else { return }
    do {
      let pass = try PKPass(data: passData)
      let passLibrary = PKPassLibrary()
      if passLibrary.containsPass(pass) {
        await UIApplication.shared.open(pass.passURL!, options: [:])
      } else {
        if let addController = PKAddPassesViewController(pass: pass) {
          browserViewController.present(addController, animated: true, completion: nil)
        }
      }
    } catch {
      // display an error
      let alertController = UIAlertController(
        title: Strings.unableToAddPassErrorTitle,
        message: Strings.unableToAddPassErrorMessage,
        preferredStyle: .alert
      )
      alertController.addAction(
        UIAlertAction(title: Strings.unableToAddPassErrorDismiss, style: .cancel) { (action) in
          // Do nothing.
        }
      )
      browserViewController.present(alertController, animated: true, completion: nil)
      return
    }
  }
}
