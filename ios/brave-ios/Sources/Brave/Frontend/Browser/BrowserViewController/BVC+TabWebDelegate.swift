// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import Preferences
import Shared
import Strings
import UIKit

/// A protocol that tells an object about web UI related events happening
///
/// `WKWebView` specific things should not be accessed from these methods, if you need to access
/// the underlying web view, you should only access it via `Tab`
protocol TabWebDelegate: AnyObject {
  func tabWebViewDidClose(_ tab: Tab)
  func tab(
    _ tab: Tab,
    contextMenuConfigurationForLinkURL linkURL: URL?
  ) async -> UIContextMenuConfiguration?
  func tab(
    _ tab: Tab,
    requestMediaCapturePermissionsFor type: WebMediaCaptureType
  ) async -> WebPermissionDecision
  func tab(_ tab: Tab, runJavaScriptAlertPanelWithMessage message: String, pageURL: URL) async
  func tab(
    _ tab: Tab,
    runJavaScriptConfirmPanelWithMessage message: String,
    pageURL: URL
  ) async -> Bool
  func tab(
    _ tab: Tab,
    runJavaScriptConfirmPanelWithPrompt prompt: String,
    defaultText: String?,
    pageURL: URL
  ) async -> String?
  func tab(
    _ tab: Tab,
    didRequestHTTPAuthFor protectionSpace: URLProtectionSpace,
    proposedCredential credential: URLCredential?,
    previousFailureCount: Int
  ) async -> URLCredential?
}

/// Media device capture types that a web page may request
enum WebMediaCaptureType {
  case camera
  case microphone
  case cameraAndMicrophone
}

/// Permission decisions for responding to various permission prompts
enum WebPermissionDecision {
  case prompt
  case grant
  case deny
}

extension TabWebDelegate {
  func tabWebViewDidClose(_ tab: Tab) {}

  func tab(
    _ tab: Tab,
    contextMenuConfigurationForLinkURL linkURL: URL?
  ) async -> UIContextMenuConfiguration? {
    return nil
  }

  func tab(
    _ tab: Tab,
    requestMediaCapturePermissionsFor type: WebMediaCaptureType
  ) async -> WebPermissionDecision {
    return .prompt
  }

  func tab(_ tab: Tab, runJavaScriptAlertPanelWithMessage message: String, pageURL: URL) async {}

  func tab(
    _ tab: Tab,
    runJavaScriptConfirmPanelWithMessage message: String,
    pageURL: URL
  ) async -> Bool {
    return false
  }

  func tab(
    _ tab: Tab,
    runJavaScriptConfirmPanelWithPrompt prompt: String,
    defaultText: String?,
    pageURL: URL
  ) async -> String? {
    return nil
  }

  func tab(
    _ tab: Tab,
    didRequestHTTPAuthFor protectionSpace: URLProtectionSpace,
    proposedCredential credential: URLCredential?,
    previousFailureCount: Int
  ) async -> URLCredential? {
    return nil
  }
}

extension BrowserViewController: TabWebDelegate {
  func tabWebViewDidClose(_ tab: Tab) {
    tabManager.addTabToRecentlyClosed(tab)
    tabManager.removeTab(tab)
  }

