/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import WebKit
import Shared

private let log = Logger.browserLogger

class UserScriptManager {

    // Scripts can use this to verify the app –not js on the page– is calling into them.
    public static let securityToken = UUID()

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
    
    // Whether or not the U2F APIs should be exposed
    var isU2FEnabled: Bool {
        didSet {
            if oldValue == isU2FEnabled { return }
            reloadUserScripts()
        }
    }
    
    init(tab: Tab, isFingerprintingProtectionEnabled: Bool, isCookieBlockingEnabled: Bool, isU2FEnabled: Bool) {
        self.tab = tab
        self.isFingerprintingProtectionEnabled = isFingerprintingProtectionEnabled
        self.isCookieBlockingEnabled = isCookieBlockingEnabled
        self.isU2FEnabled = isU2FEnabled
        reloadUserScripts()
    }
    
    // MARK: -
    
    private let packedUserScripts: [WKUserScript] = {
        [(WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false),
         (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false),
         (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: true),
         (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: true)].compactMap { arg in
            let (injectionTime, mainFrameOnly) = arg
            let name = (mainFrameOnly ? "MainFrame" : "AllFrames") + "AtDocument" + (injectionTime == .atDocumentStart ? "Start" : "End")
            if let path = Bundle.main.path(forResource: name, ofType: "js"),
                let source = try? NSString(contentsOfFile: path, encoding: String.Encoding.utf8.rawValue) as String {
                let wrappedSource = "(function() { const SECURITY_TOKEN = '\(UserScriptManager.securityToken)'; \(source) })()"
                return WKUserScript(source: wrappedSource, injectionTime: injectionTime, forMainFrameOnly: mainFrameOnly)
            }
            return nil
        }
    }()
    
    private let fingerprintingProtectionUserScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "FingerprintingProtection", ofType: "js"), let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load fingerprinting protection user script")
            return nil
        }
        return WKUserScript(source: source, injectionTime: .atDocumentStart, forMainFrameOnly: false)
    }()
    
    private let cookieControlUserScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "CookieControl", ofType: "js"), let source: String = try? String(contentsOfFile: path) else {
            log.error("Failed to load cookie control user script")
            return nil
        }
        var alteredSource: String = source
        let token = UserScriptManager.securityToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<local>", with: "L\(token)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<session>", with: "S\(token)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<cookie>", with: "C\(token)", options: .literal)
        
        return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false)
    }()
    
    // U2FUserScript is injected at document start to avoid overriding the low-level
    // FIDO legacy sign and register APIs that have different arguments
    private let U2FUserScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "U2F", ofType: "js"), let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load U2F.js")
            return nil
        }
        
        var alteredSource = source
        let token = UserScriptManager.securityToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<webauthn>", with: "W\(token)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<u2f>", with: "U\(token)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<pkc>", with: "pkp\(token)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<assert>", with: "assert\(token)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<attest>", with: "attest\(token)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<u2fregister>", with: "u2fregister\(token)", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<u2fsign>", with: "u2fsign\(token)", options: .literal)
        
        return WKUserScript(source: alteredSource, injectionTime: .atDocumentStart, forMainFrameOnly: false)
    }()
    
    // U2FLowLevelUserScript is injected at documentEnd to override the message channels
    // with hooks that plug into the Yubico API
    private let U2FLowLevelUserScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "U2F-low-level", ofType: "js"), let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load U2F-low-level.js")
            return nil
        }
        var alteredSource = source
        let token = UserScriptManager.securityToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<u2f>", with: "U\(token)", options: .literal)

        return WKUserScript(source: alteredSource, injectionTime: .atDocumentEnd, forMainFrameOnly: false)
    }()
    
    private let resourceDownloadManagerUserScript: WKUserScript? = {
        guard let path = Bundle.main.path(forResource: "ResourceDownloader", ofType: "js"), let source = try? String(contentsOfFile: path) else {
            log.error("Failed to load ResourceDownloader.js")
            return nil
        }
        var alteredSource: String = source
        
        //Verify that the application itself is making a call to the JS script instead of other scripts on the page.
        //This variable will be unique amongst scripts loaded in the page.
        //When the script is called, the token is provided in order to access teh script variable.
        let token = UserScriptManager.securityToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)
        alteredSource = alteredSource.replacingOccurrences(of: "$<downloadManager>", with: "D\(token)", options: .literal)
        
        return WKUserScript(source: alteredSource, injectionTime: .atDocumentEnd, forMainFrameOnly: false)
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
            
            if isU2FEnabled, let script = U2FUserScript {
                $0.addUserScript(script)
            }

            if isU2FEnabled, let script = U2FLowLevelUserScript {
                $0.addUserScript(script)
            }
            
            if let script = resourceDownloadManagerUserScript {
                $0.addUserScript(script)
            }
        }
    }
}
