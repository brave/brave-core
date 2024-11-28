// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import BraveCore
import BraveShields
import Data
import Foundation
import MobileCoreServices
import Playlist
import Preferences
import Shared
import Storage
import WebKit
import os.log

class LivePlaylistWebLoaderFactory: PlaylistWebLoaderFactory {
  func makeWebLoader() -> PlaylistWebLoader {
    LivePlaylistWebLoader()
  }
}

class LivePlaylistWebLoader: UIView, PlaylistWebLoader {
  fileprivate static var pageLoadTimeout = 300.0
  private var pendingRequests = [String: URLRequest]()

  private let tab = Tab(
    configuration: WKWebViewConfiguration().then {
      $0.processPool = WKProcessPool()
      $0.preferences = WKPreferences()
      $0.preferences.javaScriptCanOpenWindowsAutomatically = false
      $0.allowsInlineMediaPlayback = true
      $0.ignoresViewportScaleLimits = true
    },
    type: .private
  ).then {
    $0.createWebview()
    $0.setScript(
      script: .playlistMediaSource,
      enabled: true
    )
    $0.webView?.scrollView.layer.masksToBounds = true
  }

  private weak var certStore: CertStore?
  private var handler: ((PlaylistInfo?) -> Void)?

  init() {
    super.init(frame: .zero)

    guard let webView = tab.webView else {
      return
    }

    self.addSubview(webView)
    webView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    self.removeFromSuperview()
  }

  @MainActor
  func load(url: URL) async -> PlaylistInfo? {
    return await withCheckedContinuation { continuation in
      self.handler = { [weak self] in
        // Handler cannot be called more than once!
        self?.handler = nil
        continuation.resume(returning: $0)
      }

      guard let webView = tab.webView,
        let browserViewController = self.currentScene?.browserViewController
      else {
        self.handler?(nil)
        return
      }

      self.certStore = browserViewController.profile.certStore
      let kvos: [KVOConstants] = [
        .estimatedProgress, .loading, .canGoBack,
        .canGoForward, .url, .title,
        .hasOnlySecureContent, .serverTrust,
      ]

      browserViewController.tab(tab, didCreateWebView: webView)
      kvos.forEach { webView.removeObserver(browserViewController, forKeyPath: $0.keyPath) }

      // When creating a tab, TabManager automatically adds a uiDelegate
      // This webView is invisible and we don't want any UI being handled.
      webView.uiDelegate = nil
      webView.navigationDelegate = self

      tab.replaceContentScript(
        PlaylistWebLoaderContentHelper(self),
        name: PlaylistWebLoaderContentHelper.scriptName,
        forTab: tab
      )

      webView.frame = superview?.bounds ?? self.bounds
      webView.load(
        URLRequest(url: url, cachePolicy: .reloadIgnoringCacheData, timeoutInterval: 60.0)
      )
    }
  }

  func stop() {
    guard let webView = tab.webView else { return }
    webView.stopLoading()
    self.handler?(nil)
    webView.loadHTMLString("<html><body>PlayList</body></html>", baseURL: nil)
  }

  private class PlaylistWebLoaderContentHelper: TabContentScript {
    private weak var webLoader: LivePlaylistWebLoader?
    private var playlistItems = Set<String>()
    private var isPageLoaded = false
    private var timeout: DispatchWorkItem?

    init(_ webLoader: LivePlaylistWebLoader) {
      self.webLoader = webLoader

      timeout = DispatchWorkItem(block: { [weak self] in
        guard let self = self else { return }
        self.webLoader?.handler?(nil)
        self.webLoader?.tab.webView?.loadHTMLString(
          "<html><body>PlayList</body></html>",
          baseURL: nil
        )
        self.webLoader = nil
      })

      if let timeout = timeout {
        DispatchQueue.main.asyncAfter(
          deadline: .now() + LivePlaylistWebLoader.pageLoadTimeout,
          execute: timeout
        )
      }
    }

    static let scriptName = "PlaylistScript"
    static let scriptId = PlaylistScriptHandler.scriptId
    static let messageHandlerName = PlaylistScriptHandler.messageHandlerName
    static let scriptSandbox = PlaylistScriptHandler.scriptSandbox
    static let userScript: WKUserScript? = nil
    static let playlistProcessDocumentLoad = PlaylistScriptHandler.playlistProcessDocumentLoad