  func tab(
    _ tab: Tab,
    contextMenuConfigurationForLinkURL linkURL: URL?
  ) async -> UIContextMenuConfiguration? {
    // Only show context menu for valid links such as `http`, `https`, `data`. Safari does not show it for anything else.
    // This is because you cannot open `javascript:something` URLs in a new page, or share it, or anything else.
    guard let url = linkURL, url.isWebPage() else {
      return UIContextMenuConfiguration(identifier: nil, previewProvider: nil, actionProvider: nil)
    }

    let actionProvider: UIContextMenuActionProvider = { _ -> UIMenu? in
      var actions = [UIAction]()

      if let currentTab = self.tabManager.selectedTab {
        let tabType = currentTab.type

        if !tabType.isPrivate {
          let openNewTabAction = UIAction(
            title: Strings.openNewTabButtonTitle,
            image: UIImage(systemName: "plus")
          ) { _ in
            self.addTab(url: url, inPrivateMode: false, currentTab: currentTab)
          }

          openNewTabAction.accessibilityLabel = "linkContextMenu.openInNewTab"
          actions.append(openNewTabAction)
        }

        let openNewPrivateTabAction = UIAction(
          title: Strings.openNewPrivateTabButtonTitle,
          image: UIImage(named: "private_glasses", in: .module, compatibleWith: nil)!.template
        ) { _ in
          if !tabType.isPrivate, Preferences.Privacy.privateBrowsingLock.value {
            self.askForLocalAuthentication { [weak self] success, error in
              if success {
                self?.addTab(url: url, inPrivateMode: true, currentTab: currentTab)
              }
            }
          } else {
            self.addTab(url: url, inPrivateMode: true, currentTab: currentTab)
          }
        }
        openNewPrivateTabAction.accessibilityLabel = "linkContextMenu.openInNewPrivateTab"

        actions.append(openNewPrivateTabAction)

        if UIApplication.shared.supportsMultipleScenes {
          if !tabType.isPrivate {
            let openNewWindowAction = UIAction(
              title: Strings.openInNewWindowTitle,
              image: UIImage(braveSystemNamed: "leo.window")
            ) { _ in
              self.openInNewWindow(url: url, isPrivate: false)
            }

            openNewWindowAction.accessibilityLabel = "linkContextMenu.openInNewWindow"
            actions.append(openNewWindowAction)
          }

          let openNewPrivateWindowAction = UIAction(
            title: Strings.openInNewPrivateWindowTitle,
            image: UIImage(braveSystemNamed: "leo.window.tab-private")
          ) { _ in
            if !tabType.isPrivate, Preferences.Privacy.privateBrowsingLock.value {
              self.askForLocalAuthentication { [weak self] success, error in
                if success {
                  self?.openInNewWindow(url: url, isPrivate: true)
                }
              }
            } else {
              self.openInNewWindow(url: url, isPrivate: true)
            }
          }

          openNewPrivateWindowAction.accessibilityLabel = "linkContextMenu.openInNewPrivateWindow"
          actions.append(openNewPrivateWindowAction)
        }

        let copyAction = UIAction(
          title: Strings.copyLinkActionTitle,
          image: UIImage(systemName: "doc.on.doc"),
          handler: UIAction.deferredActionHandler { _ in
            UIPasteboard.general.url = url as URL
          }
        )
        copyAction.accessibilityLabel = "linkContextMenu.copyLink"
        actions.append(copyAction)

        let copyCleanLinkAction = UIAction(
          title: Strings.copyCleanLink,
          image: UIImage(braveSystemNamed: "leo.broom"),
          handler: UIAction.deferredActionHandler { _ in
            let service = URLSanitizerServiceFactory.get(privateMode: currentTab.isPrivate)
            let cleanedURL = service?.sanitizeURL(url) ?? url
            UIPasteboard.general.url = cleanedURL
          }
        )
        copyCleanLinkAction.accessibilityLabel = "linkContextMenu.copyCleanLink"
        actions.append(copyCleanLinkAction)

        if let braveWebView = tab.webView {
          let shareAction = UIAction(
            title: Strings.shareLinkActionTitle,
            image: UIImage(systemName: "square.and.arrow.up")
          ) { _ in
            let touchPoint = braveWebView.lastHitPoint
            let touchRect = CGRect(origin: touchPoint, size: .zero)

            // TODO: Find a way to add fixes #3323 and #2961 here:
            // Normally we use `tab.temporaryDocument` for the downloaded file on the tab.
            // `temporaryDocument` returns the downloaded file to disk on the current tab.
            // Using a downloaded file url results in having functions like "Save to files" available.
            // It also attaches the file (image, pdf, etc) and not the url to emails, slack, etc.
            // Since this is **not** a tab but a standalone web view, the downloaded temporary file is **not** available.
            // This results in the fixes for #3323 and #2961 not being included in this share scenario.
            // This is not a regression, we simply never handled this scenario in both fixes.
            // Some possibile fixes include:
            // - Detect the file type and download it if necessary and don't rely on the `tab.temporaryDocument`.
            // - Add custom "Save to file" functionality (needs investigation).
            self.presentActivityViewController(
              url,
              sourceView: braveWebView,
              sourceRect: touchRect,
              arrowDirection: .any
            )
          }

          shareAction.accessibilityLabel = "linkContextMenu.share"

          actions.append(shareAction)
        }

        let linkPreview = Preferences.General.enableLinkPreview.value

        let linkPreviewTitle =
          linkPreview ? Strings.hideLinkPreviewsActionTitle : Strings.showLinkPreviewsActionTitle
        let linkPreviewAction = UIAction(
          title: linkPreviewTitle,
          image: UIImage(systemName: "eye.fill")
        ) { _ in
          Preferences.General.enableLinkPreview.value.toggle()
        }

        actions.append(linkPreviewAction)
      }

      return UIMenu(title: url.absoluteString.truncate(length: 100), children: actions)
    }

    let linkPreview: UIContextMenuContentPreviewProvider? = { [unowned self, weak tab] in
      guard let tab else { return nil }
      return LinkPreviewViewController(url: url, for: tab, browserController: self)
    }

    let linkPreviewProvider = Preferences.General.enableLinkPreview.value ? linkPreview : nil
    return UIContextMenuConfiguration(
      identifier: nil,
      previewProvider: linkPreviewProvider,
      actionProvider: actionProvider
    )
  }

