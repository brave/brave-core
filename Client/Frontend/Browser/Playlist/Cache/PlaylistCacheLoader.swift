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
import BraveShared

private let log = Logger.browserLogger

// IANA List of Audio types: https://www.iana.org/assignments/media-types/media-types.xhtml#audio
// IANA List of Video types: https://www.iana.org/assignments/media-types/media-types.xhtml#video
// APPLE List of UTI types: https://developer.apple.com/library/archive/documentation/Miscellaneous/Reference/UTIRef/Articles/System-DeclaredUniformTypeIdentifiers.html

public class PlaylistMimeTypeDetector {
    private(set) var mimeType: String?
    private(set) var fileExtension: String? // When nil, assume `mpg` format.
    
    init(url: URL) {
        let possibleFileExtension = url.pathExtension.lowercased()
        if let supportedExtension = knownFileExtensions.first(where: { $0.lowercased() == possibleFileExtension }) {
            self.fileExtension = supportedExtension
            self.mimeType = mimeTypeMap.first(where: { $0.value == supportedExtension })?.key
        } else if let fileExtension = PlaylistMimeTypeDetector.supportedAVAssetFileExtensions().first(where: { $0.lowercased() == possibleFileExtension }) {
            self.fileExtension = fileExtension
            self.mimeType = PlaylistMimeTypeDetector.fileExtensionToMimeType(fileExtension)
        }
    }
    