    func userContentController(
      _ userContentController: WKUserContentController,
      didReceiveScriptMessage message: WKScriptMessage,
      replyHandler: (Any?, String?) -> Void
    ) {
      if !verifyMessage(message: message) {
        assertionFailure("Missing required security token.")
        return
      }

      replyHandler(nil, nil)

      let cancelRequest = {
        self.timeout?.cancel()
        self.timeout = nil
        self.webLoader?.handler?(nil)
        self.webLoader?.tab.webView?.loadHTMLString(
          "<html><body>PlayList</body></html>",
          baseURL: nil
        )
        self.webLoader = nil
      }

      if let readyState = PlaylistScriptHandler.ReadyState.from(message: message) {
        isPageLoaded = true

        if readyState.state == "cancel" {
          cancelRequest()
          return
        }

        if isPageLoaded {
          timeout?.cancel()
          timeout = DispatchWorkItem(block: { [weak self] in
            guard let self = self else { return }
            self.webLoader?.handler?(nil)
            self.webLoader?.tab.webView?.loadHTMLString(
              "<html><body>PlayList</body></html>",
              baseURL: nil
            )
            self.webLoader = nil
          })

          if let timeout = timeout {
            DispatchQueue.main.asyncAfter(
              deadline: .now() + LivePlaylistWebLoader.pageLoadTimeout,
              execute: timeout
            )
          }
        }
        return
      }

      guard let item = PlaylistInfo.from(message: message),
        item.detected
      else {
        cancelRequest()
        return
      }

      if item.isInvisible {
        timeout?.cancel()
        timeout = DispatchWorkItem(block: { [weak self] in
          guard let self = self else { return }
          self.webLoader?.handler?(nil)
          self.webLoader?.tab.webView?.loadHTMLString(
            "<html><body>PlayList</body></html>",
            baseURL: nil
          )
          self.webLoader = nil
        })

        if let timeout = timeout {
          DispatchQueue.main.asyncAfter(
            deadline: .now() + LivePlaylistWebLoader.pageLoadTimeout,
            execute: timeout
          )
        }
        return
      }

      // For now, we ignore base64 video mime-types loaded via the `data:` scheme.
      if item.duration <= 0.0 && !item.detected || item.src.isEmpty || item.src.hasPrefix("data:")
        || item.src.hasPrefix("blob:")
      {
        cancelRequest()
        return
      }

      DispatchQueue.main.async {
        if !self.playlistItems.contains(item.src) {
          self.playlistItems.insert(item.src)

          self.timeout?.cancel()
          self.timeout = nil
          self.webLoader?.handler?(item)
          self.webLoader = nil
        }

        // This line MAY cause problems.. because some websites have a loading delay for the source of the media item
        // If the second we receive the src, we reload the page by doing the below HTML,
        // It may not have received all info necessary to play the item such as MetadataInfo
        // For now it works 100% of the time and it is safe to do it. If we come across such a website, that causes problems,
        // we'll need to find a different way of forcing the WebView to STOP loading metadata in the background
        self.webLoader?.tab.webView?.loadHTMLString(
          "<html><body>PlayList</body></html>",
          baseURL: nil
        )
        self.webLoader = nil
      }
    }
  }
}

extension LivePlaylistWebLoader: WKNavigationDelegate {
  // Recognize an Apple Maps URL. This will trigger the native app. But only if a search query is present. Otherwise
  // it could just be a visit to a regular page on maps.apple.com.
  fileprivate func isAppleMapsURL(_ url: URL) -> Bool {
    if url.scheme == "http" || url.scheme == "https" {
      if url.host == "maps.apple.com" && url.query != nil {
        return true
      }
    }
    return false
  }

