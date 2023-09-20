// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import NaturalLanguage
import Shared
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
  private var fromLang: String = "en"
  private var toLang: String = "en"

  init(tab: Tab) {
    self.tab = tab
    super.init()
  }

  static let scriptName = "BraveTranslateScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }

    return WKUserScript(
      source: secureScript(
        handlerNamesMap: [
          "$<message_handler>": messageHandlerName,
          "$<brave_translate_script>": namespace,
        ],
        securityToken: scriptId,
        script: script
      ),
      injectionTime: .atDocumentEnd,
      forMainFrameOnly: true,
      in: scriptSandbox
    )
  }()

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

  private func getPageSource() async -> String? {
    return await executePageFunction(name: "getPageSource")
  }

  @MainActor
  func guessLanguage() async -> String? {
    if let rawPageSource = await executePageFunction(name: "getRawPageSource") {
      recognizer.reset()
      recognizer.processString(rawPageSource)

      if let dominantLanguage = recognizer.dominantLanguage, dominantLanguage != .undetermined {
        return dominantLanguage.rawValue
      }
    }

    if let languageCode = await executePageFunction(name: "getPageLanguage") {
      return languageCode
    }

    return Locale.current.languageCode
  }

  func getLanguageInfo() async throws -> (String, String) {
    var currentLanguageCode = ""

    if #available(iOS 16, *) {
      guard let currentLanguage = Locale.current.language.languageCode?.identifier else {
        throw BraveTranslateError.invalidLanguage
      }

      currentLanguageCode = currentLanguage
    } else {
      guard let currentLanguage = Locale.current.languageCode else {
        throw BraveTranslateError.invalidLanguage
      }

      currentLanguageCode = currentLanguage
    }

    let pageLanguage = await guessLanguage() ?? currentLanguageCode
    return (currentLanguageCode, pageLanguage)
  }

  func translatePage(
    pageSource: String,
    from fromLanguage: String?,
    to toLanguage: String?
  ) async throws -> Any {
    guard var url = URL(string: "https://translate.bsg.brave.software/translate_a/t") as? NSURL
    else {
      throw BraveTranslateError.invalidURL
    }

    var currentLanguageCode = ""

    if let toLanguage = toLanguage {
      currentLanguageCode = toLanguage
    } else {
      if #available(iOS 16, *) {
        guard let currentLanguage = Locale.current.language.languageCode?.identifier else {
          throw BraveTranslateError.invalidLanguage
        }

        currentLanguageCode = currentLanguage
      } else {
        guard let currentLanguage = Locale.current.languageCode else {
          throw BraveTranslateError.invalidLanguage
        }

        currentLanguageCode = currentLanguage
      }
    }

    var pageLanguage = currentLanguageCode
    if let fromLanguage = fromLanguage {
      pageLanguage = fromLanguage
    } else {
      pageLanguage = await guessLanguage() ?? currentLanguageCode
    }

    // Cannot translate from source to current as they're the same
    if pageLanguage.isEmpty || currentLanguageCode.isEmpty || pageLanguage == currentLanguageCode {
      throw BraveTranslateError.sameLanguage
    }

    url = url.addingQueryParameter(key: "tl", value: currentLanguageCode) as NSURL
    url = url.addingQueryParameter(key: "sl", value: pageLanguage) as NSURL

    let authenticator = BasicAuthCredentialsManager()
    let session = URLSession(
      configuration: .ephemeral,
      delegate: authenticator,
      delegateQueue: .main
    )
    defer { session.finishTasksAndInvalidate() }

    func executeRequest(
      url: URL,
      method: String,
      headers: [String: String],
      body: String
    ) async throws -> (Data, HTTPURLResponse) {
      /*let formEncode = { (fields: [(String, String)]) -> String in
        let escape = { (str: String) -> String in
          var charset = CharacterSet.alphanumerics
          charset.insert(" ")
          charset.remove("+")
          charset.remove("/")
          charset.remove("?")
          charset.remove(";")

          return str.replacingOccurrences(of: "\n", with: "\r\n")
            .addingPercentEncoding(withAllowedCharacters: charset)!
            .replacingOccurrences(of: " ", with: "+")
        }

        return fields.map { (key, value) in
            return escape(key) + "=" + escape(value)
        }.joined(separator: "&")
      }*/

      var request = URLRequest(url: url)
      request.httpMethod = method
      headers.forEach({ request.setValue($0.value, forHTTPHeaderField: $0.key) })
      request.httpBody = "q=\(body)".data(using: .utf8)

      let (data, response) = try await NetworkManager(session: session).dataRequest(with: request)
      guard let response = response as? HTTPURLResponse,
        response.statusCode == 304 || response.statusCode >= 200 || response.statusCode <= 299
      else {
        throw BraveTranslateError.invalidTranslationResponseCode
      }

      return (data, response)
    }

    let (data, _) = try await executeRequest(
      url: url as URL,
      method: "POST",
      headers: ["Content-Type": "application/x-www-form-urlencoded"],
      body: pageSource
    )
    return try JSONSerialization.jsonObject(with: data, options: [.fragmentsAllowed])
  }

  func activateScript(from: String, to: String) {
    guard let webView = tab?.webView else {
      return
    }

    self.fromLang = from
    self.toLang = to

    Task { @MainActor in
      await webView.evaluateSafeJavaScript(
        functionName: "window.__firefox__.\(BraveTranslateScriptHandler.namespace).start",
        contentWorld: BraveTranslateScriptHandler.scriptSandbox,
        asFunction: true
      )
    }
  }

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceiveScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    guard let message = message.body as? [String: String] else {
      Logger.module.error("Invalid Brave Translate Message")
      replyHandler(nil, nil)
      return
    }

    guard let text = message["text"],
      let encodedText = message["encoded"]
    else {
      Logger.module.error("Invalid Brave Translate Message")
      replyHandler(nil, nil)
      return
    }

    Task { @MainActor in
      do {
        if text == ":" || text == "-" || text == ", " || text == "," {
          replyHandler(nil, nil)
          return
        }

        let result = try await translatePage(pageSource: encodedText, from: fromLang, to: toLang)
        if let result = result as? [String] {
          let prefix = text.prefix(while: {
            $0.unicodeScalars.allSatisfy(CharacterSet.whitespacesAndNewlines.contains)
          })
          let suffix = text.suffix(while: {
            $0.unicodeScalars.allSatisfy(CharacterSet.whitespacesAndNewlines.contains)
          })
          replyHandler("\(prefix)\(result[0])\(suffix)", nil)
        } else {
          replyHandler(nil, nil)
        }
      } catch {
        replyHandler(nil, nil)
      }
    }
  }
}
