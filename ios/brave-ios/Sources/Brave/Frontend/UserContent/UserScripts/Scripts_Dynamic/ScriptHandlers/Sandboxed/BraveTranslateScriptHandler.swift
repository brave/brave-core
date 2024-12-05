// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import NaturalLanguage
import Preferences
import Shared
import WebKit
import os.log

protocol BraveTranslateScriptHandlerDelegate: NSObject {
  func updateTranslateURLBar(tab: Tab?, state: TranslateURLBarButton.TranslateState)
  func showTranslateOnboarding(tab: Tab?, completion: @escaping (_ translateEnabled: Bool) -> Void)
  func presentToast(tab: Tab?, languageInfo: BraveTranslateLanguageInfo)
}

class BraveTranslateScriptHandler: NSObject, TabContentScript {
  private weak var tab: Tab?
  private static var elementScriptTask: Task<String, Error> = downloadElementScript()

  init(tab: Tab) {
    self.tab = tab
    super.init()
  }

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

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceiveScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {

    // Setup

    guard let webView = message.webView else {
      Logger.module.error("Invalid WebView for Translation")
      replyHandler(nil, BraveTranslateError.otherError.rawValue)
      return
    }

    if !Preferences.Translate.translateEnabled.value {
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

    if command == "load_brave_translate_script" {
      Task {
        let script = try await BraveTranslateScriptHandler.elementScriptTask.value
        replyHandler(script, nil)
      }
      return
    }

    if command == "ready" {
      Task { @MainActor [weak tab] in
        try await
          (tab?.getTabHelper(named: BraveTranslateTabHelper.tabHelperName)
          as? BraveTranslateTabHelper)?.setupTranslate()
        replyHandler(nil, nil)
      }

      return
    }

    if command == "request" {
      Task { @MainActor [weak tab] in
        do {
          let message = try JSONDecoder().decode(
            BraveTranslateSession.RequestMessage.self,
            from: JSONSerialization.data(withJSONObject: body, options: .fragmentsAllowed)
          )

          guard let tab = tab,
            let tabHelper = tab.getTabHelper(named: BraveTranslateTabHelper.tabHelperName)
              as? BraveTranslateTabHelper
          else {
            replyHandler(nil, BraveTranslateError.otherError.rawValue)
            return
          }

          let (data, response) = try await tabHelper.processTranslationRequest(message)

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
          Logger.module.error("Brave Translate Error: \(error)")
          replyHandler(nil, "Translation Error")
        }
      }

      return
    }

    replyHandler(nil, nil)
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
  private weak var tab: Tab?
  private static let namespace = "translate_\(uniqueID)"

  init(tab: Tab) {
    self.tab = tab
    super.init()
  }

  static let scriptName = "BraveTranslateLanguageDetectionScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "LanguageDetectionTextCaptured"
  static let scriptSandbox = WKContentWorld.world(name: "BraveTranslateContentWorld")
  static let userScript: WKUserScript? = nil

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceiveScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {

    // In the future we'd get this from: components/language/ios/browser/language_detection_java_script_feature.mm

    guard let body = message.body as? [String: Any] else {
      Logger.module.error("Invalid Brave Translate Language Detection Message")
      return
    }

    guard
      let tabHelper = tab?.getTabHelper(named: BraveTranslateTabHelper.tabHelperName)
        as? BraveTranslateTabHelper
    else {
      return
    }

    do {
      let message = try JSONDecoder().decode(
        Message.self,
        from: JSONSerialization.data(withJSONObject: body, options: .fragmentsAllowed)
      )

      if message.hasNoTranslate {
        tabHelper.currentLanguageInfo.pageLanguage = tabHelper.currentLanguageInfo.currentLanguage
      } else {
        tabHelper.currentLanguageInfo.pageLanguage =
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
