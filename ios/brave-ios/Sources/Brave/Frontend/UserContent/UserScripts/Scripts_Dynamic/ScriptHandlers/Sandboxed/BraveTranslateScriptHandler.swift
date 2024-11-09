// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Foundation
import NaturalLanguage
import Preferences
import Shared
import SwiftUI
import Translation
import WebKit
import os.log

protocol BraveTranslateScriptHandlerDelegate: NSObject {
  func updateTranslateURLBar(tab: Tab?, state: TranslateURLBarButton.TranslateState)
  func showTranslateOnboarding(tab: Tab?, completion: @escaping (_ translateEnabled: Bool) -> Void)
  func presentToast(tab: Tab?, languageInfo: BraveTranslateLanguageInfo)
}

class BraveTranslateScriptHandler: NSObject, TabContentScript {
  private weak var tab: Tab?
  private weak var delegate: BraveTranslateScriptHandlerDelegate?
  private let recognizer = NLLanguageRecognizer()
  private static let namespace = "translate_\(uniqueID)"

  private var url: URL?
  private var isTranslationReady = false
  private var translationController: UIHostingController<BraveTranslateContainerView>!
  private var translationSession: BraveTranslateSession?
  private var urlObserver: NSObjectProtocol?
  fileprivate var currentLanguageInfo = BraveTranslateLanguageInfo()
  private static var elementScriptTask: Task<String, Error> = downloadElementScript()
  private var translationTask: (() async throws -> Void)?
  private var canShowToast = false

  struct RequestMessage: Codable {
    let method: String
    let url: URL
    let headers: [String: String]
    let body: String
  }

  init(tab: Tab, delegate: BraveTranslateScriptHandlerDelegate) {
    self.tab = tab
    self.delegate = delegate
    self.url = tab.url
    super.init()

    translationController = UIHostingController(
      rootView: BraveTranslateContainerView(
        onTranslationSessionUpdated: { [weak self] session in
          guard let self = self else { return }

          self.translationSession = session
          if let translationTask = self.translationTask, session != nil {
            try? await translationTask()
            self.translationTask = nil
          }
        },
        languageInfo: currentLanguageInfo
      )
    )

    urlObserver = tab.webView?.observe(
      \.url,
      options: [.new],
      changeHandler: { [weak self] _, change in
        guard let self = self, let url = change.newValue else { return }
        if self.url != url {
          self.url = url
          self.isTranslationReady = false
          self.canShowToast = false
          self.delegate?.updateTranslateURLBar(tab: self.tab, state: .unavailable)
        }
      }
    )
  }

  deinit {
    translationController?.willMove(toParent: nil)
    translationController?.removeFromParent()
    translationController = nil
    translationTask = nil
  }

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
    if !Preferences.Translate.translateEnabled.value {
      replyHandler(nil, BraveTranslateError.translateDisabled.rawValue)
      return
    }

    guard translationController != nil else {
      Logger.module.error("Invalid Translation Controller")
      return
    }

    guard let body = message.body as? [String: Any] else {
      Logger.module.error("Invalid Brave Translate Message")
      return
    }

    if body["command"] as? String == "load_brave_translate_script" {
      Task {
        let script = try await BraveTranslateScriptHandler.elementScriptTask.value
        replyHandler(script, nil)
      }
      return
    }