  func tab(
    _ tab: Tab,
    requestMediaCapturePermissionsFor type: WebMediaCaptureType
  ) async -> WebPermissionDecision {
    guard let origin = tab.committedURL?.origin else { return .deny }

    let presentAlert: (CheckedContinuation<WebPermissionDecision, Never>) -> Void = {
      [weak self] contination in
      guard let self = self else { return }

      let titleFormat: String = {
        switch type {
        case .camera:
          return Strings.requestCameraPermissionPrompt
        case .microphone:
          return Strings.requestMicrophonePermissionPrompt
        case .cameraAndMicrophone:
          return Strings.requestCameraAndMicrophonePermissionPrompt
        @unknown default:
          return Strings.requestCaptureDevicePermissionPrompt
        }
      }()
      let title = String.localizedStringWithFormat(titleFormat, origin.host)
      let alertController = BrowserAlertController(
        title: title,
        message: nil,
        preferredStyle: .alert
      )
      tab.shownPromptAlert = alertController

      alertController.addAction(
        .init(
          title: Strings.requestCaptureDevicePermissionAllowButtonTitle,
          style: .default,
          handler: { _ in
            contination.resume(returning: .grant)
          }
        )
      )
      alertController.addAction(
        .init(
          title: Strings.CancelString,
          style: .cancel,
          handler: { _ in
            contination.resume(returning: .deny)
          }
        )
      )
      alertController.dismissedWithoutAction = {
        contination.resume(returning: .prompt)
      }

      self.present(alertController, animated: true)
    }

    return await withCheckedContinuation { continuation in
      if let presentedViewController = presentedViewController as? BrowserAlertController {
        presentedViewController.dismiss(animated: true) {
          presentAlert(continuation)
        }
      } else {
        presentAlert(continuation)
      }
    }
  }

  func tab(_ tab: Tab, runJavaScriptAlertPanelWithMessage message: String, pageURL: URL) async {
    guard case let origin = pageURL.origin, !origin.isOpaque else { return }
    await withCheckedContinuation { continuation in
      let completionHandler: () -> Void = {
        continuation.resume()
      }
      var messageAlert = MessageAlert(
        message: message,
        origin: origin,
        completionHandler: completionHandler,
        suppressHandler: nil
      )
      handleAlert(tab: tab, origin: origin, alert: &messageAlert) {
        completionHandler()
      }
    }
  }

  func tab(
    _ tab: Tab,
    runJavaScriptConfirmPanelWithMessage message: String,
    pageURL: URL
  ) async -> Bool {
    guard case let origin = pageURL.origin, !origin.isOpaque else { return false }
    return await withCheckedContinuation { continuation in
      let completionHandler: (Bool) -> Void = { result in
        continuation.resume(returning: result)
      }
      var confirmAlert = ConfirmPanelAlert(
        message: message,
        origin: origin,
        completionHandler: completionHandler,
        suppressHandler: nil
      )
      handleAlert(tab: tab, origin: origin, alert: &confirmAlert) {
        completionHandler(false)
      }
    }
  }

  func tab(
    _ tab: Tab,
    runJavaScriptConfirmPanelWithPrompt prompt: String,
    defaultText: String?,
    pageURL: URL
  ) async -> String? {
    guard case let origin = pageURL.origin, !origin.isOpaque else { return nil }
    return await withCheckedContinuation { continuation in
      let completionHandler: (String?) -> Void = { result in
        continuation.resume(returning: result)
      }
      var textInputAlert = TextInputAlert(
        message: prompt,
        origin: origin,
        completionHandler: completionHandler,
        defaultText: defaultText,
        suppressHandler: nil
      )
      handleAlert(tab: tab, origin: origin, alert: &textInputAlert) {
        completionHandler(nil)
      }
    }
  }
}

extension BrowserViewController {
  fileprivate func addTab(url: URL, inPrivateMode: Bool, currentTab: Tab) {
    let tab = self.tabManager.addTab(
      URLRequest(url: url),
      afterTab: currentTab,
      isPrivate: inPrivateMode
    )
    if inPrivateMode && !privateBrowsingManager.isPrivateBrowsing {
      self.tabManager.selectTab(tab)
    } else {
      // We're not showing the top tabs; show a toast to quick switch to the fresh new tab.
      let toast = ButtonToast(
        labelText: Strings.contextMenuButtonToastNewTabOpenedLabelText,
        buttonText: Strings.contextMenuButtonToastNewTabOpenedButtonText,
        completion: { buttonPressed in
          if buttonPressed {
            self.tabManager.selectTab(tab)
          }
        }
      )
      self.show(toast: toast)
    }
    self.toolbarVisibilityViewModel.toolbarState = .expanded
  }

