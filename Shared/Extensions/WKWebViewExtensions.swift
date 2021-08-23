// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import JavaScriptCore
import WebKit

enum JavascriptError: Error {
    case invalid
}

extension WKUserScript {
    public class func createInDefaultContentWorld(source: String, injectionTime: WKUserScriptInjectionTime, forMainFrameOnly: Bool) -> WKUserScript {
        if #available(iOS 14.0, *) {
            return WKUserScript(source: source, injectionTime: injectionTime, forMainFrameOnly: forMainFrameOnly, in: .defaultClient)
        } else {
            return WKUserScript(source: source, injectionTime: injectionTime, forMainFrameOnly: forMainFrameOnly)
        }
    }
}

public extension WKWebView {
    func generateJavascriptFunctionString(functionName: String, args: [Any], escapeArgs: Bool = true) -> (javascript: String, error: Error?) {
        let context = JSContext()

        var sanitizedArgs: [String] = []
        var error: Error?

        args.forEach {
            if !escapeArgs {
                sanitizedArgs.append("\($0)")
                return
            }
            context?.exceptionHandler = { context, exception in
                if exception != nil {
                    error = JavascriptError.invalid
                }
            }
            context?.evaluateScript("JSON.parse('\"\($0)\"')")
            sanitizedArgs.append("'\(String(describing: $0).htmlEntityEncodedString)'")
            return
        }
        
        return ("\(functionName)(\(sanitizedArgs.joined(separator: ", ")))", error)
    }

    func evaluateSafeJavaScript(functionName: String, args: [Any] = [], sandboxed: Bool = true, escapeArgs: Bool = true, asFunction: Bool = true, completion: ((Any?, Error?) -> Void)? = nil) {
        var javascript = functionName
        
        if asFunction {
            let js = generateJavascriptFunctionString(functionName: functionName, args: args, escapeArgs: escapeArgs)
            if js.error != nil {
                if let completionHandler = completion {
                    completionHandler(nil, js.error)
                }
                return
            }
            javascript = js.javascript
        }
        if #available(iOS 14.0, *), sandboxed {
            // swiftlint:disable:next safe_javascript
            evaluateJavaScript(javascript, in: nil, in: .defaultClient) { result  in
                switch result {
                    case .success(let value):
                        completion?(value, nil)
                    case .failure(let error):
                        completion?(nil, error)
                }
            }
        } else {
            // swiftlint:disable:next safe_javascript
            evaluateJavaScript(javascript) { data, error  in
                completion?(data, error)
            }
        }
    }
}
