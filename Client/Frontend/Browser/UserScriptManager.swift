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
    
    init(tab: Tab, isFingerprintingProtectionEnabled: Bool) {
        self.tab = tab
        self.isFingerprintingProtectionEnabled = isFingerprintingProtectionEnabled
        
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
    
    private func reloadUserScripts() {
        tab?.webView?.configuration.userContentController.do {
            $0.removeAllUserScripts()
            self.packedUserScripts.forEach($0.addUserScript)
            
            if isFingerprintingProtectionEnabled, let script = fingerprintingProtectionUserScript {
                $0.addUserScript(script)
            }
        }
    }
}