  func suppressJSAlerts(tab: Tab) {
    let script = """
      window.alert=window.confirm=window.prompt=function(n){},
      [].slice.apply(document.querySelectorAll('iframe')).forEach(function(n){if(n.contentWindow != window){n.contentWindow.alert=n.contentWindow.confirm=n.contentWindow.prompt=function(n){}}})
      """
    tab.webView?.evaluateSafeJavaScript(
      functionName: script,
      contentWorld: .defaultClient,
      asFunction: false
    )
  }

  func handleAlert<T: JSAlertInfo>(
    tab: Tab,
    origin: URLOrigin,
    alert: inout T,
    completionHandler: @escaping () -> Void
  ) {
    if origin.isOpaque {
      completionHandler()
      return
    }

    if tab.blockAllAlerts {
      suppressJSAlerts(tab: tab)
      tab.cancelQueuedAlerts()
      completionHandler()
      return
    }

    tab.alertShownCount += 1
    let suppressBlock: JSAlertInfo.SuppressHandler = { [unowned self, weak tab] suppress in
      guard let tab else { return }
      if suppress {
        func suppressDialogues(_: UIAlertAction) {
          self.suppressJSAlerts(tab: tab)
          tab.blockAllAlerts = true
          tab.cancelQueuedAlerts()
          completionHandler()
        }
        // Show confirm alert here.
        let suppressSheet = UIAlertController(
          title: nil,
          message: Strings.suppressAlertsActionMessage,
          preferredStyle: .actionSheet
        )
        suppressSheet.addAction(
          UIAlertAction(
            title: Strings.suppressAlertsActionTitle,
            style: .destructive,
            handler: suppressDialogues
          )
        )
        suppressSheet.addAction(
          UIAlertAction(
            title: Strings.cancelButtonTitle,
            style: .cancel,
            handler: { _ in
              completionHandler()
            }
          )
        )
        if UIDevice.current.userInterfaceIdiom == .pad,
          let popoverController = suppressSheet.popoverPresentationController
        {
          popoverController.sourceView = self.view
          popoverController.sourceRect = CGRect(
            x: self.view.bounds.midX,
            y: self.view.bounds.midY,
            width: 0,
            height: 0
          )
          popoverController.permittedArrowDirections = []
        }

        tab.shownPromptAlert = suppressSheet
        self.present(suppressSheet, animated: true)
      } else {
        completionHandler()
      }
    }
    alert.suppressHandler = tab.alertShownCount > 1 ? suppressBlock : nil
    if tabManager.selectedTab === tab {
      let controller = alert.alertController()
      controller.delegate = self
      tab.shownPromptAlert = controller

      present(controller, animated: true)
    } else {
      tab.queueJavascriptAlertPrompt(alert)
    }
  }

  func tab(
    _ tab: Tab,
    didRequestHTTPAuthFor protectionSpace: URLProtectionSpace,
    proposedCredential credential: URLCredential?,
    previousFailureCount: Int
  ) async -> URLCredential? {
    let host = protectionSpace.host
    let origin = "\(host):\(protectionSpace.port)"

    // The challenge may come from a background tab, so ensure it's the one visible.
    tabManager.selectTab(tab)
    tab.isDisplayingBasicAuthPrompt = true
    defer { tab.isDisplayingBasicAuthPrompt = false }

    if let webView = tab.webView {
      let isHidden = webView.isHidden
      defer { webView.isHidden = isHidden }

      // Manually trigger a `url` change notification
      if host != tab.url?.host {
        webView.isHidden = true

        if tabManager.selectedTab === tab {
          updateToolbarCurrentURL(
            URL(string: "\(InternalURL.baseUrl)/\(InternalURL.Path.basicAuth.rawValue)")
          )
        }
      }
    }

    do {
      let credentials = try await Authenticator.handleAuthRequest(
        self,
        credential: credential,
        protectionSpace: protectionSpace,
        previousFailureCount: previousFailureCount
      ).credentials

      if BasicAuthCredentialsManager.validDomains.contains(host) {
        BasicAuthCredentialsManager.setCredential(
          origin: origin,
          credential: credentials
        )
      }

      return credential
    } catch {
      return nil
    }
  }
}
