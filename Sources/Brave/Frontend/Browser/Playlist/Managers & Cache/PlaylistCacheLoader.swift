// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import AVFoundation
import WebKit
import MobileCoreServices
import Data
import Shared
import BraveShields
import Preferences
import BraveCore
import Storage
import os.log

// IANA List of Audio types: https://www.iana.org/assignments/media-types/media-types.xhtml#audio
// IANA List of Video types: https://www.iana.org/assignments/media-types/media-types.xhtml#video
// APPLE List of UTI types: https://developer.apple.com/library/archive/documentation/Miscellaneous/Reference/UTIRef/Articles/System-DeclaredUniformTypeIdentifiers.html

public class PlaylistMimeTypeDetector {
  private(set) var mimeType: String?
  private(set) var fileExtension: String?  // When nil, assume `mpg` format.

  init(url: URL) {
    let possibleFileExtension = url.pathExtension.lowercased()
    if let supportedExtension = knownFileExtensions.first(where: { $0.lowercased() == possibleFileExtension }) {
      self.fileExtension = supportedExtension
      self.mimeType = mimeTypeMap.first(where: { $0.value == supportedExtension })?.key
    } else if let fileExtension = PlaylistMimeTypeDetector.supportedAVAssetFileExtensions().first(where: { $0.lowercased() == possibleFileExtension }) {
      self.fileExtension = fileExtension
      self.mimeType = UTType(filenameExtension: fileExtension)?.preferredMIMEType
    }
  }

  init(mimeType: String) {
    if let fileExtension = mimeTypeMap[mimeType.lowercased()] {
      self.mimeType = mimeType
      self.fileExtension = fileExtension
    } else if let mimeType = PlaylistMimeTypeDetector.supportedAVAssetMimeTypes().first(where: { $0.lowercased() == mimeType.lowercased() }) {
      self.mimeType = mimeType
      self.fileExtension = UTType(mimeType: mimeType)?.preferredFilenameExtension
    }
  }

  init(data: Data) {
    // Assume mpg by default. If it can't play, it will fail anyway..
    // AVPlayer REQUIRES that you give a file extension no matter what and will refuse to determine the extension for you without an
    // AVResourceLoaderDelegate :S

    if findHeader(offset: 0, data: data, header: [0x1A, 0x45, 0xDF, 0xA3]) {
      mimeType = "video/webm"
      fileExtension = "webm"
      return
    }

    if findHeader(offset: 0, data: data, header: [0x1A, 0x45, 0xDF, 0xA3]) {
      mimeType = "video/matroska"
      fileExtension = "mkv"
      return
    }

    if findHeader(offset: 0, data: data, header: [0x4F, 0x67, 0x67, 0x53]) {
      mimeType = "application/ogg"
      fileExtension = "ogg"
      return
    }

    if findHeader(offset: 0, data: data, header: [0x52, 0x49, 0x46, 0x46]) && findHeader(offset: 8, data: data, header: [0x57, 0x41, 0x56, 0x45]) {
      mimeType = "audio/x-wav"
      fileExtension = "wav"
      return
    }

    if findHeader(offset: 0, data: data, header: [0xFF, 0xFB]) || findHeader(offset: 0, data: data, header: [0x49, 0x44, 0x33]) {
      mimeType = "audio/mpeg"
      fileExtension = "mp4"
      return
    }

    if findHeader(offset: 0, data: data, header: [0x66, 0x4C, 0x61, 0x43]) {
      mimeType = "audio/flac"
      fileExtension = "flac"
      return
    }

    if findHeader(offset: 4, data: data, header: [0x66, 0x74, 0x79, 0x70, 0x4D, 0x53, 0x4E, 0x56]) || findHeader(offset: 4, data: data, header: [0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6F, 0x6D]) || findHeader(offset: 4, data: data, header: [0x66, 0x74, 0x79, 0x70, 0x6D, 0x70, 0x34, 0x32]) || findHeader(offset: 0, data: data, header: [0x33, 0x67, 0x70, 0x35]) {
      mimeType = "video/mp4"
      fileExtension = "mp4"
      return
    }

    if findHeader(offset: 0, data: data, header: [0x00, 0x00, 0x00, 0x1C, 0x66, 0x74, 0x79, 0x70, 0x4D, 0x34, 0x56]) {
      mimeType = "video/x-m4v"
      fileExtension = "m4v"
      return
    }

    if findHeader(offset: 0, data: data, header: [0x00, 0x00, 0x00, 0x14, 0x66, 0x74, 0x79, 0x70]) {
      mimeType = "video/quicktime"
      fileExtension = "mov"
      return
    }

    if findHeader(offset: 0, data: data, header: [0x52, 0x49, 0x46, 0x46]) && findHeader(offset: 8, data: data, header: [0x41, 0x56, 0x49]) {
      mimeType = "video/x-msvideo"
      fileExtension = "avi"
      return
    }

    if findHeader(offset: 0, data: data, header: [0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9]) {
      mimeType = "video/x-ms-wmv"
      fileExtension = "wmv"
      return
    }

    // Maybe
    if findHeader(offset: 0, data: data, header: [0x00, 0x00, 0x01]) {
      mimeType = "video/mpeg"
      fileExtension = "mpg"
      return
    }

    if findHeader(offset: 0, data: data, header: [0x49, 0x44, 0x33]) || findHeader(offset: 0, data: data, header: [0xFF, 0xFB]) {
      mimeType = "audio/mpeg"
      fileExtension = "mp3"
      return
    }

    if findHeader(offset: 0, data: data, header: [0x4D, 0x34, 0x41, 0x20]) || findHeader(offset: 4, data: data, header: [0x66, 0x74, 0x79, 0x70, 0x4D, 0x34, 0x41]) {
      mimeType = "audio/m4a"
      fileExtension = "m4a"
      return
    }

    if findHeader(offset: 0, data: data, header: [0x23, 0x21, 0x41, 0x4D, 0x52, 0x0A]) {
      mimeType = "audio/amr"
      fileExtension = "amr"
      return
    }

    if findHeader(offset: 0, data: data, header: [0x46, 0x4C, 0x56, 0x01]) {
      mimeType = "video/x-flv"
      fileExtension = "flv"
      return
    }

    mimeType = "application/x-mpegURL"  // application/vnd.apple.mpegurl
    fileExtension = nil
  }

