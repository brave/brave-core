/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import WebKit
import Shared
import SwiftyJSON

public class SyncCrypto: JSInjector {

    public static let shared = SyncCrypto()
    
    fileprivate let syncCryptoWebView = WKWebView(frame: CGRect.zero, configuration: SyncCrypto.webConfig)
    /// Whehter or not sync crypto is ready to be used
    fileprivate var isSyncCryptoReady = false
    
    override init() {
        super.init()
        
        self.maximumDelayAttempts = 4
        
        // Overriding javascript check for this subclass
        self.isJavascriptReadyCheck = { return self.isSyncCryptoReady }
        
        // Load HTML and await for response, to verify the webpage is loaded to receive brave-crypto commands
        self.syncCryptoWebView.navigationDelegate = self
        // Must load HTML for delegate method to fire
        self.syncCryptoWebView.loadHTMLString("<body>TEST</body>", baseURL: nil)
    }
    
    fileprivate class var webConfig: WKWebViewConfiguration {
        let webCfg = WKWebViewConfiguration()
        webCfg.userContentController = WKUserContentController()
        
        if let script = ScriptOpener.get(withName: "crypto") {
            webCfg.userContentController.addUserScript(WKUserScript(source: script, injectionTime: .atDocumentEnd, forMainFrameOnly: true))
        }
        
        return webCfg
    }
    
    class func uniqueSerialBytes(count byteCount: Int) -> [Int]? {
        if byteCount % 2 != 0 { return nil }
        return (0..<byteCount).map { _ in Int(arc4random_uniform(256)) }
    }
    
    /// Used to retrive unique bytes for UUIDs (e.g. bookmarks), that will map well with niceware
    /// count: The number of unique bytes desired
    /// returns (via completion): Array of unique bytes
    func uniqueBytes(count byteCount: Int, completion: @escaping (([Int]?, Error?) -> Void)) {
        // TODO: Add byteCount validation (e.g. must be even)
        
        executeBlockOnReady {
            self.syncCryptoWebView.evaluateJavaScript("JSON.stringify(module.exports.passphrase.toBytes32(module.exports.getSeed(\(byteCount))))") { (result, error) in
                
                let bytes = JSON(parseJSON: result as? String ?? "")["data"].array?.map({ $0.intValue })
                completion(bytes, error)
            }
        }
    }
    
    /// Takes hex values and returns associated English words
    /// fromBytesOrHex: Array of hex strings (no "0x" prefix) : ["00", "ee", "4a", "42"]
    /// returns (via completion): String of words from brave-crypto that map to those hex values:
    /// "administrational experimental"
    // TODO: Massage data a bit more for completion block
    public func passphrase(fromBytes bytes: [Int], completion: @escaping (([String]?, Error?) -> Void)) {
        let bipWordLength = 24
        
        executeBlockOnReady {
            
            let input = "new Uint8Array(\(bytes))"
            let jsToExecute = "JSON.stringify(module.exports.passphrase.fromBytesOrHex(\(input)));"

            self.syncCryptoWebView.evaluateJavaScript(jsToExecute, completionHandler: {
                (result, error) in

                // Words come in format: \"word1 word2 word3... \"
                let words = (result as? String ?? "").replacingOccurrences(of: "\"", with: "").components(separatedBy: " ")
                if words.count != bipWordLength {
                    completion(nil, nil)
                    return
                }
                
                completion(words, error)
            })
        }
    }
    
    /// Takes joined string of unique hex bytes (e.g. from QR code) and returns
    func passphrase(fromJoinedBytes bytes: String, completion: @escaping (([String]?, Error?) -> Void)) {
        if let split = splitBytes(fromJoinedBytes: bytes) {
            return passphrase(fromBytes: split, completion: completion)
        }
        // TODO: Create real error
        completion(nil, nil)
    }
    
    /// Takes a joined string of unique hex bytes (e.g. from QR code) and splits up the hex values
    /// fromJoinedBytes: a single string of hex data (even # of chars required): 897a6f0219fd2950
    /// return: integer values split into 8 bit groupings [0x98, 0x7a, 0x6f, ...]
    public func splitBytes(fromJoinedBytes bytes: String) -> [Int]? {
        var chars = bytes.map { String($0) }
        
        if chars.count % 2 == 1 {
            // Must be an even array
            return nil
        }
        
        var result = [Int]()
        while !chars.isEmpty {
            let hex = chars[0...1].reduce("", +)
            guard let integer = Int(hex, radix: 16) else {
                // bad error
                return nil
            }
            result.append(integer)
            chars.removeFirst(2) // According to docs this returns removed result, behavior is different (at least for Swift 2.3)
        }
        return result.isEmpty ? nil : result
    }
    
    /// Takes a string description of an array and returns a string of hex used for Sync
    /// fromCombinedBytes: ([123, 119, 25, 14, 85, 125])
    /// returns: 7b77190e557d
    public func joinBytes(fromCombinedBytes bytes: [Int]?) -> String {
        guard let bytes = bytes else {
            return ""
        }
        
        let hex = bytes.map { String($0, radix: 16, uppercase: false) }
        
        // Sync hex must be 2 chars, with optional leading 0
        let fullHex = hex.map { $0.count == 2 ? $0 : "0" + $0 }
        let combinedHex = fullHex.joined(separator: "")
        return combinedHex
    }
    
    /// Takes English words and returns associated bytes.
    /// toBytes32: A string of words: "administrational experimental"
    /// returns (via completion): Array of integer values : [0x00, 0xee, 0x4a, 0x42]
    public func bytes(fromPassphrase passphrase: Array<String>, completion: (([Int]?, Error?) -> Void)?) {
        // TODO: Add some keyword validation

        let wordsWithSpaces = passphrase.joined(separator: " ")
        executeBlockOnReady {
            let jsToExecute = "JSON.stringify(module.exports.passphrase.toBytes32(\"\(wordsWithSpaces)\"));"
            
            self.syncCryptoWebView.evaluateJavaScript(jsToExecute, completionHandler: {
                (result, error) in

                // Input JSON dictionary is unordered, reordering it by keys.
                let bytes = JSON(parseJSON: result as? String ?? "").dictionary?.sorted(by: {
                    guard let firstIntKey = Int($0.key), let secondIntKey = Int($1.key) else { return false }
                    return firstIntKey < secondIntKey
                }).map({ $0.value.intValue })

                completion?(bytes, error)
            })
        }
    }
}

extension SyncCrypto: WKNavigationDelegate {
    public func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
        self.isSyncCryptoReady = true
    }
}