    if body["command"] as? String == "request" {
      Task {
        do {
          let message = try JSONDecoder().decode(
            RequestMessage.self,
            from: JSONSerialization.data(withJSONObject: body, options: .fragmentsAllowed)
          )

          let translationSession = self.translationSession ?? BraveTranslateSession()
          let isTranslationRequest = BraveTranslateSession.isPhraseTranslationRequest(message)

          // The message is for HTML or CSS request
          if self.translationSession == nil
            && isTranslationRequest
          {
            throw BraveTranslateError.otherError
          }

          let (data, response) = try await translationSession.translate(message)

          guard let response = response as? HTTPURLResponse else {
            throw BraveTranslateError.invalidTranslationResponse
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

          if isTranslationRequest && canShowToast {
            canShowToast = false

            Task { @MainActor in
              self.delegate?.updateTranslateURLBar(tab: self.tab, state: .active)
              self.delegate?.presentToast(tab: self.tab, languageInfo: currentLanguageInfo)
            }
          }
        } catch {
          Logger.module.error("Brave Translate Error: \(error)")
          replyHandler(nil, "Translation Error")
        }
      }

      return
    }

    if body["command"] as? String == "ready" {
      isTranslationReady = true

      Task { @MainActor [weak self] in
        guard let self = self else {
          return
        }

        try Task.checkCancellation()
        let languageInfo = await getLanguageInfo()
        self.currentLanguageInfo.pageLanguage = languageInfo.pageLanguage
        self.currentLanguageInfo.currentLanguage = languageInfo.currentLanguage

        try Task.checkCancellation()
        try await updateTranslationStatus()
      }
    }

    replyHandler(nil, nil)
  }

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
  func executeChromiumFunction(_ functionName: String, args: [Any] = []) async throws -> Any {
    guard let webView = tab?.webView else {
      throw BraveTranslateError.otherError
    }

    let (result, error) = await webView.evaluateSafeJavaScript(
      functionName: "window.__gCrWeb.\(functionName)",
      args: args,
      contentWorld: BraveTranslateScriptHandler.scriptSandbox,
      asFunction: true
    )

    if let error = error {
      Logger.module.error("Unable to execute page function \(functionName) error: \(error)")
      throw error
    }

    return result
  }

  private func getPageSource() async -> String? {
    return await executePageFunction(name: "getPageSource")
  }

  @MainActor
  func guessLanguage() async -> Locale.Language? {
    try? await executeChromiumFunction("languageDetection.detectLanguage")

    // Language was identified by the Translate Script
    if let currentLanguage = currentLanguageInfo.currentLanguage.languageCode?.identifier,
      let pageLanguage = currentLanguageInfo.pageLanguage?.languageCode?.identifier
    {
      if currentLanguage == pageLanguage {
        currentLanguageInfo.pageLanguage = nil
      }
      return currentLanguageInfo.pageLanguage
    }

    // Language identified via our own Javascript
    if let languageCode = await executePageFunction(name: "getPageLanguage"), !languageCode.isEmpty
    {
      return Locale.Language(identifier: languageCode)
    }

    // Language identified by running the NL detection on the page source
    if let rawPageSource = await executePageFunction(name: "getRawPageSource") {
      recognizer.reset()
      recognizer.processString(rawPageSource)

      if let dominantLanguage = recognizer.dominantLanguage, dominantLanguage != .undetermined {
        return Locale.Language(identifier: dominantLanguage.rawValue)
      }
    }

    return nil
  }

  func getLanguageInfo() async -> BraveTranslateLanguageInfo {
    let pageLanguage = await guessLanguage()
    return .init(currentLanguage: Locale.current.language, pageLanguage: pageLanguage)
  }

  func startTranslation(canShowToast: Bool) {
    // Translation already in progress
    if tab?.translationState == .pending {
      return
    }

    // This is necessary because if the session hasn't be initialized yet (since it's asynchronous in Apple's API),
    // then translation would not be possible. So we need to store this request for now, and when the session is ready
    // then we execute the translation. If the session is already available, we execute immediately.
    self.translationTask = { @MainActor [weak self] in
      guard let self,
        let currentLanguage = currentLanguageInfo.currentLanguage.languageCode?.identifier,
        let pageLanguage = currentLanguageInfo.pageLanguage?.languageCode?.identifier,
        currentLanguage != pageLanguage
      else {
        throw BraveTranslateError.invalidLanguage
      }

      try Task.checkCancellation()

      // Normalize Chinese if necessary to Chinese (Simplified). zh-TW = Traditional
      // We don't current use the components/language/ios/browser/language_detection_java_script_feature.mm
      // Supported List of Languages is from: components/translate/core/browser/translate_language_list.cc
      // So we have to do this for now.
      try await executeChromiumFunction(
        "translate.startTranslation",
        args: [
          pageLanguage == "zh" ? "zh-CN" : pageLanguage,
          currentLanguage == "zh" ? "zh-CN" : currentLanguage,
        ]
      )

      try Task.checkCancellation()

      self.canShowToast = canShowToast
      self.delegate?.updateTranslateURLBar(tab: self.tab, state: .pending)
    }

    if translationSession != nil {
      Task { @MainActor in
        try? await translationTask?()
        translationTask = nil
      }
    }
  }

