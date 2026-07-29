// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import Foundation
import MarketplaceKit
import Shared
import UIKit
import Web

extension TabDataValues {
  private struct ExternalAppURLTabHelperKey: TabDataKey {
    static var defaultValue: ExternalAppURLTabHelper?
  }
  var externalAppURLHelper: ExternalAppURLTabHelper? {
    get { self[ExternalAppURLTabHelperKey.self] }
    set { self[ExternalAppURLTabHelperKey.self] = newValue }
  }
}

/// Handles requests for URLs which are meant to be opened by another application, prompting the
/// user before switching apps.
@MainActor
class ExternalAppURLTabHelper: TabPolicyDecider, @preconcurrency TabObserver {
  private weak var tab: (any TabState)?
  private weak var browserViewController: BrowserViewController?

  /// Whether or not a custom url-scheme alert is currently presented
  private var isAlertPresented = false
  private var popup: AlertPopupView?
  private var popupContinuation: CheckedContinuation<Bool, Never>?
  /// The number of alerts presented for the current domain, used to offer suppression
  private var alertCounter = 0
  /// Whether or not the user chose to suppress any further alerts for the current domain
  private var isAlertSuppressed = false
  /// The domain which the currently tracked alerts belong to
  private var alertDomain: String?
  /// The url the currently presented custom url-scheme alert is asking about
  private var externalAppURL: URL?

  init(tab: some TabState, browserViewController: BrowserViewController) {
    self.tab = tab
    self.browserViewController = browserViewController
    tab.addPolicyDecider(self)
    tab.addObserver(self)
  }

  /// Resets any tracked alert state, for example when the user navigates somewhere new themselves
  func reset() {
    alertCounter = 0
    isAlertPresented = false
    isAlertSuppressed = false
    alertDomain = nil
    externalAppURL = nil
  }

  // MARK: - TabPolicyDecider

  func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    guard let requestURL = request.url else { return .allow }
    if isAlertPresented {
      // Some external-app schemes may fire the same request back-to-back.
      // Don't tear down and re-present the alert for a request that matches
      // the one it's already asking about, otherwise it can dismiss and
      // presents repeatedly.
      if externalAppURL == requestURL {
        return .cancel
      }
      // Dismiss any alert presented for a previous request, since the page is
      // navigating elsewhere
      popup?.dismissWithType(dismissType: .noAnimation)
      popupContinuation?.resume(with: .success(false))
      popupContinuation = nil
      popup = nil
      externalAppURL = nil
      isAlertPresented = false
    }

    // First special case are some schemes that are about Calling. We prompt the user to confirm this action. This
    // gives us the exact same behaviour as Safari.
    // tel:, facetime:, facetime-audio:, already has its own native alert displayed by the OS!
    //
    // The system's prompt could handle tel/facetime via UIApplication.shared.openURL.
    // However, this can lead to a spoof if the prompt shows,
    // then a new tab is opened while the prompt is showing.
    // There is no way to dismiss the system prompt when navigation changes!
    // So handle it manually
    if ["sms", "mailto", "tel", "facetime", "facetime-audio"].contains(requestURL.scheme) {
      let shouldOpen = await handleExternalURL(
        requestURL,
        request: request,
        requestInfo: requestInfo
      )
      return shouldOpen ? .allow : .cancel
    }

    // Second special case are a set of URLs that look like regular http links, but should be handed over to iOS
    // instead of being loaded in the webview.
    // In addition we are exchaging actual scheme with "maps" scheme
    // So the Apple maps URLs will open properly
    if let mapsURL = isAppleMapsURL(requestURL), mapsURL.enabled {
      let shouldOpen = await handleExternalURL(
        mapsURL.url,
        request: request,
        requestInfo: requestInfo
      )
      return shouldOpen ? .allow : .cancel
    }

    if isStoreURL(requestURL) {
      let shouldOpen = await handleExternalURL(
        requestURL,
        request: request,
        requestInfo: requestInfo
      )
      return shouldOpen ? .allow : .cancel
    }

    // Anything else that the browser itself handles is not an external app URL
    guard requiresExternalApp(requestURL) else { return .allow }

    // Standard schemes are handled in previous if-case.
    // This check handles custom app schemes to open external apps.
    // Our own 'brave' scheme does not require the switch-app prompt.
    // Do not allow opening external URLs from child tabs
    let shouldOpen = await handleExternalURL(
      requestURL,
      request: request,
      requestInfo: requestInfo
    )

    if !shouldOpen {
      await showUnableToOpenURLErrorIfNeeded(for: requestInfo)
    }