  private func findHeader(offset: Int, data: Data, header: [UInt8]) -> Bool {
    if offset < 0 || data.count < offset + header.count {
      return false
    }

    return [UInt8](data[offset..<(offset + header.count)]) == header
  }

  /// Converts a list of AVFileType to a list of file extensions
  private static func supportedAVAssetFileExtensions() -> [String] {
    let types = AVURLAsset.audiovisualTypes()
    return types.compactMap({ UTType($0.rawValue)?.preferredFilenameExtension }).filter({ !$0.isEmpty })
  }

  /// Converts a list of AVFileType to a list of mime-types
  private static func supportedAVAssetMimeTypes() -> [String] {
    let types = AVURLAsset.audiovisualTypes()
    return types.compactMap({ UTType($0.rawValue)?.preferredMIMEType }).filter({ !$0.isEmpty })
  }

  private let knownFileExtensions = [
    "mov",
    "qt",
    "mp4",
    "m4v",
    "m4a",
    "m4b",  // DRM protected
    "m4p",  // DRM protected
    "3gp",
    "3gpp",
    "sdv",
    "3g2",
    "3gp2",
    "caf",
    "wav",
    "wave",
    "bwf",
    "aif",
    "aiff",
    "aifc",
    "cdda",
    "amr",
    "mp3",
    "au",
    "snd",
    "ac3",
    "eac3",
    "flac",
    "aac",
    "mp2",
    "pls",
    "avi",
    "webm",
    "ogg",
    "mpg",
    "mpg4",
    "mpeg",
    "mpg3",
    "wma",
    "wmv",
    "swf",
    "flv",
    "mng",
    "asx",
    "asf",
    "mkv",
  ]

