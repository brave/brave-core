// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Data
import DesignSystem
import Foundation
import Preferences
import Shared
import Strings
import UIKit
import UserAgent
import Web

extension BrowserViewController: TabDelegate {
  public func tabWebViewDidClose(_ tab: some TabState) {
    if tab.browserData?.isInternalRedirect == true {
      // Avoid closing the tab if we cancelled the initial navigation to perform a redirect since
      // we are now performing a new request that may not cancel without user interaction.
      return
    }
    tabManager.addTabToRecentlyClosed(tab)
    tabManager.removeTab(tab)
  }

  public func tab(
    _ tab: some TabState,
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
        let isPrivate = currentTab.isPrivate

        if !isPrivate {
          let openNewTabAction = UIAction(
            title: Strings.openNewTabButtonTitle,
            image: UIImage(braveSystemNamed: "leo.browser.mobile-tab-new")
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
          if !isPrivate, Preferences.Privacy.privateBrowsingLock.value {
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
          if !isPrivate {
            let openNewWindowAction = UIAction(
              title: Strings.openInNewWindowTitle,
              image: UIImage(braveSystemNamed: "leo.window.tab-new")
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
            if !isPrivate, Preferences.Privacy.privateBrowsingLock.value {
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
          image: UIImage(braveSystemNamed: "leo.copy"),
          handler: UIAction.deferredActionHandler { _ in
            UIPasteboard.general.url = url as URL
          }
        )
        copyAction.accessibilityLabel = "linkContextMenu.copyLink"
        actions.append(copyAction)

        let copyCleanLinkAction = UIAction(
          title: Strings.copyCleanLink,
          image: UIImage(braveSystemNamed: "leo.copy.clean"),
          handler: UIAction.deferredActionHandler { _ in
            let service = URLSanitizerServiceFactory.get(privateMode: currentTab.isPrivate)
            let cleanedURL = service?.sanitizeURL(url) ?? url
            UIPasteboard.general.url = cleanedURL
          }
        )
        copyCleanLinkAction.accessibilityLabel = "linkContextMenu.copyCleanLink"
        actions.append(copyCleanLinkAction)

        let shareAction = UIAction(
          title: Strings.shareLinkActionTitle,
          image: UIImage(braveSystemNamed: "leo.share.macos")
        ) { _ in
          // TODO: Find a way to add fixes brave-ios#3323 and brave-ios#2961 here:
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
            sourceView: self.view,
            sourceRect: self.view.convert(
              self.topToolbar.shareButton.frame,
              from: self.topToolbar.shareButton.superview
            ),
            arrowDirection: .any
          )
        }
        shareAction.accessibilityLabel = "linkContextMenu.share"
        actions.append(shareAction)

        let linkPreview = Preferences.General.enableLinkPreview.value

        let linkPreviewTitle =
          linkPreview ? Strings.hideLinkPreviewsActionTitle : Strings.showLinkPreviewsActionTitle
        let linkPreviewAction = UIAction(
          title: linkPreviewTitle,
          image: UIImage(braveSystemNamed: linkPreview ? "leo.eye.off" : "leo.eye.on")
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

  public func tab(
    _ tab: some TabState,
    contextMenuWithLinkURL linkURL: URL?,
    willCommitWithAnimator animator: (any UIContextMenuInteractionCommitAnimating)?
  ) {
    guard let linkURL else { return }
    tab.loadRequest(URLRequest(url: linkURL))
  }

  public func tab(
    _ tab: some TabState,
    requestMediaCapturePermissionsFor type: WebMediaCaptureType
  ) async -> WebPermissionDecision {
    guard let origin = tab.lastCommittedURL?.origin, tab === tabManager.selectedTab else {
      return .deny
    }

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

  public func tab(
    _ tab: some TabState,
    runJavaScriptAlertPanelWithMessage message: String,
    pageURL: URL
  ) async {
    guard case let origin = pageURL.origin, !origin.isOpaque, tab === tabManager.selectedTab else {
      return
    }
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

  public func tab(
    _ tab: some TabState,
    runJavaScriptConfirmPanelWithMessage message: String,
    pageURL: URL
  ) async -> Bool {
    guard case let origin = pageURL.origin, !origin.isOpaque, tab === tabManager.selectedTab else {
      return false
    }
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

  public func tab(
    _ tab: some TabState,
    runJavaScriptConfirmPanelWithPrompt prompt: String,
    defaultText: String?,
    pageURL: URL
  ) async -> String? {
    guard case let origin = pageURL.origin, !origin.isOpaque, tab === tabManager.selectedTab else {
      return nil
    }
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

  public func tab(_ tab: some TabState, shouldBlockJavaScriptForRequest request: URLRequest) -> Bool
  {
    guard let documentTargetURL = request.mainDocumentURL else { return false }
    return tab.braveShieldsHelper?.isShieldExpected(
      for: documentTargetURL,
      shield: .noScript,
      considerAllShieldsOption: true
    ) ?? false
  }

  public func tab(
    _ tab: some TabState,
    shouldBlockUniversalLinksForRequest request: URLRequest
  ) -> Bool {
    func isYouTubeLoad() -> Bool {
      guard let domain = request.mainDocumentURL?.baseDomain else {
        return false
      }
      let domainsWithUniversalLinks: Set<String> = ["youtube.com", "youtu.be"]
      return domainsWithUniversalLinks.contains(domain)
    }
    if tab.isPrivate || !Preferences.General.followUniversalLinks.value
      || (Preferences.General.keepYouTubeInBrave.value && isYouTubeLoad())
    {
      return true
    }
    return false
  }

  public func tab(_ tab: some TabState, buildEditMenuWithBuilder builder: any UIMenuBuilder) {
    let forcePaste = UIAction(title: Strings.forcePaste) { [weak tab] _ in
      if let string = UIPasteboard.general.string {
        tab?.evaluateJavaScript(
          functionName: "window.__firefox__.forcePaste",
          args: [string, UserScriptManager.securityToken],
          contentWorld: .defaultClient
        ) { _, _ in }
      }
    }
    let searchWithBrave = UIAction(title: Strings.searchWithBrave) { [weak tab, weak self] _ in
      tab?.evaluateJavaScript(
        functionName: "getSelection().toString",
        contentWorld: .defaultClient
      ) {
        result,
        _ in
        guard let tab, let selectedText = result as? String else { return }
        self?.didSelectSearchWithBrave(selectedText, tab: tab)
      }
    }
    let braveMenuItems: UIMenu = .init(
      options: [.displayInline],
      children: [forcePaste, searchWithBrave]
    )
    builder.insertChild(braveMenuItems, atEndOfMenu: .root)
  }
}

extension BrowserViewController {
  private func didSelectSearchWithBrave(_ selectedText: String, tab: some TabState) {
    let engine = profile.searchEngines.defaultEngine(
      forType: tab.isPrivate ? .privateMode : .standard
    )

    guard let url = engine?.searchURLForQuery(selectedText) else {
      assertionFailure("If this returns nil, investigate why and add proper handling or commenting")
      return
    }

    tabManager.addTabAndSelect(
      URLRequest(url: url),
      afterTab: tab,
      isPrivate: tab.isPrivate
    )

    if !privateBrowsingManager.isPrivateBrowsing {
      RecentSearch.addItem(type: .text, text: selectedText, websiteUrl: url.absoluteString)
    }
  }
  fileprivate func addTab(url: URL, inPrivateMode: Bool, currentTab: some TabState) {
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
        completion: { [weak self, weak tab] buttonPressed in
          guard let self, let tab else { return }
          if buttonPressed {
            self.tabManager.selectTab(tab)
          }
        }
      )
      self.show(toast: toast)
    }
    self.toolbarVisibilityViewModel.toolbarState = .expanded
  }

  func suppressJSAlerts(tab: some TabState) {
    let script = """
      window.alert=window.confirm=window.prompt=function(n){},
      [].slice.apply(document.querySelectorAll('iframe')).forEach(function(n){if(n.contentWindow != window){n.contentWindow.alert=n.contentWindow.confirm=n.contentWindow.prompt=function(n){}}})
      """
    tab.evaluateJavaScript(
      functionName: script,
      contentWorld: .defaultClient,
      asFunction: false
    )
  }

  func handleAlert<T: JSAlertInfo>(
    tab: some TabState,
    origin: URLOrigin,
    alert: inout T,
    completionHandler: @escaping () -> Void
  ) {
    guard let tabData = tab.browserData else {
      completionHandler()
      return
    }

    if origin.isOpaque {
      completionHandler()
      return
    }

    if tabData.blockAllAlerts {
      suppressJSAlerts(tab: tab)
      tab.browserData?.cancelQueuedAlerts()
      completionHandler()
      return
    }

    tabData.alertShownCount += 1
    let suppressBlock: JSAlertInfo.SuppressHandler = { [unowned self, weak tab] suppress in
      guard let tab else { return }
      if suppress {
        func suppressDialogues(_: UIAlertAction) {
          self.suppressJSAlerts(tab: tab)
          tab.blockAllAlerts = true
          tab.browserData?.cancelQueuedAlerts()
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
    alert.suppressHandler = tabData.alertShownCount > 1 ? suppressBlock : nil
    if tabManager.selectedTab === tab {
      let controller = alert.alertController()
      controller.delegate = self
      tab.shownPromptAlert = controller

      present(controller, animated: true)
    } else {
      tab.browserData?.queueJavascriptAlertPrompt(alert)
    }
  }

  public func tab(
    _ tab: some TabState,
    didRequestHTTPAuthFor protectionSpace: URLProtectionSpace,
    proposedCredential credential: URLCredential?,
    previousFailureCount: Int
  ) async -> URLCredential? {
    let host = protectionSpace.host
    let origin = "\(host):\(protectionSpace.port)"

    // The challenge may come from a background tab, so ensure it's the one visible.
    tabManager.selectTab(tab)
    tab.isDisplayingBasicAuthPrompt = true
    defer {
      tab.isDisplayingBasicAuthPrompt = false
      updateToolbarCurrentURL(tab.visibleURL)
    }

    let isHidden = tab.view.isHidden
    defer { tab.view.isHidden = isHidden }

    // Manually trigger a `url` change notification
    if host != tab.visibleURL?.host {
      tab.view.isHidden = true

      if tabManager.selectedTab === tab {
        updateToolbarCurrentURL(
          URL(string: "\(InternalURL.baseUrl)/\(InternalURL.Path.basicAuth.rawValue)")
        )
      }
    }

    do {
      let resolvedCredential = try await Authenticator.handleAuthRequest(
        self,
        credential: credential,
        protectionSpace: protectionSpace,
        previousFailureCount: previousFailureCount
      ).credentials

      if BasicAuthCredentialsManager.validDomains.contains(host) {
        BasicAuthCredentialsManager.setCredential(
          origin: origin,
          credential: resolvedCredential
        )
      }

      return resolvedCredential
    } catch {
      return nil
    }
  }

  public func tab(
    _ tab: some TabState,
    createNewTabWithRequest request: URLRequest,
    isUserInitiated: Bool
  ) -> (any TabState)? {
    guard !request.isInternalUnprivileged,
      let navigationURL = request.url,
      isUserInitiated || profileController.defaultHostContentSettings.popupsAllowed,
      navigationURL.shouldRequestBeOpenedAsPopup()
    else {
      print("Denying popup from request: \(request); is user initiated: \(isUserInitiated)")
      return nil
    }

    if let currentTab = tabManager.selectedTab {
      screenshotHelper.takeScreenshot(currentTab)
    }

    // If the page uses `window.open()` or `[target="_blank"]`, open the page in a new tab.
    // IMPORTANT!!: WebKit will perform the `URLRequest` automatically!! Attempting to do
    // the request here manually leads to incorrect results!!
    let newTab = tabManager.addPopupForParentTab(tab)

    newTab.setVirtualURL(URL(string: "about:blank"))

    toolbarVisibilityViewModel.toolbarState = .expanded

    // When a child tab is being selected, dismiss any popups on the parent tab
    tab.shownPromptAlert?.dismiss(animated: false)
    // Select the tab immediately, the web view will be created at different times depending on
    // the TabState implementation
    tabManager.selectTab(newTab)

    return newTab
  }

  public func tab(
    _ tab: some TabState,
    userAgentForType type: UserAgentType,
    request: URLRequest
  ) -> String? {
    let isBraveAllowedInUA =
      request.mainDocumentURL.flatMap {
        tab.braveUserAgentExceptions?.canShowBrave($0)
      } ?? true
    let mobile = isBraveAllowedInUA ? UserAgent.mobile : UserAgent.mobileMasked
    let desktop = isBraveAllowedInUA ? UserAgent.desktop : UserAgent.desktopMasked
    switch type {
    case .none, .automatic:
      let screenWidth = UIScreen.main.bounds.width
      if traitCollection.horizontalSizeClass == .compact && view.bounds.width < screenWidth / 2 {
        return mobile
      }
      return traitCollection.userInterfaceIdiom == .pad
        && profileController.defaultHostContentSettings.defaultPageMode == .desktop
        ? desktop : mobile
    case .desktop: return desktop
    case .mobile: return mobile
    }
  }

  public func tab(_ tab: some TabState, defaultUserAgentTypeForURL url: URL) -> UserAgentType {
    return profileController.defaultHostContentSettings.defaultPageMode == .desktop
      ? .desktop : .mobile
  }
}