    init(mimeType: String) {
        if let fileExtension = mimeTypeMap[mimeType.lowercased()] {
            self.mimeType = mimeType
            self.fileExtension = fileExtension
        } else if let mimeType = PlaylistMimeTypeDetector.supportedAVAssetMimeTypes().first(where: { $0.lowercased() == mimeType.lowercased() }) {
            self.mimeType = mimeType
            self.fileExtension = PlaylistMimeTypeDetector.mimeTypeToFileExtension(mimeType)
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
        
        if findHeader(offset: 0, data: data, header: [0x52, 0x49, 0x46, 0x46]) &&
            findHeader(offset: 8, data: data, header: [0x57, 0x41, 0x56, 0x45]) {
            mimeType = "audio/x-wav"
            fileExtension = "wav"
            return
        }
        
        if findHeader(offset: 0, data: data, header: [0xFF, 0xFB]) ||
            findHeader(offset: 0, data: data, header: [0x49, 0x44, 0x33]) {
            mimeType = "audio/mpeg"
            fileExtension = "mp4"
            return
        }
        
        if findHeader(offset: 0, data: data, header: [0x66, 0x4C, 0x61, 0x43]) {
            mimeType = "audio/flac"
            fileExtension = "flac"
            return
        }
        
        if findHeader(offset: 4, data: data, header: [0x66, 0x74, 0x79, 0x70, 0x4D, 0x53, 0x4E, 0x56]) ||
            findHeader(offset: 4, data: data, header: [0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6F, 0x6D]) ||
            findHeader(offset: 4, data: data, header: [0x66, 0x74, 0x79, 0x70, 0x6D, 0x70, 0x34, 0x32]) ||
            findHeader(offset: 0, data: data, header: [0x33, 0x67, 0x70, 0x35]) {
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
        
        if findHeader(offset: 0, data: data, header: [0x52, 0x49, 0x46, 0x46]) &&
            findHeader(offset: 8, data: data, header: [0x41, 0x56, 0x49]) {
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
        
        if findHeader(offset: 0, data: data, header: [0x49, 0x44, 0x33]) ||
            findHeader(offset: 0, data: data, header: [0xFF, 0xFB]) {
            mimeType = "audio/mpeg"
            fileExtension = "mp3"
            return
        }
        
        if findHeader(offset: 0, data: data, header: [0x4D, 0x34, 0x41, 0x20]) ||
            findHeader(offset: 4, data: data, header: [0x66, 0x74, 0x79, 0x70, 0x4D, 0x34, 0x41]) {
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
        
        mimeType = "application/x-mpegURL" // application/vnd.apple.mpegurl
        fileExtension = nil
    }
    
    private func findHeader(offset: Int, data: Data, header: [UInt8]) -> Bool {
        if offset < 0 || data.count < offset + header.count {
            return false
        }
        
        return [UInt8](data[offset..<(offset + header.count)]) == header
    }
    
    /// Converts a File Extension to a Mime-Type
    private static func fileExtensionToMimeType(_ fileExtension: String) -> String? {
        if #available(iOS 14.0, *) {
            return UTType(tag: fileExtension, tagClass: .filenameExtension, conformingTo: nil)?.preferredMIMEType
        } else {
            if let tag = UTTypeCreatePreferredIdentifierForTag(kUTTagClassFilenameExtension, fileExtension as CFString, nil)?.takeRetainedValue() {
                return UTTypeCopyPreferredTagWithClass(tag, kUTTagClassMIMEType)?.takeRetainedValue() as String?
            }
            return nil
        }
    }
    
    /// Converts a Mime-Type to File Extension
    private static func mimeTypeToFileExtension(_ mimeType: String) -> String? {
        if #available(iOS 14.0, *) {
            return UTType(tag: mimeType, tagClass: .mimeType, conformingTo: nil)?.preferredFilenameExtension
        } else {
            if let tag = UTTypeCreatePreferredIdentifierForTag(kUTTagClassMIMEType, mimeType as CFString, nil)?.takeRetainedValue() {
                return UTTypeCopyPreferredTagWithClass(tag, kUTTagClassFilenameExtension)?.takeRetainedValue() as String?
            }
            return nil
        }
    }
    
    /// Converts a list of AVFileType to a list of file extensions
    private static func supportedAVAssetFileExtensions() -> [String] {
        if #available(iOS 14.0, *) {
            let types = AVURLAsset.audiovisualTypes()
            return types.compactMap({ UTType($0.rawValue)?.preferredFilenameExtension }).filter({ !$0.isEmpty })
        } else {
            let types = AVURLAsset.audiovisualTypes()
            return types.compactMap({ UTTypeCopyPreferredTagWithClass($0 as CFString, kUTTagClassFilenameExtension)?.takeRetainedValue() as String? }).filter({ !$0.isEmpty })
        }
    }
    
    /// Converts a list of AVFileType to a list of mime-types
    private static func supportedAVAssetMimeTypes() -> [String] {
        if #available(iOS 14.0, *) {
            let types = AVURLAsset.audiovisualTypes()
            return types.compactMap({ UTType($0.rawValue)?.preferredMIMEType }).filter({ !$0.isEmpty })
        } else {
            let types = AVURLAsset.audiovisualTypes()
            return types.compactMap({ UTTypeCopyPreferredTagWithClass($0 as CFString, kUTTagClassMIMEType)?.takeRetainedValue() as String? }).filter({ !$0.isEmpty })
        }
    }
    
    private let knownFileExtensions = [
        "mov",
        "qt",
        "mp4",
        "m4v",
        "m4a",
        "m4b", // DRM protected
        "m4p", // DRM protected
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
        "mkv"
    ]
    
    private let mimeTypeMap = [
        "audio/x-wav": "wav",
        "audio/vnd.wave": "wav",
        "audio/aacp": "aacp",
        "audio/mpeg3": "mp3",
        "audio/mp3": "mp3",
        "audio/x-caf": "caf",
        "audio/mpeg": "mp3", // mpg3
        "audio/x-mpeg3": "mp3",
        "audio/wav": "wav",
        "audio/flac": "flac",
        "audio/x-flac": "flac",
        "audio/mp4": "mp4",
        "audio/x-mpg": "mp3", // maybe mpg3
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
        "audio/mpg": "mp3", // mpg3
        "audio/mpegurl": "mpg", // actually .m3u8, .m3u HLS stream
        "audio/x-m4b": "m4b",
        "audio/x-m4p": "m4p",
        "audio/x-scpls": "pls",
        "audio/x-mpegurl": "mpg", // actually .m3u8, .m3u HLS stream
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
        "application/vnd.apple.mpegurl": "mpg", // actually .m3u8, .m3u HLS stream
        "video/3gpp": "3gp",
        "text/vtt": "vtt",  // Subtitles format
        "application/mp4": "mp4",
        "application/x-mpegurl": "mpg", // actually .m3u8, .m3u HLS stream
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
        "video/matroska": "mkv"
    ]
}

