// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import PassKit
import SafariServices
import Shared
import UniformTypeIdentifiers
import Web
import WebKit

extension UTType {
  static let textCalendar = UTType(mimeType: "text/calendar")!  // Not the same as `calendarEvent`
  static let mobileConfiguration = UTType(mimeType: "application/x-apple-aspen-config")!
}

extension BrowserViewController: TabDownloadDelegate {
  public func tab(_ tab: some TabState, didCreateDownload download: Download) {
    guard tab.browserData?.isTabVisible() == true else {
      download.cancel()
      return
    }

    /// Safari Controller
    let mimeTypesThatRequireSFSafariViewControllerHandling: [UTType] = [
      .textCalendar,
      .mobileConfiguration,
    ]

    let mimeTypesThatRequireSFSafariViewControllerHandlingTexts: [UTType: (String, String)] = [
      .textCalendar: (
        Strings.openTextCalendarAlertTitle, Strings.openTextCalendarAlertDescription
      ),
      .mobileConfiguration: (
        Strings.openMobileConfigurationAlertTitle, Strings.openMobileConfigurationAlertDescription
      ),
    ]

    // SFSafariViewController only supports http/https links
    if let url = download.originalURL,
      url.isWebPage(includeDataURIs: false),
      tab === tabManager.selectedTab,
      let mimeType = UTType(mimeType: download.mimeType),
      mimeTypesThatRequireSFSafariViewControllerHandling.contains(mimeType),
      let (alertTitle, alertMessage) = mimeTypesThatRequireSFSafariViewControllerHandlingTexts[
        mimeType
      ]
    {
      // Do what Chromium does: https://source.chromium.org/chromium/chromium/src/+/main:ios/chrome/browser/download/ui_bundled/safari_download_coordinator.mm;l=100;bpv=1;bpt=1?q=presentMobileConfigAlertFromURL&ss=chromium%2Fchromium%2Fsrc
      // and present an alert before showing the Safari View Controller
      let alert = UIAlertController(
        title: alertTitle,
        message: String.init(
          format: alertMessage,
          url.absoluteString
        ),
        preferredStyle: .alert
      )
      alert.addAction(
        UIAlertAction(title: Strings.OBContinueButton, style: .default) { [weak self] _ in
          self?.handleLinkWithSafariViewController(url, tab: tab)
        }
      )

      alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel))
      present(alert, animated: true)

      download.cancel()
      return
    }

    Task {
      let suggestedFilename = download.filename
      if ![MIMEType.passbook, MIMEType.passbookBundle].contains(download.mimeType) {
        let shouldDownload = await downloadAlert(
          for: download,
          tab: tab,
          suggestedFileName: suggestedFilename
        )
        if !shouldDownload {
          return
        }
      }

      let temporaryDir = NSTemporaryDirectory()
      let fileName = temporaryDir + "/" + suggestedFilename
      let url = URL(fileURLWithPath: fileName)

      // WKDownload will fail with a -3000 error code if the file already exists at the given path
      if await AsyncFileManager.default.fileExists(atPath: url.path(percentEncoded: false)) {
        try? await AsyncFileManager.default.removeItem(at: url)
      }

      // FIXME: I dont think DownloadQueue even works atm
      downloadQueue.enqueue(download)

      download.startDownloadToLocalFileAtPath(url.path)
    }
  }

  public func tab(_ tab: some TabState, didFinishDownload download: Download, error: (any Error)?) {
    guard let destinationURL = download.destinationURL, error == nil else {
      downloadQueue.download(download, didCompleteWithError: error)

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
      present(alertController, animated: true, completion: nil)

      return
    }

    if [MIMEType.passbook, MIMEType.passbookBundle].contains(download.mimeType) {
      downloadQueue.download(download, didFinishDownloadingTo: destinationURL)
      if let passbookHelper = OpenPassBookHelper(
        request: nil,
        mimeType: download.mimeType,
        passURL: download.destinationURL,
        canShowInWebView: false,
        forceDownload: false,
        browserViewController: self
      ) {
        Task {
          await passbookHelper.open()
          try await AsyncFileManager.default.removeItem(at: destinationURL)
        }
      }
      return
    }

    let filename = download.filename
    let location = destinationURL
    let temporaryLocation = FileManager.default.temporaryDirectory
      .appending(component: "\(filename)-\(location.lastPathComponent)")
    try? FileManager.default.moveItem(at: location, to: temporaryLocation)
    Task {
      do {
        let destination = try await Download.uniqueDownloadPathForFilename(filename)
        try await AsyncFileManager.default.moveItem(at: temporaryLocation, to: destination)
        downloadQueue.download(download, didFinishDownloadingTo: destination)
      } catch {
        downloadQueue.download(download, didCompleteWithError: error)
      }
    }
  }

  // MARK: -

  /// Handles a link by opening it in an SFSafariViewController and presenting it on the BVC.
  ///
  /// This is unfortunately neccessary to handle certain downloads natively such as ics/calendar invites and
  /// mobileconfiguration files.
  ///
  /// The user unfortunately has to  dismiss it manually after they have handled the file.
  /// Chrome iOS does the same
  private func handleLinkWithSafariViewController(_ url: URL, tab: some TabState) {
    let vc = SFSafariViewController(url: url, configuration: .init())
    vc.modalPresentationStyle = .formSheet
    self.present(vc, animated: true)

    // If the website opened this URL in a separate tab, remove the empty tab
    if tab.visibleURL == nil || tab.visibleURL?.absoluteString == "about:blank" {
      tabManager.removeTab(tab)
    }
  }

  @MainActor
  private func downloadAlert(
    for download: Download,
    tab: some TabState,
    suggestedFileName: String
  ) async -> Bool {
    // Only download if there is a valid host
    let host = download.originalURL?.host() ?? ""

    // Never present the download alert on a tab that isn't visible
    guard tab === tabManager.selectedTab
    else {
      return false
    }

    let filename = Download.stripUnicode(fromFilename: suggestedFileName)
    let totalBytesExpected = download.totalBytesExpected

    let expectedSize =
      totalBytesExpected != nil
      ? ByteCountFormatter.string(fromByteCount: totalBytesExpected!, countStyle: .file)
      : nil

    let title = host.isEmpty ? "\(filename)" : "\(filename) - \(host)"

    var downloadActionText = Strings.download
    if let expectedSize = expectedSize {
      downloadActionText += " (\(expectedSize))"
    }

    return await withCheckedContinuation { continuation in
      let downloadAlert = UIAlertController(
        title: title,
        message: nil,
        preferredStyle: .actionSheet
      )

      downloadAlert.addAction(
        UIAlertAction(title: downloadActionText, style: .default) { _ in
          continuation.resume(returning: true)
        }
      )

      downloadAlert.addAction(
        UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel) { _ in
          continuation.resume(returning: false)
        }
      )

      downloadAlert.popoverPresentationController?.do {
        $0.sourceView = view
        $0.sourceRect = CGRect(x: view.bounds.midX, y: view.bounds.maxY - 16, width: 0, height: 0)
        $0.permittedArrowDirections = []
      }

      present(downloadAlert, animated: true, completion: nil)
    }
  }
}