  // Recognize a iTunes Store URL. These all trigger the native apps. Note that appstore.com and phobos.apple.com
  // used to be in this list. I have removed them because they now redirect to itunes.apple.com. If we special case
  // them then iOS will actually first open Safari, which then redirects to the app store. This works but it will
  // leave a 'Back to Safari' button in the status bar, which we do not want.
  fileprivate func isStoreURL(_ url: URL) -> Bool {
    let isStoreScheme = ["itms-apps", "itms-appss", "itmss"].contains(url.scheme)
    if isStoreScheme {
      return true
    }

    let isHttpScheme = ["http", "https"].contains(url.scheme)
    let isAppStoreHost = ["itunes.apple.com", "apps.apple.com", "appsto.re"].contains(url.host)
    return isHttpScheme && isAppStoreHost
  }

  func webView(_ webView: WKWebView, didCommit navigation: WKNavigation!) {
    webView.evaluateSafeJavaScript(
      functionName:
        "window.__firefox__.\(PlaylistWebLoaderContentHelper.playlistProcessDocumentLoad)()",
      args: [],
      contentWorld: PlaylistWebLoaderContentHelper.scriptSandbox,
      asFunction: false
    )
  }

  func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
    // There is a bug on some sites or something where the page may load TWICE OR there is a bug in WebKit where the page fails to load
    // Either way, WebKit returns _WKRecoveryAttempterErrorKey with a WKReloadFrameErrorRecoveryAttempter
    // Then it automatically reloads the page. In this case, we don't want to error and cancel loading and show the user an alert
    // We want to continue waiting for the page to load and a proper response to come to us.
    // If there is a real error, then we handle it and display an alert to the user.
    if let error = error as? NSError {
      if error.userInfo["_WKRecoveryAttempterErrorKey"] == nil {
        self.handler?(nil)
      }
      return
    }

