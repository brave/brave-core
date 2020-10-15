// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit

class BraveWebView: WKWebView {
    
    /// Stores last position when the webview was touched on.
    private(set) var lastHitPoint = CGPoint(x: 0, y: 0)
    
    private static var nonPersistentDataStore: WKWebsiteDataStore?
    static func sharedNonPersistentStore() -> WKWebsiteDataStore {
        if let dataStore = nonPersistentDataStore {
            return dataStore
        }
        
        let dataStore = WKWebsiteDataStore.nonPersistent()
        nonPersistentDataStore = dataStore
        return dataStore
    }
    
    init(frame: CGRect, configuration: WKWebViewConfiguration = WKWebViewConfiguration(), isPrivate: Bool = true) {
        if isPrivate {
            configuration.websiteDataStore = BraveWebView.sharedNonPersistentStore()
        } else {
            configuration.websiteDataStore = WKWebsiteDataStore.default()
        }
        
        super.init(frame: frame, configuration: configuration)
        
        customUserAgent = UserAgent.userAgentForDesktopMode
    }
    
    static func removeNonPersistentStore() {
        BraveWebView.nonPersistentDataStore = nil
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
    
    override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
        lastHitPoint = point
        return super.hitTest(point, with: event)
    }

    private func generateJavascriptFunctionString(functionName: String, args: [Any]) -> String {
        let argsJS = args
          .map { "'\(String(describing: $0).escapeHTML())'" }
          .joined(separator: ", ")
        return "\(functionName)(\(argsJS))"
    }

    func evaluateSafeJavascript(functionName: String, args: [Any], completion: @escaping ((Any?, Error?) -> Void)) {
        let javascript = generateJavascriptFunctionString(functionName: functionName, args: args)
        evaluateJavaScript(javascript) { data, error  in
            completion(data, error)
        }
    }
}

extension String {
    /// Encode HTMLStrings
    func escapeHTML() -> String {
       return self
        .replacingOccurrences(of: "&", with: "&amp;", options: .literal)
        .replacingOccurrences(of: "\"", with: "&quot;", options: .literal)
        .replacingOccurrences(of: "'", with: "&#39;", options: .literal)
        .replacingOccurrences(of: "<", with: "&lt;", options: .literal)
        .replacingOccurrences(of: ">", with: "&gt;", options: .literal)
        .replacingOccurrences(of: "`", with: "&lsquo;", options: .literal)
    }
}