  private let mimeTypeMap = [
    "audio/x-wav": "wav",
    "audio/vnd.wave": "wav",
    "audio/aacp": "aacp",
    "audio/mpeg3": "mp3",
    "audio/mp3": "mp3",
    "audio/x-caf": "caf",
    "audio/mpeg": "mp3",  // mpg3
    "audio/x-mpeg3": "mp3",
    "audio/wav": "wav",
    "audio/flac": "flac",
    "audio/x-flac": "flac",
    "audio/mp4": "mp4",
    "audio/x-mpg": "mp3",  // maybe mpg3
    "audio/scpls": "pls",
    "audio/x-aiff": "aiff",
    "audio/usac": "eac3",  // Extended AC3
    "audio/x-mpeg": "mp3",
    "audio/wave": "wav",
    "audio/x-m4r": "m4r",
    "audio/x-mp3": "mp3",
    "audio/amr": "amr",
    "audio/aiff": "aiff",
    "audio/3gpp2": "3gp2",
    "audio/aac": "aac",
    "audio/mpg": "mp3",  // mpg3
    "audio/mpegurl": "mpg",  // actually .m3u8, .m3u HLS stream
    "audio/x-m4b": "m4b",
    "audio/x-m4p": "m4p",
    "audio/x-scpls": "pls",
    "audio/x-mpegurl": "mpg",  // actually .m3u8, .m3u HLS stream
    "audio/x-aac": "aac",
    "audio/3gpp": "3gp",
    "audio/basic": "au",
    "audio/au": "au",
    "audio/snd": "snd",
    "audio/x-m4a": "m4a",
    "audio/x-realaudio": "ra",
    "video/3gpp2": "3gp2",
    "video/quicktime": "mov",
    "video/mp4": "mp4",
    "video/mp4v": "mp4",
    "video/mpg": "mpg",
    "video/mpeg": "mpeg",
    "video/x-mpg": "mpg",
    "video/x-mpeg": "mpeg",
    "video/avi": "avi",
    "video/x-m4v": "m4v",
    "video/mp2t": "ts",
    "application/vnd.apple.mpegurl": "mpg",  // actually .m3u8, .m3u HLS stream
    "video/3gpp": "3gp",
    "text/vtt": "vtt",  // Subtitles format
    "application/mp4": "mp4",
    "application/x-mpegurl": "mpg",  // actually .m3u8, .m3u HLS stream
    "video/webm": "webm",
    "application/ogg": "ogg",
    "video/msvideo": "avi",
    "video/x-msvideo": "avi",
    "video/x-ms-wmv": "wmv",
    "video/x-ms-wma": "wma",
    "application/x-shockwave-flash": "swf",
    "video/x-flv": "flv",
    "video/x-mng": "mng",
    "video/x-ms-asx": "asx",
    "video/x-ms-asf": "asf",
    "video/matroska": "mkv",
  ]
}

class PlaylistWebLoader: UIView {
  fileprivate static var pageLoadTimeout = 300.0
  private var pendingHTTPUpgrades = [String: URLRequest]()
  private var pendingRequests = [String: URLRequest]()

  private let tab = Tab(
    configuration: WKWebViewConfiguration().then {
      $0.processPool = WKProcessPool()
      $0.preferences = WKPreferences()
      $0.preferences.javaScriptCanOpenWindowsAutomatically = false
      $0.allowsInlineMediaPlayback = true
      $0.ignoresViewportScaleLimits = true
    }, type: .private
  ).then {
    $0.createWebview()
    $0.setScript(script: .playlistMediaSource, enabled: Preferences.Playlist.webMediaSourceCompatibility.value)
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

  func load(url: URL, handler: @escaping (PlaylistInfo?) -> Void) {
    self.handler = { [weak self] in
      // Handler cannot be called more than once!
      self?.handler = nil
      handler($0)
    }
    
    guard let webView = tab.webView,
          let browserViewController = self.currentScene?.browserViewController else {
      self.handler?(nil)
      return
    }
    
    self.certStore = browserViewController.profile.certStore
    let KVOs: [KVOConstants] = [
      .estimatedProgress, .loading, .canGoBack,
      .canGoForward, .URL, .title,
      .hasOnlySecureContent, .serverTrust,
    ]

    browserViewController.tab(tab, didCreateWebView: webView)
    KVOs.forEach { webView.removeObserver(browserViewController, forKeyPath: $0.rawValue) }

    // When creating a tab, TabManager automatically adds a uiDelegate
    // This webView is invisible and we don't want any UI being handled.
    webView.uiDelegate = nil
    webView.navigationDelegate = self
    
    tab.replaceContentScript(PlaylistWebLoaderContentHelper(self),
                             name: PlaylistWebLoaderContentHelper.scriptName,
                             forTab: tab)

    webView.frame = superview?.bounds ?? self.bounds
    webView.load(URLRequest(url: url, cachePolicy: .reloadIgnoringCacheData, timeoutInterval: 60.0))
  }

  func stop() {
    guard let webView = tab.webView else { return }
    webView.stopLoading()
    DispatchQueue.main.async {
      self.handler?(nil)
      webView.loadHTMLString("<html><body>PlayList</body></html>", baseURL: nil)
    }
  }

  private class PlaylistWebLoaderContentHelper: TabContentScript {
    private weak var webLoader: PlaylistWebLoader?
    private var playlistItems = Set<String>()
    private var isPageLoaded = false
    private var timeout: DispatchWorkItem?

    init(_ webLoader: PlaylistWebLoader) {
      self.webLoader = webLoader
      
      timeout = DispatchWorkItem(block: { [weak self] in
        guard let self = self else { return }
        self.webLoader?.handler?(nil)
        self.webLoader?.tab.webView?.loadHTMLString("<html><body>PlayList</body></html>", baseURL: nil)
        self.webLoader = nil
      })

      if let timeout = timeout {
        DispatchQueue.main.asyncAfter(deadline: .now() + PlaylistWebLoader.pageLoadTimeout, execute: timeout)
      }
    }

    static let scriptName = "PlaylistScript"
    static let scriptId = PlaylistScriptHandler.scriptId
    static let messageHandlerName = PlaylistScriptHandler.messageHandlerName
    static let scriptSandbox = PlaylistScriptHandler.scriptSandbox
    static let userScript: WKUserScript? = nil
    static let playlistProcessDocumentLoad = PlaylistScriptHandler.playlistProcessDocumentLoad

    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: (Any?, String?) -> Void) {
      if !verifyMessage(message: message) {
        assertionFailure("Missing required security token.")
        return
      }
      
      replyHandler(nil, nil)
      
      let cancelRequest = {
        self.timeout?.cancel()
        self.timeout = nil
        self.webLoader?.handler?(nil)
        self.webLoader?.tab.webView?.loadHTMLString("<html><body>PlayList</body></html>", baseURL: nil)
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
            self.webLoader?.tab.webView?.loadHTMLString("<html><body>PlayList</body></html>", baseURL: nil)
            self.webLoader = nil
          })
            
          if let timeout = timeout {
            DispatchQueue.main.asyncAfter(deadline: .now() + PlaylistWebLoader.pageLoadTimeout, execute: timeout)
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

      // For now, we ignore base64 video mime-types loaded via the `data:` scheme.
      if item.duration <= 0.0 && !item.detected || item.src.isEmpty || item.src.hasPrefix("data:") || item.src.hasPrefix("blob:") {
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
        self.webLoader?.tab.webView?.loadHTMLString("<html><body>PlayList</body></html>", baseURL: nil)
        self.webLoader = nil
      }
    }
  }
}