class PlaylistWebLoader: UIView {
    fileprivate static var pageLoadTimeout = 10.0
    
    private let safeBrowsing = SafeBrowsing()
    private var pendingHTTPUpgrades = [String: URLRequest]()
    private var pendingRequests = [String: URLRequest]()
    
    private let tab = Tab(configuration: WKWebViewConfiguration().then {
        $0.processPool = WKProcessPool()
        $0.preferences = WKPreferences()
        $0.preferences.javaScriptCanOpenWindowsAutomatically = false
        $0.allowsInlineMediaPlayback = true
        $0.ignoresViewportScaleLimits = true
        // $0.mediaTypesRequiringUserActionForPlayback = []
    }, type: .private).then {
        $0.createWebview()
        $0.webView?.scrollView.layer.masksToBounds = true
    }
    
    private let playlistDetectorScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "PlaylistDetector", ofType: "js"), let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load PlaylistDetector.js")
            return nil
        }
        
        var alteredSource = source
        let token = UserScriptManager.securityToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)
        
        let replacements = [
            "$<PlaylistDetector>": "PlaylistDetector_\(token)",
            "$<security_token>": "\(token)",
            "$<sendMessage>": "playlistDetector_sendMessage_\(token)",
            "$<handler>": "playlistCacheLoader_\(UserScriptManager.messageHandlerTokenString)",
            "$<notify>": "notify_\(token)",
            "$<onLongPressActivated>": "onLongPressActivated_\(token)",
            "$<setupLongPress>": "setupLongPress_\(token)",
            "$<setupDetector>": "setupDetector_\(token)",
            "$<notifyNodeSource>": "notifyNodeSource_\(token)",
            "$<notifyNode>": "notifyNode_\(token)",
            "$<observeNode>": "observeNode_\(token)",
            "$<observeDocument>": "observeDocument_\(token)",
            "$<observeDynamicElements>": "observeDynamicElements_\(token)",
            "$<getAllVideoElements>": "getAllVideoElements_\(token)",
            "$<getAllAudioElements>": "getAllAudioElements_\(token)",
            "$<onReady>": "onReady_\(token)",
            "$<observePage>": "observePage_\(token)",
        ]
        
        replacements.forEach({
            alteredSource = alteredSource.replacingOccurrences(of: $0.key, with: $0.value, options: .literal)
        })
        return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false)
    }()
    
    private var handler: (PlaylistInfo?) -> Void
    
    init(handler: @escaping (PlaylistInfo?) -> Void) {
        self.handler = handler
        super.init(frame: .zero)

        guard let webView = tab.webView else {
            handler(nil)
            return
        }
        
        self.addSubview(webView)
        webView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        
        if let browserController = (UIApplication.shared.delegate as? AppDelegate)?.browserViewController {
            let KVOs: [KVOConstants] = [
                .estimatedProgress, .loading, .canGoBack,
                .canGoForward, .URL, .title,
                .hasOnlySecureContent, .serverTrust
            ]
            
            browserController.tab(tab, didCreateWebView: webView)
            KVOs.forEach { webView.removeObserver(browserController, forKeyPath: $0.rawValue) }
            webView.scrollView.removeObserver(browserController.scrollController, forKeyPath: KVOConstants.contentSize.rawValue)
        }
        
        // When creating a tab, TabManager automatically adds a uiDelegate
        // This webView is invisible and we don't want any UI being handled.
        webView.uiDelegate = nil
        webView.navigationDelegate = self
        tab.addContentScript(PlaylistWebLoaderContentHelper(self), name: PlaylistWebLoaderContentHelper.name(), sandboxed: false)
        
        if let script = playlistDetectorScript {
            // Do NOT inject the PlaylistHelper script!
            // The Playlist Detector script will interfere with it.
            // The detector script is only to be used in the background in an invisible webView
            // but the helper script is to be used in the foreground!
            tab.userScriptManager?.isPlaylistEnabled = false
            
            tab.webView?.configuration.userContentController.do {
                $0.addUserScript(script)
            }
        }
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    deinit {
        self.removeFromSuperview()
    }
    
    func load(url: URL) {
        guard let webView = tab.webView else { return }
        webView.frame = self.window?.bounds ?? .zero
        webView.load(URLRequest(url: url, cachePolicy: .reloadIgnoringCacheData, timeoutInterval: 60.0))
    }
    
    private class PlaylistWebLoaderContentHelper: TabContentScript {
        private weak var webLoader: PlaylistWebLoader?
        private var playlistItems = Set<String>()
        private var isPageLoaded = false
        private var timeout: DispatchWorkItem?
        
        init(_ webLoader: PlaylistWebLoader) {
            self.webLoader = webLoader
        }
        
        static func name() -> String {
            return "PlaylistWebLoader"
        }
        
        func scriptMessageHandlerName() -> String? {
            return "playlistCacheLoader_\(UserScriptManager.messageHandlerTokenString)"
        }
        
        func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {

            if let info = PageInfo.from(message: message) {
                isPageLoaded = info.pageLoad
                
                if isPageLoaded {
                    timeout?.cancel()
                    timeout = DispatchWorkItem(block: { [weak self] in
                        guard let self = self else { return }
                        self.webLoader?.handler(nil)
                        self.webLoader?.tab.webView?.loadHTMLString("<html><body>PlayList</body></html>", baseURL: nil)
                    })
                    
                    if let timeout = timeout {
                        DispatchQueue.main.asyncAfter(deadline: .now() + PlaylistWebLoader.pageLoadTimeout, execute: timeout)
                    }
                }
                return
            }
            
            guard let item = PlaylistInfo.from(message: message),
                  item.detected else {
                timeout?.cancel()
                timeout = nil
                webLoader?.handler(nil)
                self.webLoader?.tab.webView?.loadHTMLString("<html><body>PlayList</body></html>", baseURL: nil)
                return
            }
            
            // For now, we ignore base64 video mime-types loaded via the `data:` scheme.
            if item.duration <= 0.0 && !item.detected || item.src.isEmpty || item.src.hasPrefix("data:") || item.src.hasPrefix("blob:") {
                timeout?.cancel()
                timeout = nil
                webLoader?.handler(nil)
                self.webLoader?.tab.webView?.loadHTMLString("<html><body>PlayList</body></html>", baseURL: nil)
                return
            }
            
            // We have to create an AVURLAsset here to determine if the item is playable
            // because otherwise it will add an invalid item to playlist that can't be played.
            // IE: WebM videos aren't supported so can't be played.
            // Therefore we shouldn't prompt the user to add to playlist.
            if let url = URL(string: item.src), !AVURLAsset(url: url).isPlayable {
                timeout?.cancel()
                timeout = nil
                webLoader?.handler(nil)
                self.webLoader?.tab.webView?.loadHTMLString("<html><body>PlayList</body></html>", baseURL: nil)
                return
            }
            
            if !playlistItems.contains(item.src) {
                playlistItems.insert(item.src)
                
                timeout?.cancel()
                timeout = nil
                webLoader?.handler(item)
            }
            
            // This line MAY cause problems.. because some websites have a loading delay for the source of the media item
            // If the second we receive the src, we reload the page by doing the below HTML,
            // It may not have received all info necessary to play the item such as MetadataInfo
            // For now it works 100% of the time and it is safe to do it. If we come across such a website, that causes problems,
            // we'll need to find a different way of forcing the WebView to STOP loading metadata in the background
            DispatchQueue.main.async {
                self.webLoader?.tab.webView?.loadHTMLString("<html><body>PlayList</body></html>", baseURL: nil)
            }
        }
    }
    
    private struct PageInfo: Codable {
        let pageLoad: Bool
        
        public static func from(message: WKScriptMessage) -> PageInfo? {
            if !JSONSerialization.isValidJSONObject(message.body) {
                return nil
            }
            
            do {
                let data = try JSONSerialization.data(withJSONObject: message.body, options: [.fragmentsAllowed])
                return try JSONDecoder().decode(PageInfo.self, from: data)
            } catch {
                log.error("Error Decoding PageInfo: \(error)")
            }
            
            return nil
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
    
    func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
        self.handler(nil)
    }

    func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
        guard let url = navigationAction.request.url else {
            decisionHandler(.cancel)
            return
        }

        if url.scheme == "about" || url.isBookmarklet {
            decisionHandler(.cancel)
            return
        }

        if !navigationAction.isAllowed && navigationAction.navigationType != .backForward {
            decisionHandler(.cancel)
            return
        }
        
        if safeBrowsing.shouldBlock(url) {
            decisionHandler(.cancel)
            return
        }
        
        // Universal links do not work if the request originates from the app, manual handling is required.
        if let mainDocURL = navigationAction.request.mainDocumentURL,
           let universalLink = UniversalLinkManager.universalLinkType(for: mainDocURL, checkPath: true) {
            switch universalLink {
            case .buyVPN:
                decisionHandler(.cancel)
                return
            }
        }

        // First special case are some schemes that are about Calling. We prompt the user to confirm this action. This
        // gives us the exact same behaviour as Safari.
        if url.scheme == "tel" ||
            url.scheme == "facetime" ||
            url.scheme == "facetime-audio" ||
            url.scheme == "mailto" ||
            isAppleMapsURL(url) ||
            isStoreURL(url) {
            decisionHandler(.cancel)
            return
        }
        
        tab.userScriptManager?.handleDomainUserScript(for: url)
        
        // For Playlist automatic detection since the above `handleDomainUserScript` removes ALL scripts!
        if let script = playlistDetectorScript {
            tab.webView?.configuration.userContentController.do {
                $0.addUserScript(script)
            }
        }

        if ["http", "https", "data", "blob", "file"].contains(url.scheme) {
            if navigationAction.targetFrame?.isMainFrame == true {
                tab.updateUserAgent(webView, newURL: url)
            }

            pendingRequests[url.absoluteString] = navigationAction.request
            
            if let urlHost = url.normalizedHost() {
                if let mainDocumentURL = navigationAction.request.mainDocumentURL, url.scheme == "http" {
                    let domainForShields = Domain.getOrCreate(forUrl: mainDocumentURL, persistent: false)
                    if domainForShields.isShieldExpected(.HTTPSE, considerAllShieldsOption: true) && HttpsEverywhereStats.shared.shouldUpgrade(url) {
                        // Check if HTTPSE is on and if it is, whether or not this http url would be upgraded
                        pendingHTTPUpgrades[urlHost] = navigationAction.request
                    }
                }
            }

            if
                let mainDocumentURL = navigationAction.request.mainDocumentURL,
                mainDocumentURL.schemelessAbsoluteString == url.schemelessAbsoluteString,
                !url.isSessionRestoreURL,
                navigationAction.sourceFrame.isMainFrame || navigationAction.targetFrame?.isMainFrame == true {
                
                // Identify specific block lists that need to be applied to the requesting domain
                let domainForShields = Domain.getOrCreate(forUrl: mainDocumentURL, persistent: false)
                
                // Force adblocking on
                domainForShields.shield_allOff = 1
                domainForShields.shield_adblockAndTp = true
                domainForShields.shield_httpse = true
                
                let (on, off) = BlocklistName.blocklists(forDomain: domainForShields)
                let controller = webView.configuration.userContentController
                
                // Grab all lists that have valid rules and add/remove them as necessary
                on.compactMap { $0.rule }.forEach(controller.add)
                off.compactMap { $0.rule }.forEach(controller.remove)
              
                tab.userScriptManager?.isFingerprintingProtectionEnabled =
                    domainForShields.isShieldExpected(.FpProtection, considerAllShieldsOption: true)

                webView.configuration.preferences.javaScriptEnabled = !domainForShields.isShieldExpected(.NoScript, considerAllShieldsOption: true)
            }
            
            // Cookie Blocking code below
            tab.userScriptManager?.isCookieBlockingEnabled = Preferences.Privacy.blockAllCookies.value
            
            if let rule = BlocklistName.cookie.rule {
                if Preferences.Privacy.blockAllCookies.value {
                    webView.configuration.userContentController.add(rule)
                } else {
                    webView.configuration.userContentController.remove(rule)
                }
            }
            
            decisionHandler(.allow)
            return
        }

        decisionHandler(.cancel)
    }

    func webView(_ webView: WKWebView, decidePolicyFor navigationResponse: WKNavigationResponse, decisionHandler: @escaping (WKNavigationResponsePolicy) -> Void) {
        let response = navigationResponse.response
        let responseURL = response.url
        
        if responseURL?.isSessionRestoreURL == true {
            tab.shouldClassifyLoadsForAds = false
        }
        
        var request: URLRequest?
        if let url = responseURL {
            request = pendingRequests.removeValue(forKey: url.absoluteString)
        }

        if let url = responseURL, let urlHost = responseURL?.normalizedHost() {
            // If an upgraded https load happens with a host which was upgraded, increase the stats
            if url.scheme == "https", let _ = pendingHTTPUpgrades.removeValue(forKey: urlHost) {
                BraveGlobalShieldStats.shared.httpse += 1
                tab.contentBlocker.stats = tab.contentBlocker.stats.create(byAddingListItem: .https)
            }
        }
        
        if let browserController = (UIApplication.shared.delegate as? AppDelegate)?.browserViewController {
            // Check if this response should be handed off to Passbook.
            if OpenPassBookHelper(request: request, response: response, canShowInWebView: false, forceDownload: false, browserViewController: browserController) != nil {
                decisionHandler(.cancel)
                return
            }
        }
        
        if navigationResponse.isForMainFrame {
            if response.mimeType?.isKindOfHTML == false, request != nil {
                decisionHandler(.cancel)
                return
            } else {
                tab.temporaryDocument = nil
            }
            
            tab.mimeType = response.mimeType
        }

        decisionHandler(.allow)
    }
    
    func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, preferences: WKWebpagePreferences, decisionHandler: @escaping (WKNavigationActionPolicy, WKWebpagePreferences) -> Void) {
        
        self.webView(webView, decidePolicyFor: navigationAction) {
            decisionHandler($0, preferences)
        }
    }

    func webView(_ webView: WKWebView, didReceive challenge: URLAuthenticationChallenge, completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {
        
        guard let profile = (UIApplication.shared.delegate as? AppDelegate)?.profile else {
            completionHandler(.rejectProtectionSpace, nil)
            return
        }
        
        let origin = "\(challenge.protectionSpace.host):\(challenge.protectionSpace.port)"
        if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
           let trust = challenge.protectionSpace.serverTrust,
           let cert = SecTrustGetCertificateAtIndex(trust, 0), profile.certStore.containsCertificate(cert, forOrigin: origin) {
            completionHandler(.useCredential, URLCredential(trust: trust))
            return
        }

        guard challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPBasic ||
              challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodHTTPDigest ||
              challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodNTLM else {
            completionHandler(.performDefaultHandling, nil)
            return
        }

        completionHandler(.rejectProtectionSpace, nil)
    }
}
