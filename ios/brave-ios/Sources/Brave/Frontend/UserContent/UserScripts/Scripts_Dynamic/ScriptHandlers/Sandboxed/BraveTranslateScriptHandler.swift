// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#if compiler(>=6.0)
import Foundation
import NaturalLanguage
import Shared
import Translation
import WebKit
import os.log

@available(iOS 18.0, *)
enum BraveTranslateError: Error {
  case invalidURL
  case invalidLanguage
  case sameLanguage
  case invalidPageSource
  case invalidTranslationResponseCode
  case invalidTranslationResponse
  case otherError
}

@available(iOS 18.0, *)
class BraveTranslateScriptHandler: NSObject, TabContentScript {
  private weak var tab: Tab?
  private let recognizer = NLLanguageRecognizer()
  private static let namespace = "translate_\(uniqueID)"
  private var translationSession: TranslationSession?

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
  func guessLanguage() async -> Locale.Language? {
    if let rawPageSource = await executePageFunction(name: "getRawPageSource") {
      recognizer.reset()
      recognizer.processString(rawPageSource)

      if let dominantLanguage = recognizer.dominantLanguage, dominantLanguage != .undetermined {
        return Locale.Language(identifier: dominantLanguage.rawValue)
      }
    }

    if let languageCode = await executePageFunction(name: "getPageLanguage") {
      return Locale.Language(identifier: languageCode)
    }

    return Locale.current.language
  }

  func getLanguageInfo() async throws -> (Locale.Language, Locale.Language) {
    let pageLanguage = await guessLanguage() ?? Locale.current.language
    return (Locale.current.language, pageLanguage)
  }

  func translatePage(
    pageSource: String
      //    from fromLanguage: String?,
      //    to toLanguage: String?
  ) async throws -> String {
    guard let translationSession else { throw BraveTranslateError.otherError }
    //    var currentLanguageCode = ""

    //    if let toLanguage = toLanguage {
    //      currentLanguageCode = toLanguage
    //    } else {
    //      guard let currentLanguage = Locale.current.language.languageCode?.identifier else {
    //        throw BraveTranslateError.invalidLanguage
    //      }
    //
    //      currentLanguageCode = currentLanguage
    //    }
    //
    //    var pageLanguage = currentLanguageCode
    //    if let fromLanguage = fromLanguage {
    //      pageLanguage = fromLanguage
    //    } else {
    //      pageLanguage = await guessLanguage() ?? currentLanguageCode
    //    }

    // Cannot translate from source to current as they're the same
    //    if pageLanguage.isEmpty || currentLanguageCode.isEmpty || pageLanguage == currentLanguageCode {
    //      throw BraveTranslateError.sameLanguage
    //    }

    return try await translationSession.translate(pageSource).targetText
  }

  func activateScript(using session: TranslationSession) {
    guard let webView = tab?.webView else {
      return
    }

    self.translationSession = session

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

        let result = try await translatePage(pageSource: text)
        replyHandler(result, nil)
        //        if let result = result as? [String] {
        //          let prefix = text.prefix(while: {
        //            $0.unicodeScalars.allSatisfy(CharacterSet.whitespacesAndNewlines.contains)
        //          })
        //          let suffix = text.suffix(while: {
        //            $0.unicodeScalars.allSatisfy(CharacterSet.whitespacesAndNewlines.contains)
        //          })
        //          replyHandler("\(prefix)\(result[0])\(suffix)", nil)
        //        } else {
        //          replyHandler(nil, nil)
        //        }
      } catch {
        replyHandler(nil, nil)
      }
    }
  }
}
#endif
