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
import Web
import WebKit
import os.log

class LivePlaylistWebLoaderFactory: PlaylistWebLoaderFactory {
  func makeWebLoader() -> PlaylistWebLoader {
    LivePlaylistWebLoader()
  }
}

class LivePlaylistWebLoader: UIView, PlaylistWebLoader {
  fileprivate static var pageLoadTimeout = 300.0

  private let tab: any TabState

  private weak var certStore: CertStore?
  private var handler: ((PlaylistInfo?) -> Void)?

  init() {
    let tab = TabStateFactory.create(
      with: .init(
        initialConfiguration:
          WKWebViewConfiguration().then {
            $0.processPool = WKProcessPool()
            $0.preferences = WKPreferences()
            $0.preferences.javaScriptCanOpenWindowsAutomatically = false
            $0.allowsInlineMediaPlayback = true
            $0.ignoresViewportScaleLimits = true
          },
        braveCore: nil
      )
    )
    self.tab = tab
    super.init(frame: .zero)

    tab.browserData = .init(tab: tab)
    tab.createWebView()
    tab.browserData?.setScript(
      script: .playlistMediaSource,
      enabled: true
    )
    tab.webViewProxy?.scrollView?.layer.masksToBounds = true

    self.addSubview(tab.view)
    tab.view.snp.makeConstraints {
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

      guard let browserViewController = self.currentScene?.browserViewController
      else {
        self.handler?(nil)
        return
      }

      self.certStore = browserViewController.profile.certStore
      browserViewController.tabDidCreateWebView(AnyTabState(tab))

      tab.addObserver(self)
      tab.addPolicyDecider(self)
      tab.browserData?.replaceContentScript(
        PlaylistWebLoaderContentHelper(self),
        name: PlaylistWebLoaderContentHelper.scriptName,
        forTab: AnyTabState(tab)
      )

      tab.view.frame = superview?.bounds ?? self.bounds
      tab.loadRequest(
        URLRequest(url: url, cachePolicy: .reloadIgnoringCacheData, timeoutInterval: 60.0)
      )
    }
  }

  func stop() {
    tab.removeObserver(self)
    tab.removePolicyDecider(self)
    tab.stopLoading()
    self.handler?(nil)
    tab.loadHTMLString("<html><body>PlayList</body></html>", baseURL: nil)
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
        self.webLoader?.tab.loadHTMLString(
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

    func tab(
      _ tab: some TabState,
      receivedScriptMessage message: WKScriptMessage,
      replyHandler: @escaping (Any?, String?) -> Void
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
        self.webLoader?.tab.loadHTMLString(
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
            self.webLoader?.tab.loadHTMLString(
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
          self.webLoader?.tab.loadHTMLString(
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
        self.webLoader?.tab.loadHTMLString(
          "<html><body>PlayList</body></html>",
          baseURL: nil
        )
        self.webLoader = nil
      }
    }
  }
}

extension LivePlaylistWebLoader: TabObserver {
  func tabDidCommitNavigation(_ tab: some TabState) {
    tab.evaluateJavaScript(
      functionName:
        "window.__firefox__.\(PlaylistWebLoaderContentHelper.playlistProcessDocumentLoad)()",
      args: [],
      contentWorld: PlaylistWebLoaderContentHelper.scriptSandbox,
      asFunction: false
    )
  }

  func tab(_ tab: some TabState, didFailNavigationWithError error: any Error) {
    // There is a bug on some sites or something where the page may load TWICE OR there is a bug in WebKit where the page fails to load
    // Either way, WebKit returns _WKRecoveryAttempterErrorKey with a WKReloadFrameErrorRecoveryAttempter
    // Then it automatically reloads the page. In this case, we don't want to error and cancel loading and show the user an alert
    // We want to continue waiting for the page to load and a proper response to come to us.
    // If there is a real error, then we handle it and display an alert to the user.
    let error = error as NSError
    if error.userInfo["_WKRecoveryAttempterErrorKey"] == nil {
      self.handler?(nil)
    }
  }
}

extension LivePlaylistWebLoader: TabPolicyDecider {
  func tab(
    _ tab: some TabState,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    guard let requestURL = request.url else {
      return .cancel
    }

    // Add logic that powers adblock into the playlist loader, this is mostly copied from
    // BVC+TabPolicyDecider with the only difference being we force shields on and don't use
    // persistent Domain lookups.

    if let mainDocumentURL = request.mainDocumentURL {
      if mainDocumentURL != tab.currentPageData?.mainFrameURL {
        tab.currentPageData = PageData(mainFrameURL: mainDocumentURL)
      }

      if !requestInfo.isNewWindow {
        tab.currentPageData?.addSubframeURL(
          forRequestURL: requestURL,
          isForMainFrame: requestInfo.isMainFrame
        )
        let scriptTypes =
          await tab.currentPageData?.makeUserScriptTypes(
            isDeAmpEnabled: false,
            isAdBlockEnabled: true,  // enabled for playlist
            isBlockFingerprintingEnabled: true
          ) ?? []
        tab.browserData?.setCustomUserScript(scripts: scriptTypes)
      }
    }

    if ["http", "https", "data", "blob", "file"].contains(requestURL.scheme) {
      if requestInfo.isMainFrame,
        let etldP1 = requestURL.baseDomain,
        tab.proceedAnywaysDomainList?.contains(etldP1) == false
      {
        let shouldBlock = await AdBlockGroupsManager.shared.shouldBlock(
          requestURL: requestURL,
          sourceURL: requestURL,
          resourceType: .document,
          isAdBlockEnabled: true,  // enabled for playlist
          isAdBlockModeAggressive: true
        )

        if shouldBlock, let url = requestURL.encodeEmbeddedInternalURL(for: .blocked) {
          let request = PrivilegedRequest(url: url) as URLRequest
          tab.loadRequest(request)
          return .cancel
        }
      }

      if let mainDocumentURL = request.mainDocumentURL,
        mainDocumentURL.schemelessAbsoluteString == requestURL.schemelessAbsoluteString,
        requestInfo.isMainFrame
      {
        let ruleLists = await AdBlockGroupsManager.shared.ruleLists(
          isBraveShieldsEnabled: true,
          shieldLevel: .aggressive
        )
        tab.contentBlocker?.set(ruleLists: ruleLists)
      }

      // Cookie Blocking code below
      tab.browserData?.setScript(
        script: .cookieBlocking,
        enabled: Preferences.Privacy.blockAllCookies.value
      )
    }
    return .allow
  }
}
