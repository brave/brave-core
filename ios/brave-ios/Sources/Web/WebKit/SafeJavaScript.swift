// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import WebKit

extension WKWebView {
  enum JavaScriptError: Error {
    case invalid
  }
  func generateJSFunctionString(
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
              throw JavaScriptError.invalid
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
      return ("", JavaScriptError.invalid)
    }

    return ("\(functionName)(\(sanitizedArgs.joined(separator: ", ")))", nil)
  }

  @discardableResult @MainActor
  public func evaluateJavaScript(
    functionName: String,
    args: [Any] = [],
    frame: WKFrameInfo? = nil,
    contentWorld: WKContentWorld,
    escapeArgs: Bool = true,
    asFunction: Bool = true
  ) async throws -> Any? {
    var javascript = functionName

    if asFunction {
      let js = generateJSFunctionString(
        functionName: functionName,
        args: args,
        escapeArgs: escapeArgs
      )
      if let error = js.error {
        throw error
      }
      javascript = js.javascript
    }
    return try await withCheckedThrowingContinuation { continuation in
      // We _must_ use the non-async method here due to a bug in WebKit where if you attempt to
      // discard the error using `try?` it will crash (an EXC_BREAKPOINT without any exception)
      self.evaluateJavaScript(javascript, in: frame, in: contentWorld) { result in
        continuation.resume(with: result)
      }
    }
  }
}
