// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import DataImporter
import Foundation
import MobileCoreServices
import PassKit
import Shared
import UniformTypeIdentifiers
import WebKit
import os.log

struct MIMEType {
  static let bitmap = "image/bmp"
  static let css = "text/css"
  static let gif = "image/gif"
  static let javaScript = "text/javascript"
  static let jpeg = "image/jpeg"
  static let html = "text/html"
  static let octetStream = "application/octet-stream"
  static let passbook = "application/vnd.apple.pkpass"
  static let passbookBundle = "application/vnd.apple.pkpasses"
  static let pdf = "application/pdf"
  static let plainText = "text/plain"
  static let png = "image/png"
  static let vCard = "text/vcard"
  static let webP = "image/webp"
  static let xHTML = "application/xhtml+xml"

  static func isVCard(_ mimeType: String) -> Bool {
    return mimeType == MIMEType.vCard
  }
}

extension String {
  var isKindOfHTML: Bool {
    return [MIMEType.html, MIMEType.xHTML].contains(self)
  }
}

class OpenPassBookHelper: NSObject {
  private let mimeType: String
  fileprivate var passURL: URL

  fileprivate let browserViewController: BrowserViewController

  required init?(
    request: URLRequest?,
    mimeType: String?,
    passURL: URL?,
    canShowInWebView: Bool,
    forceDownload: Bool,
    browserViewController: BrowserViewController
  ) {
    guard let mimeType,
      [MIMEType.passbook, MIMEType.passbookBundle].contains(mimeType.lowercased()),
      PKAddPassesViewController.canAddPasses(),
      let passURL, !forceDownload
    else { return nil }
    self.mimeType = mimeType
    self.passURL = passURL
    self.browserViewController = browserViewController
    super.init()
  }

  @MainActor func open() async {
    do {
      let passes = try await parsePasses()
      if let addController = PKAddPassesViewController(passes: passes) {
        browserViewController.present(addController, animated: true, completion: nil)
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

  private func parsePasses() async throws -> [PKPass] {
    let passData = try Data(contentsOf: passURL)
    if mimeType == MIMEType.passbookBundle {
      let files = try await Unzip.unzip(data: passData)
      do {
        let result = try files.map { try PKPass(data: $0) }
        return result
      } catch {
        throw error
      }
    }
    return try [PKPass(data: passData)]
  }

  func enumerateFiles(
    in directory: URL,
    withExtensions extensions: [String] = []
  ) async -> [URL] {
    let enumerator = AsyncFileManager.default.enumerator(
      at: directory,
      includingPropertiesForKeys: [.isRegularFileKey]
    )

    var result: [URL] = []
    for await fileURL in enumerator {
      do {
        let resourceValues = try fileURL.resourceValues(forKeys: [.isRegularFileKey])
        if resourceValues.isRegularFile == true, extensions.contains(fileURL.pathExtension) {
          result.append(fileURL)
        }
      } catch {
        Logger.module.error("Error reading file \(fileURL): \(error)")
      }
    }

    return result
  }
}