  func revertTranslation() {
    Task { @MainActor [weak self] in
      guard let self = self,
        self.isTranslationReady,
        self.tab?.translationState == .active
      else {
        return
      }

      try? await executeChromiumFunction("translate.revertTranslation")
      self.delegate?.updateTranslateURLBar(tab: self.tab, state: .available)
    }
  }

  func presentUI(on controller: UIViewController) {
    guard isTranslationReady else {
      return
    }

    if translationController?.parent != controller {
      controller.addChild(translationController)
      tab?.webView?.addSubview(translationController.view)
      translationController.didMove(toParent: controller)
      tab?.webView?.sendSubviewToBack(translationController.view)
    }
  }

  @MainActor
  private func updateTranslationStatus() async throws {
    guard let pageLanguage = currentLanguageInfo.pageLanguage else {
      delegate?.updateTranslateURLBar(tab: tab, state: .unavailable)
      return
    }

    var isTranslationSupported = false

    if #available(iOS 18.0, *) {
      #if compiler(>=6.0) && !targetEnvironment(simulator)
      isTranslationSupported = await BraveTranslateSessionApple.isTranslationSupported(
        from: pageLanguage,
        to: currentLanguageInfo.currentLanguage
      )
      #else
      isTranslationSupported = await BraveTranslateSession.isTranslationSupported(
        from: pageLanguage,
        to: currentLanguageInfo.currentLanguage
      )
      #endif
    } else {
      isTranslationSupported = await BraveTranslateSession.isTranslationSupported(
        from: pageLanguage,
        to: currentLanguageInfo.currentLanguage
      )
    }