extension PlaylistWebLoader: WKNavigationDelegate {
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
    if url.scheme == "http" || url.scheme == "https" {
      if url.host == "itunes.apple.com" {
        return true
      }
    }
    if url.scheme == "itms-appss" || url.scheme == "itmss" {
      return true
    }
    return false
  }
  
  func webView(_ webView: WKWebView, didCommit navigation: WKNavigation!) {
    webView.evaluateSafeJavaScript(functionName: "window.__firefox__.\(PlaylistWebLoaderContentHelper.playlistProcessDocumentLoad)()",
                                   args: [],
                                   contentWorld: PlaylistWebLoaderContentHelper.scriptSandbox,
                                   asFunction: false)
  }

  func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
    self.handler?(nil)
  }

  func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, preferences: WKWebpagePreferences) async -> (WKNavigationActionPolicy, WKWebpagePreferences) {
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
      let universalLink = UniversalLinkManager.universalLinkType(for: mainDocURL, checkPath: true) {
      switch universalLink {
      case .buyVPN:
        return (.cancel, preferences)
      }
    }

    // First special case are some schemes that are about Calling. We prompt the user to confirm this action. This
    // gives us the exact same behaviour as Safari.
    if url.scheme == "tel" || url.scheme == "facetime" || url.scheme == "facetime-audio" || url.scheme == "mailto" || isAppleMapsURL(url) || isStoreURL(url) {
      return (.cancel, preferences)
    }
    
    // Ad-blocking checks
    if let mainDocumentURL = navigationAction.request.mainDocumentURL {
      if mainDocumentURL != tab.currentPageData?.mainFrameURL {
        // Clear the current page data if the page changes.
        // Do this before anything else so that we have a clean slate.
        tab.currentPageData = PageData(mainFrameURL: mainDocumentURL)
      }
      
      let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
      let domainForMainFrame = Domain.getOrCreate(forUrl: mainDocumentURL, persistent: !isPrivateBrowsing)
      
      if let requestURL = navigationAction.request.url,
         let targetFrame = navigationAction.targetFrame {
        tab.currentPageData?.addSubframeURL(forRequestURL: requestURL, isForMainFrame: targetFrame.isMainFrame)
        let scriptTypes = await tab.currentPageData?.makeUserScriptTypes(domain: domainForMainFrame) ?? []
        tab.setCustomUserScript(scripts: scriptTypes)
      }
      
      webView.configuration.preferences.isFraudulentWebsiteWarningEnabled = domainForMainFrame.isShieldExpected(.SafeBrowsing, considerAllShieldsOption: true)
    }

    if ["http", "https", "data", "blob", "file"].contains(url.scheme) {
      if navigationAction.targetFrame?.isMainFrame == true {
        tab.updateUserAgent(webView, newURL: url)
      }

      pendingRequests[url.absoluteString] = navigationAction.request

      if let mainDocumentURL = navigationAction.request.mainDocumentURL,
        mainDocumentURL.schemelessAbsoluteString == url.schemelessAbsoluteString,
        !(InternalURL(url)?.isSessionRestore ?? false),
        navigationAction.sourceFrame.isMainFrame || navigationAction.targetFrame?.isMainFrame == true {

        // Identify specific block lists that need to be applied to the requesting domain
        let domainForShields = Domain.getOrCreate(forUrl: mainDocumentURL, persistent: false)

        // Force adblocking on
        domainForShields.shield_allOff = 0
        domainForShields.shield_adblockAndTp = true
        
        // Load block lists
        let ruleLists = await ContentBlockerManager.shared.ruleLists(for: domainForShields)
        tab.contentBlocker.set(ruleLists: ruleLists)

        let isScriptsEnabled = !domainForShields.isShieldExpected(.NoScript, considerAllShieldsOption: true)
        preferences.allowsContentJavaScript = isScriptsEnabled
      }

      // Cookie Blocking code below
      tab.setScript(script: .cookieBlocking, enabled: Preferences.Privacy.blockAllCookies.value)

      return (.allow, preferences)
    }

    return (.cancel, preferences)
  }

  func webView(_ webView: WKWebView, decidePolicyFor navigationResponse: WKNavigationResponse) async -> WKNavigationResponsePolicy {
    let response = navigationResponse.response
    let responseURL = response.url

    if let responseURL = responseURL,
      let internalURL = InternalURL(responseURL),
      internalURL.isSessionRestore {
      tab.shouldClassifyLoadsForAds = false
    }
    
    // We also add subframe urls in case a frame upgraded to https
    if let responseURL = responseURL,
       tab.currentPageData?.upgradeFrameURL(forResponseURL: responseURL, isForMainFrame: navigationResponse.isForMainFrame) == true,
       let domain = tab.currentPageData?.domain(persistent: false) {
      let scriptTypes = await tab.currentPageData?.makeUserScriptTypes(domain: domain) ?? []
      tab.setCustomUserScript(scripts: scriptTypes)
    }

    var request: URLRequest?
    if let url = responseURL {
      request = pendingRequests.removeValue(forKey: url.absoluteString)
    }

    if let url = responseURL, let urlHost = responseURL?.normalizedHost() {
      // If an upgraded https load happens with a host which was upgraded, increase the stats
      if url.scheme == "https", let _ = pendingHTTPUpgrades.removeValue(forKey: urlHost) {
        BraveGlobalShieldStats.shared.httpse += 1
        tab.contentBlocker.stats = tab.contentBlocker.stats.adding(httpsCount: 1)
      }
    }

    // TODO: REFACTOR to support Multiple Windows Better
    if let browserController = webView.currentScene?.browserViewController {
      // Check if this response should be handed off to Passbook.
      if OpenPassBookHelper(request: request, response: response, canShowInWebView: false, forceDownload: false, browserViewController: browserController) != nil {
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

  public func webView(_ webView: WKWebView, respondTo challenge: URLAuthenticationChallenge) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    let origin = "\(challenge.protectionSpace.host):\(challenge.protectionSpace.port)"
    if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
       let trust = challenge.protectionSpace.serverTrust,
       let cert = (SecTrustCopyCertificateChain(trust) as? [SecCertificate])?.first,
       certStore?.containsCertificate(cert, forOrigin: origin) == true {
      return (.useCredential, URLCredential(trust: trust))
    }
    
    if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
       let serverTrust = challenge.protectionSpace.serverTrust {
      let host = challenge.protectionSpace.host
      let port = challenge.protectionSpace.port
      
      let result = BraveCertificateUtility.verifyTrust(serverTrust,
                                                       host: host,
                                                       port: port)
      
      if result == 0 {
        return (.useCredential, URLCredential(trust: serverTrust))
      }
      
      if result == Int32.min {
        return (.performDefaultHandling, nil)
      }
      
      return (.cancelAuthenticationChallenge, nil)
    }

    guard challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPBasic || challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPDigest || challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodNTLM else {
      return (.performDefaultHandling, nil)
    }

    return (.cancelAuthenticationChallenge, nil)
  }
}
