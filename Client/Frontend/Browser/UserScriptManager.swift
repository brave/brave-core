/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import WebKit
import Shared
import Data

private let log = Logger.browserLogger

class UserScriptManager {

    // Scripts can use this to verify the app –not js on the page– is calling into them.
    private static let securityToken = UUID()
    
    // Ensures that the message handlers cannot be invoked by the page scripts
    private static let messageHandlerToken = UUID()
    
    // String representation of messageHandlerToken
    public static let messageHandlerTokenString = UserScriptManager.messageHandlerToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)

    // String representation of securityToken
    public static let securityTokenString = UserScriptManager.securityToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)

    private weak var tab: Tab?
    
    // Whether or not the fingerprinting protection
    var isFingerprintingProtectionEnabled: Bool {
        didSet {
            if oldValue == isFingerprintingProtectionEnabled { return }
            reloadUserScripts()
        }
    }
    
    var isCookieBlockingEnabled: Bool {
        didSet {
            if oldValue == isCookieBlockingEnabled { return }
            reloadUserScripts()
        }
    }
    
    // Whether or not the PaymentRequest APIs should be exposed
    var isPaymentRequestEnabled: Bool {
        didSet {
            if oldValue == isPaymentRequestEnabled { return }
            reloadUserScripts()
        }
    }
    
    /// Whether or not Playlist is enabled
    var isPlaylistEnabled: Bool {
        didSet {
            if oldValue == isPlaylistEnabled { return }
            reloadUserScripts()
        }
    }
    
    /// Whether or not the MediaSource API should be disabled for Playlists
    var isWebCompatibilityMediaSourceAPIEnabled: Bool {
        didSet {
            if oldValue == isWebCompatibilityMediaSourceAPIEnabled { return }
            reloadUserScripts()
        }
    }
    
    /// Whether or not the Media Background Playback is enabled
    var isMediaBackgroundPlaybackEnabled: Bool {
        didSet {
            if oldValue == isMediaBackgroundPlaybackEnabled { return }
            reloadUserScripts()
        }
    }
    
    /// Stores domain specific scriplet, usually used for webcompat workarounds.
    var domainUserScript: DomainUserScript? {
        didSet {
            if oldValue == domainUserScript { return }
            reloadUserScripts()
        }
    }
    
    func handleDomainUserScript(for url: URL) {
        guard let customDomainUserScript = DomainUserScript.get(for: url) else {
                // No custom script for this domain, clearing existing user script
                // in case the previous domain had one.
                domainUserScript = nil
                return
        }
        
        if let shieldType = customDomainUserScript.shieldType {
            let domain = Domain.getOrCreate(forUrl: url,
                                            persistent: !PrivateBrowsingManager.shared.isPrivateBrowsing)
            
            if domain.isShieldExpected(shieldType, considerAllShieldsOption: true) {
                domainUserScript = customDomainUserScript
            } else {
                // Remove old user script.
                domainUserScript = nil
            }
        } else {
            domainUserScript = customDomainUserScript
        }
    }
    
    public static func isMessageHandlerTokenMissing(in body: [String: Any]) -> Bool {
        guard let token = body["securitytoken"] as? String, token == UserScriptManager.messageHandlerTokenString else {
            return true
        }
        return false
    }
    
    init(tab: Tab, isFingerprintingProtectionEnabled: Bool, isCookieBlockingEnabled: Bool, isPaymentRequestEnabled: Bool, isWebCompatibilityMediaSourceAPIEnabled: Bool, isMediaBackgroundPlaybackEnabled: Bool) {
        self.tab = tab
        self.isFingerprintingProtectionEnabled = isFingerprintingProtectionEnabled
        self.isCookieBlockingEnabled = isCookieBlockingEnabled
        self.isPaymentRequestEnabled = isPaymentRequestEnabled
        self.isWebCompatibilityMediaSourceAPIEnabled = isWebCompatibilityMediaSourceAPIEnabled
        self.isPlaylistEnabled = true
        self.isMediaBackgroundPlaybackEnabled = isMediaBackgroundPlaybackEnabled
        reloadUserScripts()
    }
    
    // MARK: -
    
    private let packedUserScripts: [WKUserScript] = {
        [(WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false, sandboxed: false),
         (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false, sandboxed: false),
         (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false, sandboxed: true),
         (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false, sandboxed: true),
         (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: true, sandboxed: false),
         (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: true, sandboxed: false)].compactMap { arg in
            let (injectionTime, mainFrameOnly, sandboxed) = arg
            let name = (mainFrameOnly ? "MainFrame" : "AllFrames") + "AtDocument" + (injectionTime == .atDocumentStart ? "Start" : "End") + (sandboxed ? "Sandboxed" : "")
            if let path = Bundle.main.path(forResource: name, ofType: "js"),
                let source = try? NSString(contentsOfFile: path, encoding: String.Encoding.utf8.rawValue) as String {
                let wrappedSource = "(function() { const SECURITY_TOKEN = '\(UserScriptManager.messageHandlerTokenString)'; \(source) })()"

                if sandboxed {
                    return WKUserScript.createInDefaultContentWorld(source: wrappedSource, injectionTime: injectionTime, forMainFrameOnly: mainFrameOnly)
                } else {
                    return WKUserScript(source: wrappedSource, injectionTime: injectionTime, forMainFrameOnly: mainFrameOnly)
                }
            }
            return nil
        }
    }()
    
    private let fingerprintingProtectionUserScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "FingerprintingProtection", ofType: "js"), let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load fingerprinting protection user script")
            return nil
        }
        var alteredSource = source
        alteredSource = alteredSource.replacingOccurrences(of: "$<handler>", with: "FingerprintingProtection\(messageHandlerTokenString)", options: .literal)
        return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false)
    }()
    
    private let cookieControlUserScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "CookieControl", ofType: "js"), let source: String = try? String(contentsOfFile: path) else {
            log.error("Failed to load cookie control user script")
            return nil
        }
        
        return WKUserScript(source: source, injectionTime: .atDocumentStart, forMainFrameOnly: false)
    }()
    
    // PaymentRequestUserScript is injected at document start to handle
    // requests to payment APIs
    private let PaymentRequestUserScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "PaymentRequest", ofType: "js"), let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load PaymentRequest.js")
            return nil
        }
        
        var alteredSource = source
        
        alteredSource = alteredSource.replacingOccurrences(of: "$<paymentreq>", with: "PaymentRequest\(securityTokenString)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<paymentresponse>", with: "PaymentResponse\(securityTokenString)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<paymentresponsedetails>", with: "PaymentResponseDetails\(securityTokenString)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<paymentreqcallback>", with: "PaymentRequestCallback\(securityTokenString)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<handler>", with: "PaymentRequest\(messageHandlerTokenString)", options: .literal)
        
        return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false)
    }()
    
    private let resourceDownloadManagerUserScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "ResourceDownloader", ofType: "js"), let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load ResourceDownloader.js")
            return nil
        }
        var alteredSource: String = source

        alteredSource = alteredSource.replacingOccurrences(of: "$<downloadManager>", with: "D\(securityTokenString)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<handler>", with: "ResourceDownloadManager\(messageHandlerTokenString)", options: .literal)
        
        return WKUserScript(source: alteredSource, injectionTime: .atDocumentEnd, forMainFrameOnly: false)
    }()
    
    private let WindowRenderHelperScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "WindowRenderHelper", ofType: "js"), let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load WindowRenderHelper.js")
            return nil
        }
        
        // Verify that the application itself is making a call to the JS script instead of other scripts on the page.
        // This variable will be unique amongst scripts loaded in the page.
        // When the script is called, the token is provided in order to access teh script variable.
        var alteredSource = source
        
        alteredSource = alteredSource.replacingOccurrences(of: "$<windowRenderer>", with: "W\(securityTokenString)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<handler>", with: "WindowRenderHelper\(messageHandlerTokenString)", options: .literal)
        
        return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false)
    }()
    
    private let FullscreenHelperScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "FullscreenHelper", ofType: "js"), let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load FullscreenHelper.js")
            return nil
        }
        return WKUserScript(source: source, injectionTime: .atDocumentStart, forMainFrameOnly: false)
    }()
    
    private let PlaylistSwizzlerScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "PlaylistSwizzler", ofType: "js"),
              let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load PlaylistSwizzler.js")
            return nil
        }
        return WKUserScript(source: source, injectionTime: .atDocumentStart, forMainFrameOnly: false)
    }()
    
    private let PlaylistHelperScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "Playlist", ofType: "js"), let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load Playlist.js")
            return nil
        }
        
        var alteredSource = source
        let token = UserScriptManager.securityTokenString
        
        let replacements = [
            "$<Playlist>": "Playlist_\(token)",
            "$<security_token>": "\(token)",
            "$<tagNode>": "tagNode_\(token)",
            "$<tagUUID>": "tagUUID_\(token)",
            "$<mediaCurrentTimeFromTag>": "mediaCurrentTimeFromTag_\(token)",
            "$<stopMediaPlayback>": "stopMediaPlayback_\(token)",
            "$<sendMessage>": "playlistHelper_sendMessage_\(token)",
            "$<handler>": "playlistHelper_\(messageHandlerTokenString)",
            "$<notify>": "notify_\(token)",
            "$<onLongPressActivated>": "onLongPressActivated_\(token)",
            "$<setupLongPress>": "setupLongPress_\(token)",
            "$<setupDetector>": "setupDetector_\(token)",
            "$<setupTagNode>": "setupTagNode_\(token)",
            "$<notifyNodeSource>": "notifyNodeSource_\(token)",
            "$<notifyNode>": "notifyNode_\(token)",
            "$<observeNode>": "observeNode_\(token)",
            "$<observeDocument>": "observeDocument_\(token)",
            "$<observeDynamicElements>": "observeDynamicElements_\(token)",
            "$<getAllVideoElements>": "getAllVideoElements_\(token)",
            "$<getAllAudioElements>": "getAllAudioElements_\(token)",
            "$<onReady>": "onReady_\(token)",
            "$<requestWhenIdleShim>": "requestWhenIdleShim_\(token)",
            "$<observePage>": "observePage_\(token)",
        ]
        
        replacements.forEach({
            alteredSource = alteredSource.replacingOccurrences(of: $0.key, with: $0.value, options: .literal)
        })
        
        return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false)
    }()
    
    private let MediaBackgroundingScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "MediaBackgrounding", ofType: "js"), let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load MediaBackgrounding.js")
            return nil
        }
        
        var alteredSource = source
        let token = UserScriptManager.securityTokenString
        
        let replacements = [
            "$<MediaBackgrounding>": "MediaBackgrounding_\(token)",
            "$<handler>": "mediaBackgrounding_\(messageHandlerTokenString)",
        ]
        
        replacements.forEach({
            alteredSource = alteredSource.replacingOccurrences(of: $0.key, with: $0.value, options: .literal)
        })
        
        return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false)
    }()

    private func reloadUserScripts() {
        tab?.webView?.configuration.userContentController.do {
            $0.removeAllUserScripts()
            self.packedUserScripts.forEach($0.addUserScript)
            
            if isFingerprintingProtectionEnabled, let script = fingerprintingProtectionUserScript {
                $0.addUserScript(script)
            }
            if isCookieBlockingEnabled, let script = cookieControlUserScript {
                $0.addUserScript(script)
            }
            
            if let script = resourceDownloadManagerUserScript {
                $0.addUserScript(script)
            }
            
            if let script = WindowRenderHelperScript {
                $0.addUserScript(script)
            }
            
            if let script = FullscreenHelperScript {
                $0.addUserScript(script)
            }
            
            if UIDevice.isIpad, isWebCompatibilityMediaSourceAPIEnabled, let script = PlaylistSwizzlerScript {
                $0.addUserScript(script)
            }
            
            if isPlaylistEnabled, let script = PlaylistHelperScript {
                $0.addUserScript(script)
            }
            
            if isMediaBackgroundPlaybackEnabled, let script = MediaBackgroundingScript {
                $0.addUserScript(script)
            }
            
            if let domainUserScript = domainUserScript, let script = domainUserScript.script {
                $0.addUserScript(script)
            }
        }
    }
}
