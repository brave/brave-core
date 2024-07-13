// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import NaturalLanguage
import Shared
import Translation
import WebKit
import os.log

enum BraveTranslateError: Error {
  case invalidURL
  case invalidLanguage
  case sameLanguage
  case invalidPageSource
  case invalidTranslationResponseCode
  case invalidTranslationResponse
  case otherError
}

class BraveTranslateScriptHandler: NSObject, TabContentScript {
  private weak var tab: Tab?
  private let recognizer = NLLanguageRecognizer()
  private static let namespace = "translate_\(uniqueID)"

  private struct RequestMessage: Codable {
    let method: String
    let url: URL
    let headers: [String: String]
    let body: String
  }

  init(tab: Tab) {
    self.tab = tab
    super.init()
  }

  static let scriptName = "BraveTranslateScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "TranslateMessage"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }

    // HACKS! Need a better way to do this.
    // Chromium Scripts do NOT have a secure message handler and cannot be sandboxed the same way!
    return WKUserScript(
      source:
        script
        .replacingOccurrences(of: "$<brave_translate_script>", with: namespace)
        .replacingOccurrences(
          of: "$<brave_translate_api_key>",
          with: ""
        )
        .replacingOccurrences(of: "$<message_handler>", with: messageHandlerName),
      injectionTime: .atDocumentEnd,
      forMainFrameOnly: true,
      in: scriptSandbox
    )
  }()

  @MainActor
  private func executePageFunction(name functionName: String) async -> String? {
    guard let webView = tab?.webView else { return nil }

    let (result, error) = await webView.evaluateSafeJavaScript(
      functionName: "window.__firefox__.\(BraveTranslateScriptHandler.namespace).\(functionName)",
      contentWorld: BraveTranslateScriptHandler.scriptSandbox,
      asFunction: true
    )

    if let error = error {
      Logger.module.error("Unable to execute page function \(functionName) error: \(error)")
      return nil
    }

    guard let result = result as? String else {
      Logger.module.error("Invalid Page Result")
      return nil
    }

    return result
  }

  @MainActor
  @discardableResult
  func executeChromiumFunction(_ functionName: String, args: [Any] = []) async -> Any? {
    guard let webView = tab?.webView else { return nil }

    let (result, error) = await webView.evaluateSafeJavaScript(
      functionName: "window.__gCrWeb.\(functionName)",
      args: args,
      contentWorld: BraveTranslateScriptHandler.scriptSandbox,
      asFunction: true
    )

    if let error = error {
      Logger.module.error("Unable to execute page function \(functionName) error: \(error)")
      return nil
    }

    return result
  }

  private func getPageSource() async -> String? {
    return await executePageFunction(name: "getPageSource")
  }

  @MainActor
  func guessLanguage() async -> Locale.Language? {
    await executeChromiumFunction("languageDetection.detectLanguage")

    if let languageCode = await executePageFunction(name: "getPageLanguage") {
      return Locale.Language(identifier: languageCode)
    }

    if let rawPageSource = await executePageFunction(name: "getRawPageSource") {
      recognizer.reset()
      recognizer.processString(rawPageSource)

      if let dominantLanguage = recognizer.dominantLanguage, dominantLanguage != .undetermined {
        return Locale.Language(identifier: dominantLanguage.rawValue)
      }
    }

    return Locale.current.language
  }

  func getLanguageInfo() async throws -> (
    currentLanguage: Locale.Language, pageLanguage: Locale.Language
  ) {
    let pageLanguage = await guessLanguage() ?? Locale.current.language
    return (Locale.current.language, pageLanguage)
  }

  func activateScript() {
    Task { @MainActor in
      let languageInfo = try await getLanguageInfo()
      guard let currentLanguage = languageInfo.currentLanguage.languageCode?.identifier,
        let pageLanguage = languageInfo.pageLanguage.languageCode?.identifier,
        currentLanguage != pageLanguage
      else {
        return
      }

      await executeChromiumFunction(
        "translate.startTranslation",
        args: [pageLanguage, currentLanguage]
      )
    }
  }

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceiveScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    guard let body = message.body as? [String: Any] else {
      Logger.module.error("INVALID BRAVE TRANSLATE MESSAGE")
      return
    }

    if body["command"] as? String == "request" {
      Task {
        do {
          let message = try JSONDecoder().decode(
            RequestMessage.self,
            from: JSONSerialization.data(withJSONObject: body, options: .fragmentsAllowed)
          )

          var request = URLRequest(url: message.url)
          request.httpMethod = message.method
          request.httpBody = message.body.data(using: .utf8)
          message.headers.forEach { (key, value) in
            request.setValue(value, forHTTPHeaderField: key)
          }

          let session = URLSession(configuration: .ephemeral)
          defer { session.finishTasksAndInvalidate() }
          let (data, response) = try await session.data(for: request)

          guard let response = response as? HTTPURLResponse else {
            return
          }

          replyHandler(
            [
              "value": [
                "statusCode": response.statusCode,
                "responseType": "",
                "response": String(data: data, encoding: .utf8) ?? "",
                "headers": response.allHeaderFields.map({ "\($0): \($1)" }).joined(
                  separator: "\r\n"
                ),
              ]
            ],
            nil
          )
        } catch {
          print("ERROR: \(error)")
        }
      }

      return
    }

    if body["command"] as? String == "ready" {
      print("TRANSLATE IS READY!")

      Task { @MainActor in
        activateScript()
      }
    }

    replyHandler(nil, nil)
  }
}
