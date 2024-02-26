// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import JavaScriptCore
import WebKit

enum JavascriptError: Error {
  case invalid
}

extension WKWebView {
  public func generateJSFunctionString(
    functionName: String,
    args: [Any?],
    escapeArgs: Bool = true
  ) -> (javascript: String, error: Error?) {
    var sanitizedArgs = [String]()
    for arg in args {
      if let arg = arg {
        do {
          if let arg = arg as? String {
            sanitizedArgs.append(escapeArgs ? "'\(arg.htmlEntityEncodedString)'" : "\(arg)")
          } else {
            let data = try JSONSerialization.data(withJSONObject: arg, options: [.fragmentsAllowed])

            if let str = String(data: data, encoding: .utf8) {
              sanitizedArgs.append(str)
            } else {
              throw JavascriptError.invalid
            }
          }
        } catch {
          return ("", error)
        }
      } else {
        sanitizedArgs.append("null")
      }
    }

    if args.count != sanitizedArgs.count {
      assertionFailure("Javascript parsing failed.")
      return ("", JavascriptError.invalid)
    }

    return ("\(functionName)(\(sanitizedArgs.joined(separator: ", ")))", nil)
  }

  public func evaluateSafeJavaScript(
    functionName: String,
    args: [Any] = [],
    frame: WKFrameInfo? = nil,
    contentWorld: WKContentWorld,
    escapeArgs: Bool = true,
    asFunction: Bool = true,
    completion: ((Any?, Error?) -> Void)? = nil
  ) {
    var javascript = functionName

    if asFunction {
      let js = generateJSFunctionString(
        functionName: functionName,
        args: args,
        escapeArgs: escapeArgs
      )
      if js.error != nil {
        if let completionHandler = completion {
          completionHandler(nil, js.error)
        }
        return
      }
      javascript = js.javascript
    }

    DispatchQueue.main.async {

      self.evaluateJavaScript(javascript, in: frame, in: contentWorld) { result in
        switch result {
        case .success(let value):
          completion?(value, nil)
        case .failure(let error):
          completion?(nil, error)
        }
      }
    }
  }

  @discardableResult @MainActor public func evaluateSafeJavaScript(
    functionName: String,
    args: [Any] = [],
    frame: WKFrameInfo? = nil,
    contentWorld: WKContentWorld,
    escapeArgs: Bool = true,
    asFunction: Bool = true
  ) async -> (Any?, Error?) {
    await withCheckedContinuation { continuation in
      evaluateSafeJavaScript(
        functionName: functionName,
        args: args,
        frame: frame,
        contentWorld: contentWorld,
        escapeArgs: escapeArgs,
        asFunction: asFunction
      ) { value, error in
        continuation.resume(returning: (value, error))
      }
    }
  }

  @discardableResult
  @MainActor public func evaluateSafeJavaScriptThrowing(
    functionName: String,
    args: [Any] = [],
    frame: WKFrameInfo? = nil,
    contentWorld: WKContentWorld,
    escapeArgs: Bool = true,
    asFunction: Bool = true
  ) async throws -> Any? {
    let result = await evaluateSafeJavaScript(
      functionName: functionName,
      args: args,
      frame: frame,
      contentWorld: contentWorld,
      escapeArgs: escapeArgs,
      asFunction: asFunction
    )

    if let error = result.1 {
      throw error
    } else {
      return result.0
    }
  }

  public var sampledPageTopColor: UIColor? {
    let selector = Selector("_sampl\("edPageTopC")olor")
    if responds(to: selector), let result = perform(selector) {
      return result.takeUnretainedValue() as? UIColor
    }
    return nil
  }
}

extension WKWebViewConfiguration {
  public func enablePageTopColorSampling() {
    let selector = Selector("_setSa\("mpledPageTopColorMaxDiff")erence:")
    if responds(to: selector) {
      perform(selector, with: 5.0 as Double)
    }
  }
}
