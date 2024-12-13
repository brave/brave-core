// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import NaturalLanguage
import SwiftUI
import WebKit
import os.log

enum BraveTranslateError: String, Error {
  case invalidURL
  case invalidLanguage
  case sameLanguage
  case invalidPageSource
  case invalidTranslationResponseCode
  case invalidTranslationResponse
  case translateDisabled
  case otherError
}

class BraveTranslateTabHelper: NSObject {
  private weak var tab: Tab?
  private weak var delegate: BraveTranslateScriptHandlerDelegate?
  private let recognizer = NLLanguageRecognizer()

  private var url: URL?
  private var isTranslationReady = false
  private var translationController: UIHostingController<BraveTranslateContainerView>!
  private var translationSession: BraveTranslateSession?
  private var urlObserver: NSObjectProtocol?
  private var translationTask: (() async throws -> Void)?
  private var canShowToast = false

  var currentLanguageInfo = BraveTranslateLanguageInfo()

  // All TabHelpers in Chromium have a `WebState* web_state` parameter in their constructor
  // WebState in Brave, is the same as `Tab`.
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

  func startTranslation(canShowToast: Bool) {
    guard let tab = tab else {
      return
    }

    // Translation already in progress
    if tab.translationState == .pending {
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
        tab: tab,
        name: "translate.startTranslation",
        args: [
          pageLanguage == "zh" ? "zh-CN" : pageLanguage,
          currentLanguage == "zh" ? "zh-CN" : currentLanguage,
        ]
      )

      try Task.checkCancellation()

      self.canShowToast = canShowToast
      self.delegate?.updateTranslateURLBar(tab: tab, state: .pending)
    }

    if translationSession != nil {
      Task { @MainActor in
        try? await translationTask?()
        translationTask = nil
      }
    }
  }

  func revertTranslation() {
    guard let tab = tab else {
      return
    }

    Task { @MainActor [weak self] in
      guard let self = self,
        self.isTranslationReady,
        tab.translationState == .active
      else {
        return
      }

      try? await executeChromiumFunction(tab: tab, name: "translate.revertTranslation")
      self.delegate?.updateTranslateURLBar(tab: tab, state: .available)
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
  func setupTranslate() async throws {
    guard let tab = tab else {
      return
    }

    isTranslationReady = true
    let languageInfo = await getLanguageInfo(for: tab)
    self.currentLanguageInfo.pageLanguage = languageInfo.pageLanguage
    self.currentLanguageInfo.currentLanguage = languageInfo.currentLanguage
    try Task.checkCancellation()
    try await updateTranslationStatus(for: tab)
  }

  @MainActor
  func processTranslationRequest(
    _ request: BraveTranslateSession.RequestMessage
  ) async throws -> (Data, HTTPURLResponse) {
    let translationSession = self.translationSession ?? BraveTranslateSession()
    let isTranslationRequest = BraveTranslateSession.isPhraseTranslationRequest(request)

    // The message is for HTML or CSS request
    if self.translationSession == nil
      && isTranslationRequest
    {
      throw BraveTranslateError.otherError
    }

    let (data, response) = try await translationSession.translate(request)

    guard let response = response as? HTTPURLResponse else {
      throw BraveTranslateError.invalidTranslationResponse
    }

    if isTranslationRequest && canShowToast {
      canShowToast = false

      Task { @MainActor in
        self.delegate?.updateTranslateURLBar(tab: tab, state: .active)
        self.delegate?.presentToast(tab: tab, languageInfo: currentLanguageInfo)
      }
    }

    return (data, response)
  }

  // MARK: - Private

  @MainActor
  private func executePageFunction(tab: Tab, name functionName: String) async -> String? {
    guard let webView = tab.webView else {
      return nil
    }

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
  private func executeChromiumFunction(
    tab: Tab,
    name functionName: String,
    args: [Any] = []
  ) async throws -> Any {
    guard let webView = tab.webView else {
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

  private func getPageSource(for tab: Tab) async -> String? {
    return await executePageFunction(tab: tab, name: "getPageSource")
  }

  @MainActor
  private func guessLanguage(for tab: Tab) async -> Locale.Language? {
    try? await executeChromiumFunction(tab: tab, name: "languageDetection.detectLanguage")

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
    if let languageCode = await executePageFunction(tab: tab, name: "getPageLanguage"),
      !languageCode.isEmpty
    {
      return Locale.Language(identifier: languageCode)
    }

    // Language identified by running the NL detection on the page source
    if let rawPageSource = await executePageFunction(tab: tab, name: "getRawPageSource") {
      recognizer.reset()
      recognizer.processString(rawPageSource)

      if let dominantLanguage = recognizer.dominantLanguage, dominantLanguage != .undetermined {
        return Locale.Language(identifier: dominantLanguage.rawValue)
      }
    }

    return nil
  }

  private func getLanguageInfo(for tab: Tab) async -> BraveTranslateLanguageInfo {
    let pageLanguage = await guessLanguage(for: tab)
    return .init(currentLanguage: Locale.current.language, pageLanguage: pageLanguage)
  }

  @MainActor
  private func updateTranslationStatus(for tab: Tab) async throws {
    guard let pageLanguage = currentLanguageInfo.pageLanguage else {
      delegate?.updateTranslateURLBar(tab: tab, state: .unavailable)
      return
    }

    let isTranslationSupported = await BraveTranslateSession.isTranslationSupported(
      from: pageLanguage,
      to: currentLanguageInfo.currentLanguage
    )

    try Task.checkCancellation()
    delegate?.updateTranslateURLBar(
      tab: tab,
      state: isTranslationSupported ? .available : .unavailable
    )
  }
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
