// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Preferences
import Shared
import Web
import WebKit
import os.log

protocol BraveTranslateScriptHandlerDelegate: NSObject {
  func updateTranslateURLBar(tab: some TabState, state: TranslateURLBarButton.TranslateState)
  func canShowTranslateOnboarding(tab: some TabState) -> Bool
  func showTranslateOnboarding(
    tab: some TabState,
    completion: @escaping (_ translateEnabled: Bool) -> Void
  )
  func presentTranslateToast(tab: some TabState, languageInfo: BraveTranslateLanguageInfo)
  func presentTranslateError(tab: some TabState)
}

class BraveTranslateScriptHandler: NSObject, TabContentScript {
  private static var elementScript: String?
  private var tasks = [UUID: Task<Void, Error>]()

  static let namespace = "translate_\(uniqueID)"
  static let scriptName = "BraveTranslateScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "TranslateMessage"
  static let scriptSandbox = WKContentWorld.defaultClient
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

  deinit {
    tasks.values.forEach({ $0.cancel() })
  }

  static func checkTranslate(tab: some TabState) {
    tab.evaluateJavaScript(
      functionName:
        """
        try {
          window.__firefox__.\(namespace).detectLanguage();
        } catch(error) {
          // Page & Script not loaded yet
        }
        """,
      contentWorld: BraveTranslateScriptHandler.scriptSandbox,
      asFunction: false
    )
  }

  func tab(
    _ tab: some TabState,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    // Setup
    let isReaderMode = tab.visibleURL?.isInternalURL(for: .readermode) == true
    if tab.visibleSecureContentState != .secure && !isReaderMode {
      Logger.module.debug("Translation Disabled - Insecure Page")
      replyHandler(nil, BraveTranslateError.translateDisabled.rawValue)
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
    let taskId = UUID()
    let task = Task { [weak self, weak tab] in
      guard let self = self, let tab = tab else {
        replyHandler(nil, BraveTranslateError.otherError.rawValue)
        return
      }

      print(Thread.current.name ?? "Unknown Thread")

      defer {
        self.tasks.removeValue(forKey: taskId)
      }

      let (result, error) = try await processScriptMessage(for: tab, command: command, body: body)
      replyHandler(result, error)
    }

    tasks[taskId] = task
  }

  private func processScriptMessage(
    for tab: some TabState,
    command: String,
    body: [String: Any]
  ) async throws -> (Any?, String?) {
    if command == "load_brave_translate_script" {
      if Preferences.Translate.translateEnabled.value {
        if let script = Self.elementScript {
          return (script, nil)
        }

        Self.elementScript = try await downloadElementScript()
        return (Self.elementScript, nil)
      }

      return (nil, BraveTranslateError.translateDisabled.rawValue)
    }

    if command == "ready" {
      // Translate is ready
      try await tab.translateHelper?.beginSetup()
      return (nil, nil)
    }

    if command == "request" {
      do {
        let message = try JSONDecoder().decode(
          BraveTranslateSession.RequestMessage.self,
          from: JSONSerialization.data(withJSONObject: body, options: .fragmentsAllowed)
        )

        guard let translateHelper = tab.translateHelper
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
      guard let translateHelper = tab.translateHelper else {
        Logger.module.debug("[Brave Translate] - Status: \(body)")
        return (nil, nil)
      }

      if let errorCode = body["errorCode"] as? Int,
        let status = BraveTranslateTabHelper.TranslateError(rawValue: errorCode)
      {
        Logger.module.debug("[Brave Translate] - Status: \(String(describing: status))")
        await translateHelper.setTranslationStatus(status: status)
      } else {
        Logger.module.debug("[Brave Translate] - Status: \(body)")
        await translateHelper.setTranslationStatus(status: .unexpectedScriptError)
      }
    }

    return (nil, nil)
  }

  // MARK: - Private

  private func downloadElementScript() async throws -> String {
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

class BraveTranslateScriptLanguageDetectionHandler: NSObject, TabContentScript {
  private static let namespace = "translate_\(uniqueID)"

  static let scriptName = "BraveTranslateLanguageDetectionScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "LanguageDetectionTextCaptured"
  // This sandbox must always be the same world as the translate script
  // Chromium has them as separate handlers, but in the same injected script, in the same sandbox
  static let scriptSandbox = BraveTranslateScriptHandler.scriptSandbox
  static let userScript: WKUserScript? = nil

  func tab(
    _ tab: some TabState,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    // In the future we'd get this from: components/language/ios/browser/language_detection_java_script_feature.mm

    guard let body = message.body as? [String: Any] else {
      Logger.module.error("Invalid Brave Translate Language Detection Message")
      return
    }

    guard let translateHelper = tab.translateHelper else {
      return
    }

    do {
      let message = try JSONDecoder().decode(
        Message.self,
        from: JSONSerialization.data(withJSONObject: body, options: .fragmentsAllowed)
      )

      // The page cannot be translated because it has "noTranslate" flag set,
      // Or because the detected language code isn't valid.
      if message.hasNoTranslate || !Locale.LanguageCode(message.htmlLang).isISOLanguage {
        translateHelper.currentLanguageInfo.pageLanguage = nil
      } else {
        translateHelper.currentLanguageInfo.pageLanguage =
          !message.htmlLang.isEmpty
          ? Locale.Language(languageCode: .init(message.htmlLang))
          : nil
      }

      if translateHelper.currentLanguageInfo.currentLanguage
        == translateHelper.currentLanguageInfo.pageLanguage
      {
        translateHelper.currentLanguageInfo.pageLanguage = nil
      }

      Task { [weak translateHelper] in
        try await translateHelper?.finishSetup()
        replyHandler(nil, nil)
      }
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
