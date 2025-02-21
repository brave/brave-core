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
    $0.webScrollView?.layer.masksToBounds = true
  }

  private weak var certStore: CertStore?
  private var handler: ((PlaylistInfo?) -> Void)?

  init() {
    super.init(frame: .zero)

    guard let webView = tab.webContentView else {
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

      guard let webContentView = tab.webContentView,
        let browserViewController = self.currentScene?.browserViewController
      else {
        self.handler?(nil)
        return
      }

      self.certStore = browserViewController.profile.certStore
      browserViewController.tab(tab, didCreateWebView: webContentView)

      tab.addObserver(self)
      tab.replaceContentScript(
        PlaylistWebLoaderContentHelper(self),
        name: PlaylistWebLoaderContentHelper.scriptName,
        forTab: tab
      )

      tab.webContentView?.frame = superview?.bounds ?? self.bounds
      tab.loadRequest(
        URLRequest(url: url, cachePolicy: .reloadIgnoringCacheData, timeoutInterval: 60.0)
      )
    }
  }

  func stop() {
    tab.stop()
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
      _ tab: Tab,
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
  func tabDidCommitNavigation(_ tab: Tab) {
    tab.evaluateSafeJavaScript(
      functionName:
        "window.__firefox__.\(PlaylistWebLoaderContentHelper.playlistProcessDocumentLoad)()",
      args: [],
      contentWorld: PlaylistWebLoaderContentHelper.scriptSandbox,
      asFunction: false
    )
  }

  func tab(_ tab: Tab, didFailNavigationWithError error: any Error) {
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