    try Task.checkCancellation()
    delegate?.updateTranslateURLBar(
      tab: tab,
      state: isTranslationSupported ? .available : .unavailable
    )
  }

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
  private weak var delegate: BraveTranslateScriptHandler?
  private static let namespace = "translate_\(uniqueID)"

  init(tab: Tab, delegate: BraveTranslateScriptHandler) {
    self.tab = tab
    self.delegate = delegate
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

    do {
      guard let delegate = delegate else {
        replyHandler(nil, nil)
        return
      }

      let message = try JSONDecoder().decode(
        Message.self,
        from: JSONSerialization.data(withJSONObject: body, options: .fragmentsAllowed)
      )

      if message.hasNoTranslate {
        delegate.currentLanguageInfo.pageLanguage = delegate.currentLanguageInfo.currentLanguage
      } else {
        delegate.currentLanguageInfo.pageLanguage =
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

private enum BraveTranslateError: String, Error {
  case invalidURL
  case invalidLanguage
  case sameLanguage
  case invalidPageSource
  case invalidTranslationResponseCode
  case invalidTranslationResponse
  case translateDisabled
  case otherError
}

class BraveTranslateLanguageInfo: ObservableObject {
  @Published
  var currentLanguage: Locale.Language

  @Published
  var pageLanguage: Locale.Language?

  init() {
    currentLanguage = .init(identifier: Locale.current.identifier)
    pageLanguage = nil
  }

  init(currentLanguage: Locale.Language, pageLanguage: Locale.Language?) {
    self.currentLanguage = currentLanguage
    self.pageLanguage = pageLanguage
  }
}

private class BraveTranslateSession {
  static func isPhraseTranslationRequest(
    _ request: BraveTranslateScriptHandler.RequestMessage
  ) -> Bool {
    return request.url.path.starts(with: "/translate_a/")
  }

  class func isTranslationSupported(
    from source: Locale.Language,
    to target: Locale.Language
  ) async -> Bool {
    guard let sourceLanguage = source.languageCode?.identifier,
      let targetLanguage = target.languageCode?.identifier
    else {
      return false
    }
    return sourceLanguage != targetLanguage
  }

  func translate(
    _ request: BraveTranslateScriptHandler.RequestMessage
  ) async throws -> (data: Data, response: URLResponse) {
    var urlRequest = URLRequest(url: request.url)
    urlRequest.httpMethod = request.method
    urlRequest.httpBody = request.body.data(using: .utf8)
    request.headers.forEach { (key, value) in
      urlRequest.setValue(value, forHTTPHeaderField: key)
    }

    let session = URLSession(configuration: .ephemeral)
    defer { session.finishTasksAndInvalidate() }
    return try await session.data(for: urlRequest)
  }
}

#if compiler(>=6.0) && !targetEnvironment(simulator)
@available(iOS 18.0, *)
private class BraveTranslateSessionApple: BraveTranslateSession {
  private weak var session: TranslationSession?

  init(session: TranslationSession) {
    self.session = session
  }

  override class func isTranslationSupported(
    from source: Locale.Language,
    to target: Locale.Language
  ) async -> Bool {
    let availability = LanguageAvailability()
    let status = await availability.status(from: source, to: target)
    switch status {
    case .installed, .supported:
      return true
    case .unsupported:
      return false
    @unknown default:
      return false
    }
  }

  override func translate(
    _ request: BraveTranslateScriptHandler.RequestMessage
  ) async throws -> (data: Data, response: URLResponse) {
    // Do not attempt to translate requests to html and css from Brave-Translate script
    guard Self.isPhraseTranslationRequest(request) else {
      return try await super.translate(request)
    }

    guard let session = session else {
      throw BraveTranslateError.otherError
    }

    let components = URLComponents(string: "https://translate.brave.com?\(request.body)")
    let phrases = components!.queryItems!.map({ "\($0.value ?? "")" })
    let results = try await session.translations(
      from: phrases.map({ .init(sourceText: $0) })
    ).map({ $0.targetText })

    let data = try JSONSerialization.data(withJSONObject: results, options: [])
    let response =
      HTTPURLResponse(
        url: request.url,
        statusCode: 200,
        httpVersion: "HTTP/1.1",
        headerFields: ["Content-Type": "text/html"]
      )
      ?? URLResponse(
        url: request.url,
        mimeType: "text/html",
        expectedContentLength: data.count,
        textEncodingName: "UTF-8"
      )
    return (data, response)
  }
}
#endif

private struct BraveTranslateContainerView: View {
  var onTranslationSessionUpdated: ((BraveTranslateSession?) async -> Void)?

  @ObservedObject
  var languageInfo: BraveTranslateLanguageInfo

  var body: some View {
    Color.clear
      .osAvailabilityModifiers({ view in
        if #available(iOS 18.0, *) {
          #if compiler(>=6.0) && !targetEnvironment(simulator)
          view
            .translationTask(
              .init(source: languageInfo.pageLanguage, target: languageInfo.currentLanguage),
              action: { session in
                do {
                  try await session.prepareTranslation()
                  await onTranslationSessionUpdated?(BraveTranslateSessionApple(session: session))
                } catch {
                  Logger.module.error("Session Unavailable: \(error)")
                  await onTranslationSessionUpdated?(nil)
                }
              }
            )
          #else
          view.task {
            await onTranslationSessionUpdated?(BraveTranslateSession())
          }
          #endif
        } else {
          view.task {
            await onTranslationSessionUpdated?(BraveTranslateSession())
          }
        }
      })
  }
}