    self.handler?(nil)
  }

  func webView(
    _ webView: WKWebView,
    decidePolicyFor navigationAction: WKNavigationAction,
    preferences: WKWebpagePreferences
  ) async -> (WKNavigationActionPolicy, WKWebpagePreferences) {
    guard let url = navigationAction.request.url else {
      return (.cancel, preferences)
    }

    if url.scheme == "about" || url.isBookmarklet {
      return (.cancel, preferences)
    }

    if navigationAction.isInternalUnprivileged && navigationAction.navigationType != .backForward {
      return (.cancel, preferences)
    }

    // Universal links do not work if the request originates from the app, manual handling is required.
    if let mainDocURL = navigationAction.request.mainDocumentURL,
      let universalLink = UniversalLinkManager.universalLinkType(for: mainDocURL, checkPath: true)
    {
      switch universalLink {
      case .buyVPN:
        return (.cancel, preferences)
      }
    }

    // First special case are some schemes that are about Calling. We prompt the user to confirm this action. This
    // gives us the exact same behaviour as Safari.
    if url.scheme == "tel" || url.scheme == "facetime" || url.scheme == "facetime-audio"
      || url.scheme == "mailto" || isAppleMapsURL(url) || isStoreURL(url)
    {
      return (.cancel, preferences)
    }

    // Ad-blocking checks
    if let mainDocumentURL = navigationAction.request.mainDocumentURL {
      if mainDocumentURL != tab.currentPageData?.mainFrameURL {
        // Clear the current page data if the page changes.
        // Do this before anything else so that we have a clean slate.
        tab.currentPageData = PageData(mainFrameURL: mainDocumentURL)
      }

      let domainForMainFrame = Domain.getOrCreate(forUrl: mainDocumentURL, persistent: false)

      if let requestURL = navigationAction.request.url,
        let targetFrame = navigationAction.targetFrame
      {
        tab.currentPageData?.addSubframeURL(
          forRequestURL: requestURL,
          isForMainFrame: targetFrame.isMainFrame
        )
        let scriptTypes =
          await tab.currentPageData?.makeUserScriptTypes(
            domain: domainForMainFrame,
            isDeAmpEnabled: false
          ) ?? []
        tab.setCustomUserScript(scripts: scriptTypes)
      }
    }

    if ["http", "https", "data", "blob", "file"].contains(url.scheme) {
      if navigationAction.targetFrame?.isMainFrame == true {
        tab.updateUserAgent(webView, newURL: url)
      }

      pendingRequests[url.absoluteString] = navigationAction.request

      if let mainDocumentURL = navigationAction.request.mainDocumentURL,
        mainDocumentURL.schemelessAbsoluteString == url.schemelessAbsoluteString,
        !(InternalURL(url)?.isSessionRestore ?? false),
        navigationAction.sourceFrame.isMainFrame
          || navigationAction.targetFrame?.isMainFrame == true
      {

        // Identify specific block lists that need to be applied to the requesting domain
        let domainForShields = Domain.getOrCreate(forUrl: mainDocumentURL, persistent: false)

        // Force adblocking on
        domainForShields.shield_allOff = 0
        domainForShields.domainBlockAdsAndTrackingLevel = .standard

        // Load block lists
        let ruleLists = await AdBlockGroupsManager.shared.ruleLists(for: domainForShields)
        tab.contentBlocker.set(ruleLists: ruleLists)

        let isScriptsEnabled = !domainForShields.isShieldExpected(
          .noScript,
          considerAllShieldsOption: true
        )
        preferences.allowsContentJavaScript = isScriptsEnabled
      }

      // Cookie Blocking code below
      tab.setScript(script: .cookieBlocking, enabled: Preferences.Privacy.blockAllCookies.value)

      return (.allow, preferences)
    }

    return (.cancel, preferences)
  }

  func webView(
    _ webView: WKWebView,
    decidePolicyFor navigationResponse: WKNavigationResponse
  ) async -> WKNavigationResponsePolicy {
    let response = navigationResponse.response
    let responseURL = response.url

    // We also add subframe urls in case a frame upgraded to https
    if let responseURL = responseURL,
      tab.currentPageData?.upgradeFrameURL(
        forResponseURL: responseURL,
        isForMainFrame: navigationResponse.isForMainFrame
      ) == true,
      let domain = tab.currentPageData?.domain(persistent: false)
    {
      let scriptTypes =
        await tab.currentPageData?.makeUserScriptTypes(domain: domain, isDeAmpEnabled: false) ?? []
      tab.setCustomUserScript(scripts: scriptTypes)
    }

    var request: URLRequest?
    if let url = responseURL {
      request = pendingRequests.removeValue(forKey: url.absoluteString)
    }

    // TODO: REFACTOR to support Multiple Windows Better
    if let browserController = webView.currentScene?.browserViewController {
      // Check if this response should be handed off to Passbook.
      if OpenPassBookHelper(
        request: request,
        response: response,
        canShowInWebView: false,
        forceDownload: false,
        browserViewController: browserController
      ) != nil {
        return .cancel
      }
    }

    if navigationResponse.isForMainFrame {
      if response.mimeType?.isKindOfHTML == false, request != nil {
        return .cancel
      } else {
        tab.temporaryDocument = nil
      }

      tab.mimeType = response.mimeType
    }

    return .allow
  }

  nonisolated func webView(
    _ webView: WKWebView,
    respondTo challenge: URLAuthenticationChallenge
  ) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    // If this is a certificate challenge, see if the certificate has previously been
    // accepted by the user.
    let origin = "\(challenge.protectionSpace.host):\(challenge.protectionSpace.port)"
    if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
      let trust = challenge.protectionSpace.serverTrust,
      let cert = (SecTrustCopyCertificateChain(trust) as? [SecCertificate])?.first,
      await certStore?.containsCertificate(cert, forOrigin: origin) == true
    {
      return (.useCredential, URLCredential(trust: trust))
    }

    // Certificate Pinning
    if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust {
      if let serverTrust = challenge.protectionSpace.serverTrust {
        let host = challenge.protectionSpace.host
        let port = challenge.protectionSpace.port

        let result = BraveCertificateUtility.verifyTrust(
          serverTrust,
          host: host,
          port: port
        )
        let certificateChain = SecTrustCopyCertificateChain(serverTrust) as? [SecCertificate] ?? []

        // Cert is valid and should be pinned
        if result == 0 {
          return (.useCredential, URLCredential(trust: serverTrust))
        }

        // Cert is valid and should not be pinned
        // Let the system handle it and we'll show an error if the system cannot validate it
        if result == Int32.min {
          return (.performDefaultHandling, nil)
        }

        return (.cancelAuthenticationChallenge, nil)
      }
    }

    guard
      challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPBasic
        || challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPDigest
        || challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodNTLM
    else {
      return (.performDefaultHandling, nil)
    }

    return (.cancelAuthenticationChallenge, nil)
  }
}
