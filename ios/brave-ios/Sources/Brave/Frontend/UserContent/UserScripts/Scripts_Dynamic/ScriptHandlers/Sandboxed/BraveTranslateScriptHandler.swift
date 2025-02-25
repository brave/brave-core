// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Preferences
import Shared
import WebKit
import os.log

protocol BraveTranslateScriptHandlerDelegate: NSObject {
  func updateTranslateURLBar(tab: Tab?, state: TranslateURLBarButton.TranslateState)
  func canShowTranslateOnboarding(tab: Tab?) -> Bool
  func showTranslateOnboarding(tab: Tab?, completion: @escaping (_ translateEnabled: Bool?) -> Void)
  func presentToast(tab: Tab?, languageInfo: BraveTranslateLanguageInfo)
}

class BraveTranslateScriptHandler: NSObject, TabContentScript {
  private static var elementScriptTask: Task<String, Error> = downloadElementScript()

  static let namespace = "translate_\(uniqueID)"
  static let scriptName = "BraveTranslateScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "TranslateMessage"
  static let scriptSandbox = WKContentWorld.world(name: "BraveTranslateContentWorld")
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }

    return WKUserScript(
      source:
        script
        .replacingOccurrences(of: "$<brave_translate_script>", with: namespace)
        .replacingOccurrences(
          of: "$<brave_translate_api_key>",
          with: kBraveServicesKey
        )
        .replacingOccurrences(of: "$<message_handler>", with: messageHandlerName)
        .replacingOccurrences(
          of: "$<brave_core_translate_translate_js>",
          with: TranslateScript.script ?? ""
        ),
      injectionTime: .atDocumentEnd,
      forMainFrameOnly: true,
      in: scriptSandbox
    )
  }()

  static func checkTranslate(tab: Tab) {
    tab.webView?.evaluateSafeJavaScript(
      functionName:
        """
        try {
          window.__firefox__.\(namespace).checkTranslate();
        } catch(error) {
          // Page & Script not loaded yet
        }
        """,
      contentWorld: BraveTranslateScriptHandler.scriptSandbox,
      asFunction: false
    )
  }

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    // Setup

    let isReaderMode = tab.url?.isInternalURL(for: .readermode) == true
    if tab.lastKnownSecureContentState != .secure && !isReaderMode {
      Logger.module.debug("Translation Disabled - Insecure Page")
      replyHandler(nil, BraveTranslateError.translateDisabled.rawValue)
      return
    }

    if Preferences.Translate.translateEnabled.value == false {
      Logger.module.debug("Translation Disabled")
      replyHandler(nil, BraveTranslateError.translateDisabled.rawValue)
      return
    }

    guard let body = message.body as? [String: Any],
      let command = body["command"] as? String
    else {
      Logger.module.error("Invalid Brave Translate Message")
      return
    }

    // Processing
    Task {
      let (result, error) = try await processScriptMessage(for: tab, command: command, body: body)
      replyHandler(result, error)
    }
  }

  private func processScriptMessage(
    for tab: Tab?,
    command: String,
    body: [String: Any]
  ) async throws -> (Any?, String?) {
    if command == "load_brave_translate_script" {
      if Preferences.Translate.translateEnabled.value == true {
        let script = try await BraveTranslateScriptHandler.elementScriptTask.value
        return (script, nil)
      }
      return (nil, BraveTranslateError.translateDisabled.rawValue)
    }

    if command == "ready" {
      try await tab?.translateHelper?.setupTranslate()
      return (nil, nil)
    }

    if command == "request" {
      do {
        let message = try JSONDecoder().decode(
          BraveTranslateSession.RequestMessage.self,
          from: JSONSerialization.data(withJSONObject: body, options: .fragmentsAllowed)
        )

        guard let tab = tab, let translateHelper = tab.translateHelper
        else {
          return (nil, BraveTranslateError.otherError.rawValue)
        }

        let (data, response) = try await translateHelper.processTranslationRequest(message)

        return (
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
      } catch let error as BraveTranslateError {
        Logger.module.error("Brave Translate Error: \(error)")
        return (nil, "Translation Error: \(error.rawValue)")
      } catch {
        Logger.module.error("Brave Translate Error: \(error)")
        return (nil, "Translation Error")
      }
    }

    if command == "status" {
      Logger.module.debug("[Brave Translate] - Status: \(body["errorCode"] as? Int ?? 0)")
    }

    return (nil, nil)
  }

  // MARK: - Private

  private static func downloadElementScript() -> Task<String, Error> {
    return Task {
      var urlRequest = URLRequest(
        url: URL(string: "https://translate.brave.com/static/v1/element.js")!
      )
      urlRequest.httpMethod = "GET"

      let session = URLSession(configuration: .ephemeral)
      defer { session.finishTasksAndInvalidate() }
      let (data, response) = try await session.data(for: urlRequest)

      guard let response = response as? HTTPURLResponse, response.statusCode == 200 else {
        throw BraveTranslateError.invalidTranslationResponse
      }

      guard let script = String(data: data, encoding: .utf8) else {
        throw BraveTranslateError.invalidTranslationResponse
      }

      return script
    }
  }
}

class BraveTranslateScriptLanguageDetectionHandler: NSObject, TabContentScript {
  private static let namespace = "translate_\(uniqueID)"

  static let scriptName = "BraveTranslateLanguageDetectionScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "LanguageDetectionTextCaptured"
  static let scriptSandbox = WKContentWorld.world(name: "BraveTranslateContentWorld")
  static let userScript: WKUserScript? = nil

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    // In the future we'd get this from: components/language/ios/browser/language_detection_java_script_feature.mm

    guard let body = message.body as? [String: Any] else {
      Logger.module.error("Invalid Brave Translate Language Detection Message")
      return
    }

    guard
      let translateHelper = tab.translateHelper
    else {
      return
    }

    do {
      let message = try JSONDecoder().decode(
        Message.self,
        from: JSONSerialization.data(withJSONObject: body, options: .fragmentsAllowed)
      )

      if message.hasNoTranslate {
        translateHelper.currentLanguageInfo.pageLanguage =
          translateHelper.currentLanguageInfo.currentLanguage
      } else {
        translateHelper.currentLanguageInfo.pageLanguage =
          !message.htmlLang.isEmpty ? Locale.Language(identifier: message.htmlLang) : nil
      }

      replyHandler(nil, nil)
    } catch {
      Logger.module.error("Brave Translate Language Detection Error: \(error)")
      replyHandler(nil, "Translation Language Detection Error")
    }
  }

  private struct Message: Codable {
    let frameId: String
    let hasNoTranslate: Bool
    let htmlLang: String
    let httpContentLanguage: String
  }
}