    return shouldOpen ? .allow : .cancel
  }

  // MARK: - External URL handling

  /// Whether or not this helper decides the policy for the given URL, in which case the browser
  /// itself does not need to handle it
  func handlesURL(_ url: URL) -> Bool {
    if ["sms", "mailto", "tel", "facetime", "facetime-audio"].contains(url.scheme) {
      return true
    }
    if let mapsURL = isAppleMapsURL(url), mapsURL.enabled {
      return true
    }
    return isStoreURL(url) || requiresExternalApp(url)
  }

  /// Whether or not the given URL can only be handled by another application
  private func requiresExternalApp(_ url: URL) -> Bool {
    if InternalURL.isValid(url: url) || url.isBookmarklet {
      return false
    }

    guard let scheme = url.scheme else { return false }

    if ["about", "http", "https", "data", "blob", "file"].contains(scheme) {
      return false
    }

    // Our own schemes are loaded in the web view
    if scheme.contains("brave") || scheme.contains("chrome") {
      return false
    }

    if #available(iOS 17.4, *), !ProcessInfo.processInfo.isiOSAppOnVisionOS {
      // Accessing `MarketplaceKitURIScheme` on Vision OS results in a crash
      if scheme == MarketplaceKitURIScheme {
        return false
      }
    }

    return true
  }

  // Recognize an Apple Maps URL. This will trigger the native app. But only if a search query is present.
  // Otherwise it could just be a visit to a regular page on maps.apple.com.
  // Exchaging https/https scheme with maps in order to open URLS properly on Apple Maps
  private func isAppleMapsURL(_ url: URL) -> (enabled: Bool, url: URL)? {
    if url.scheme == "http" || url.scheme == "https" {
      if url.host == "maps.apple.com" && url.query != nil {
        guard var urlComponents = URLComponents(url: url, resolvingAgainstBaseURL: false) else {
          return nil
        }
        urlComponents.scheme = "maps"

        if let url = urlComponents.url {
          return (true, url)
        }
        return nil
      }
    }
    return (false, url)
  }

  // Recognize a iTunes Store URL. These all trigger the native apps. Note that appstore.com and phobos.apple.com
  // used to be in this list. I have removed them because they now redirect to itunes.apple.com. If we special case
  // them then iOS will actually first open Safari, which then redirects to the app store. This works but it will
  // leave a 'Back to Safari' button in the status bar, which we do not want.
  private func isStoreURL(_ url: URL) -> Bool {
    let isStoreScheme = ["itms-apps", "itms-appss", "itmss"].contains(url.scheme)
    if isStoreScheme {
      return true
    }

    let isHttpScheme = ["http", "https"].contains(url.scheme)
    let isAppStoreHost = ["itunes.apple.com", "apps.apple.com", "appsto.re"].contains(url.host)
    return isHttpScheme && isAppStoreHost
  }

  /// Prompts the user to open the given URL in an external application, returning whether or not
  /// the request should be allowed to continue.
  ///
  /// - Parameters:
  ///   - url: The URL to hand off to the external application. This may differ from the request's
  ///          URL, for example when rewriting an Apple Maps URL
  ///   - request: The request which resulted in this external URL
  ///   - requestInfo: Additional information about the request
  private func handleExternalURL(
    _ url: URL,
    request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> Bool {
    guard let tab, let browserViewController else { return false }

    // Do not open external links for child tabs automatically
    // The user must tap on the link to open it.
    if tab.opener != nil && requestInfo.navigationType != .linkActivated {
      return false
    }

    // If the request is cross origin frame, block it.
    // If the request is cross origin window, block it.
    if requestInfo.isCrossOriginFrame || requestInfo.isCrossOriginWindow {
      return false
    }

    // If the request is from a sub-frame and not user-initiated, block it.
    if !requestInfo.isMainFrame && !requestInfo.isUserInitiated {
      return false
    }

    // Check if the current url of the caller has changed
    if let domain = tab.visibleURL?.baseDomain, domain != alertDomain {
      alertCounter = 0
      isAlertSuppressed = false
    }

    alertDomain = tab.lastCommittedURL?.baseDomain

    // Do not try to present over existing warning
    if isAlertPresented || isAlertSuppressed {
      return false
    }

    // External dialog should not be shown for non-active tabs #6687 - #7835
    if !tab.isVisible {
      return false
    }

    var alertTitle = Strings.openExternalAppURLGenericTitle

    if requestInfo.isMainFrame {
      if case let origin = URLOrigin(url: url), !origin.isOpaque {
        let displayHost =
          "\(origin.scheme)://\(origin.host):\(origin.port)"
        alertTitle = String(format: Strings.openExternalAppURLTitle, displayHost)
      } else if let displayHost = tab.lastCommittedURL?.withoutWWW.host {
        alertTitle = String(format: Strings.openExternalAppURLTitle, displayHost)
      }
    }

    // Handling condition when Tab is empty when handling an external URL we should remove the tab once the user decides
    let removeTabIfEmpty = { [weak browserViewController, weak tab] in
      guard let tab else { return }
      if tab.visibleURL == nil {
        browserViewController?.tabManager.removeTab(tab)
      }
    }

    // Show the external sceheme invoke alert
    func showExternalSchemeAlert(
      isSuppressActive: Bool,
      openedURLCompletionHandler: @escaping (Bool) -> Void
    ) {
      // Check if active controller is bvc otherwise do not show show external scheme alerts
      guard shouldShowExternalSchemeAlert() else {
        openedURLCompletionHandler(false)
        return
      }

      browserViewController.view.endEditing(true)
      isAlertPresented = true
      externalAppURL = url

      let popup = AlertPopupView(
        imageView: nil,
        title: alertTitle,
        message: String(format: Strings.openExternalAppURLMessage, url.relativeString),
        titleWeight: .semibold,
        titleSize: 21
      )

      self.popup = popup

      if isSuppressActive {
        popup.addButton(title: Strings.suppressAlertsActionTitle, type: .destructive) {
          [weak self] () -> PopupViewDismissType in
          openedURLCompletionHandler(false)
          self?.isAlertSuppressed = true
          return .flyDown
        }
      } else {
        popup.addButton(title: Strings.openExternalAppURLDontAllow) {
          [weak self] () -> PopupViewDismissType in
          openedURLCompletionHandler(false)
          removeTabIfEmpty()
          self?.isAlertPresented = false
          self?.externalAppURL = nil
          return .flyDown
        }
      }
      popup.addButton(title: Strings.openExternalAppURLAllow, type: .primary) {
        [weak self] () -> PopupViewDismissType in
        UIApplication.shared.open(url, options: [:]) { didOpen in
          openedURLCompletionHandler(!didOpen)
        }
        removeTabIfEmpty()
        self?.isAlertPresented = false
        self?.externalAppURL = nil
        return .flyDown
      }
      popup.showWithType(showType: .flyUp)
    }

    func shouldShowExternalSchemeAlert() -> Bool {
      guard let rootVC = browserViewController.currentScene?.browserViewController else {
        return false
      }

      func topViewController(startingFrom viewController: UIViewController) -> UIViewController {
        var top = viewController
        if let navigationController = top as? UINavigationController,
          let vc = navigationController.visibleViewController
        {
          return topViewController(startingFrom: vc)
        }
        if let tabController = top as? UITabBarController,
          let vc = tabController.selectedViewController
        {
          return topViewController(startingFrom: vc)
        }
        while let next = top.presentedViewController {
          top = next
        }
        return top
      }

      let isTopController = browserViewController == topViewController(startingFrom: rootVC)
      let isTopWindow = browserViewController.view.window?.isKeyWindow == true
      return isTopController && isTopWindow
    }

    alertCounter += 1

    return await withTaskCancellationHandler {
      return await withCheckedContinuation { [weak self] continuation in
        guard let self else {
          continuation.resume(returning: false)
          return
        }
        popupContinuation = continuation
        showExternalSchemeAlert(isSuppressActive: alertCounter > 2) { [weak self] in
          self?.popupContinuation = nil
          continuation.resume(with: .success($0))
        }
      }
    } onCancel: {
      Task { @MainActor [weak self] in
        self?.popupContinuation?.resume(with: .success(false))
        self?.popupContinuation = nil
      }
    }
  }

  /// Presents an error to the user explaining that the URL could not be opened, if the request was
  /// the result of a user action.
  private func showUnableToOpenURLErrorIfNeeded(for requestInfo: WebRequestInfo) async {
    guard let browserViewController else { return }

    // Do not show error message for JS navigated links or redirect
    // as it's not the result of a user action.
    let isSyntheticClick = !requestInfo.isUserInitiated
    guard requestInfo.navigationType == .linkActivated, !isSyntheticClick,
      browserViewController.presentedViewController == nil,
      browserViewController.presentingViewController == nil,
      !isAlertPresented, !isAlertSuppressed
    else { return }

    await withCheckedContinuation { continuation in
      // This alert does not need to be a BrowserAlertController because we return a policy
      // without waiting for user action
      let alert = UIAlertController(
        title: Strings.unableToOpenURLErrorTitle,
        message: Strings.unableToOpenURLError,
        preferredStyle: .alert
      )
      alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
      browserViewController.present(alert, animated: true) {
        continuation.resume()
      }
    }
  }

  // MARK: - TabObserver

  func tabDidStartNavigation(_ tab: some TabState) {
    reset()
  }

  func tabDidCommitSameDocumentNavigation(_ tab: some TabState) {
    reset()
  }
}
