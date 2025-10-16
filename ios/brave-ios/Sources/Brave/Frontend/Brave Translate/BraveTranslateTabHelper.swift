// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import SwiftUI
import Web
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

class BraveTranslateTabHelper: NSObject, TabObserver {
  private weak var tab: (any TabState)?
  private weak var delegate: BraveTranslateScriptHandlerDelegate?
  private static var requestCache = [URL: (data: Data, response: HTTPURLResponse)]()
  private var tasks = [UUID: Task<Void, Error>]()

  private var isTranslationReady = false
  private var translationController: UIHostingController<BraveTranslateContainerView>!
  private var translationSession: BraveTranslateSession?
  private var translationTask: (() async throws -> Void)?
  private var canShowToast = false

  var currentLanguageInfo = BraveTranslateLanguageInfo()

  // Matches //components/translate/core/common/translate_errors.h
  enum TranslateError: Int, Error {
    case noError = 0
    case networkError
    case initializationError
    case unknownLanguage
    case unsupportedLanguage
    case identicalLanguages
    case translationError
    case translationTimeout
    case unexpectedScriptError
    case badOrigin
    case scriptLoadError
    case errorMax
  }

  // All TabHelpers in Chromium have a `WebState* web_state` parameter in their constructor
  // WebState in Brave, is the same as `Tab`.
  init(tab: some TabState, delegate: BraveTranslateScriptHandlerDelegate) {
    self.tab = tab
    self.delegate = delegate
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

    tab.addObserver(self)
  }

  deinit {
    translationController?.willMove(toParent: nil)
    translationController?.removeFromParent()
    translationController = nil
    translationTask = nil

    tasks.values.forEach({ $0.cancel() })
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
      guard let self = self, let tab = self.tab, let delegate = self.delegate else {
        throw BraveTranslateError.otherError
      }

      let currentLanguage = currentLanguageInfo.currentLanguage.braveTranslateLanguageIdentifier

      guard
        let pageLanguage = currentLanguageInfo.pageLanguage?.braveTranslateLanguageIdentifier,
        currentLanguage != pageLanguage
      else {
        throw BraveTranslateError.invalidLanguage
      }

      let previousState = tab.translationState ?? .unavailable
      delegate.updateTranslateURLBar(tab: tab, state: .pending)

      // TranslateAgent::TranslateFrame
      try Task.checkCancellation()

      // If the translation library isn't already loaded,
      // manually load it
      if !(await isTranslateLibAvailable()) {
        try Task.checkCancellation()

        // Load the translate script manually
        try await loadTranslateScript()

        try Task.checkCancellation()

        if !(await isTranslateLibAvailable()) {
          delegate.updateTranslateURLBar(tab: tab, state: previousState)
          throw BraveTranslateError.otherError
        }
      }

      // TranslateAgent::TranslatePageImpl
      // Check if the translate library is ready
      var attempts = 0
      while !(await isTranslateLibReady()) {
        try Task.checkCancellation()

        let error = await getTranslationErrorCode()
        if error != .noError {
          delegate.updateTranslateURLBar(tab: tab, state: previousState)
          delegate.presentTranslateError(tab: tab)
          return
        }

        try Task.checkCancellation()

        // Check a maximum of 5 times. The JS library throws an error
        // if we check a 6th time.
        // This matches Chromium's TranslateAgent::kMaxTranslateInitCheckAttempts
        attempts += 1
        if attempts >= 5 {
          delegate.updateTranslateURLBar(tab: tab, state: previousState)
          delegate.presentTranslateError(tab: tab)
          return
        }

        // To give the JS library time to load
        // we sleep 150ms.
        // This is the same in Chromium: TranslateAgent::kTranslateInitCheckDelayMs
        try await Task.sleep(seconds: 0.15)
      }

      // The library is ready, we can now begin translating the page
      let didStartTranslation = await translate(
        from: pageLanguage,
        to: currentLanguage
      )

      try Task.checkCancellation()

      if !didStartTranslation {
        self.canShowToast = canShowToast
        try await checkTranslationStatus()
        return
      }

      try Task.checkCancellation()

      // To give the JS library time to update its status
      // we sleep 400ms.
      // This is the same in Chromium: TranslateAgent::kTranslateStatusCheckDelayMs
      try await Task.sleep(seconds: 0.4)
      try Task.checkCancellation()
      try await checkTranslationStatus()
    }

    if translationSession != nil {
      let taskId = UUID()
      let task = Task { @MainActor [weak self] in
        guard let self = self else {
          return
        }

        defer {
          tasks.removeValue(forKey: taskId)
        }

        try await self.translationTask?()
        self.translationTask = nil
      }

      tasks[taskId] = task
    }
  }

  func revertTranslation() {
    let taskId = UUID()
    let task = Task { @MainActor [weak self] in
      guard let self = self else {
        return
      }

      defer {
        tasks.removeValue(forKey: taskId)
      }

      guard let tab = self.tab,
        let delegate = delegate,
        tab.translationState == .active
      else {
        return
      }

      try Task.checkCancellation()

      // If the translate library hasn't been loaded yet
      if !(await isTranslateLibAvailable()) {
        try Task.checkCancellation()

        // Load the translate script manually
        try await loadTranslateScript()
        return
      }

      await undoTranslate()
      try Task.checkCancellation()

      delegate.updateTranslateURLBar(tab: tab, state: .available)
    }

    tasks[taskId] = task
  }

  @MainActor
  func setTranslationStatus(status: TranslateError) async {
    guard let tab = tab, let delegate = delegate else { return }

    defer {
      canShowToast = false
    }

    if status == .noError {
      delegate.updateTranslateURLBar(tab: tab, state: .active)

      if canShowToast {
        delegate.presentTranslateToast(tab: tab, languageInfo: currentLanguageInfo)
      }
      return
    }

    if [.translationError, .translationTimeout].contains(status) {
      delegate.updateTranslateURLBar(tab: tab, state: .available)
      delegate.presentTranslateError(tab: tab)
    } else {
      delegate.updateTranslateURLBar(tab: tab, state: .unavailable)
    }
  }

  func presentUI(on controller: UIViewController) {
    if translationController?.parent != controller {
      controller.addChild(translationController)
      tab?.view.addSubview(translationController.view)
      translationController.didMove(toParent: controller)
      tab?.view.sendSubviewToBack(translationController.view)
    }
  }

  @MainActor
  func beginSetup() async throws {
    guard tab != nil else {
      return
    }

    isTranslationReady = true
    await detectLanguage()
  }

  @MainActor
  func finishSetup() async throws {
    guard let tab = tab, let delegate = delegate else {
      return
    }

    guard let pageLanguage = currentLanguageInfo.pageLanguage else {
      delegate.updateTranslateURLBar(tab: tab, state: .unavailable)
      return
    }

    // Check if the page is already translated
    let isTranslatedAlready = await isPageTranslated()
    if isTranslatedAlready {
      delegate.updateTranslateURLBar(tab: tab, state: .active)
      return
    }

    // Check if the translation language is supported
    let isTranslationSupported = await BraveTranslateSession.isTranslationSupported(
      from: pageLanguage,
      to: currentLanguageInfo.currentLanguage
    )

    try Task.checkCancellation()

    delegate.updateTranslateURLBar(
      tab: tab,
      state: isTranslationSupported ? .available : .unavailable
    )

    // Translation is not supported on this page, so do not show onboarding
    // Return immediately
    if !isTranslationSupported {
      return
    }

    try Task.checkCancellation()

    // Check if the user can view the translation onboarding
    guard delegate.canShowTranslateOnboarding(tab: tab) else {
      delegate.updateTranslateURLBar(
        tab: tab,
        state: Preferences.Translate.translateEnabled.value ? .available : .unavailable
      )

      return
    }

    // Let the user enable or disable translation via onboarding
    let translateEnabled = await withCheckedContinuation { continuation in
      delegate.showTranslateOnboarding(tab: tab) { translateEnabled in
        continuation.resume(returning: translateEnabled)
      }
    }

    try Task.checkCancellation()

    delegate.updateTranslateURLBar(
      tab: tab,
      state: translateEnabled ? .available : .unavailable
    )

    // User enabled translation via onboarding
    if translateEnabled {
      // Load the translate script manually
      try await loadTranslateScript()
    }
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

    // Check request cache for CSS and JS requests
    let isCacheableRequest =
      request.method == "GET"
      && (request.url.lastPathComponent == "translateelement.css"
        || request.url.lastPathComponent == "main.js")

    if isCacheableRequest {
      if let (data, response) = Self.requestCache[request.url], response.statusCode == 200 {
        return (data, response)
      }
    }

    let (data, response) = try await translationSession.translate(request)

    guard let response = response as? HTTPURLResponse else {
      throw BraveTranslateError.invalidTranslationResponse
    }

    // Cache CSS and JS requests
    if isCacheableRequest {
      Self.requestCache[request.url] = (data, response)
    }

    return (data, response)
  }

  // MARK: - TabObserver

  func tabDidFinishNavigation(_ tab: some TabState) {
    Task { @MainActor [weak self, weak tab] in
      guard let self = self, let tab = tab else { return }

      canShowToast = false
      translationTask = nil

      // Check if the page is already translated
      let isTranslatedAlready = await isPageTranslated()
      if isTranslatedAlready {
        return
      }

      currentLanguageInfo.currentLanguage = .init(identifier: Locale.current.identifier)
      currentLanguageInfo.pageLanguage = nil

      if let delegate = self.delegate {
        delegate.updateTranslateURLBar(tab: tab, state: .unavailable)
        BraveTranslateScriptHandler.checkTranslate(tab: tab)
      }
    }
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  // MARK: - Private

  @MainActor
  private func loadTranslateScript() async throws {
    guard let tab else {
      throw BraveTranslateError.otherError
    }

    // Lazy loading. Load the translate script if needed.
    _ = try? await tab.callAsyncJavaScript(
      """
      await window.__firefox__.\(BraveTranslateScriptHandler.namespace).loadTranslateScript();
      """,
      contentWorld: BraveTranslateScriptHandler.scriptSandbox
    )
  }

  @MainActor
  private func isPageTranslated() async -> Bool {
    guard let tab else { return false }

    do {
      let result = try await tab.evaluateJavaScript(
        functionName: """
          typeof cr != 'undefined' && typeof cr.googleTranslate != 'undefined' && 
          typeof cr.googleTranslate.translate == 'function' &&
          window.__firefox__.\(BraveTranslateScriptHandler.namespace).isPageTranslated()
          """,
        contentWorld: BraveTranslateScriptHandler.scriptSandbox,
        asFunction: false
      )
      return result as? Bool == true
    } catch {
      Logger.module.error("isPageTranslated error: \(error)")
      return false
    }
  }

  @MainActor
  private func checkTranslationStatus() async throws {
    // Give the Javascript 5 seconds to execute and update its status
    let timeout = 5
    let startTime = Date()

    guard let tab = tab, let delegate = delegate else {
      throw BraveTranslateError.otherError
    }

    while Date().timeIntervalSince(startTime) < TimeInterval(timeout) {
      if await hasTranslationFailed() {
        delegate.updateTranslateURLBar(tab: tab, state: .available)
        delegate.presentTranslateError(tab: tab)
        throw BraveTranslateError.otherError
      }

      try Task.checkCancellation()

      if await hasTranslationFinished() {
        let actualSourceLanguage = await getPageSourceLanguage()
        if actualSourceLanguage.isEmpty
          || actualSourceLanguage
            == currentLanguageInfo.currentLanguage.braveTranslateLanguageIdentifier
        {
          delegate.updateTranslateURLBar(tab: tab, state: .available)
          delegate.presentTranslateError(tab: tab)
          return
        }

        delegate.updateTranslateURLBar(tab: tab, state: .active)
        delegate.presentTranslateToast(tab: tab, languageInfo: currentLanguageInfo)
        return
      }

      try await Task.sleep(seconds: 0.4)
      try Task.checkCancellation()
    }

    delegate.updateTranslateURLBar(tab: tab, state: .available)
    delegate.presentTranslateError(tab: tab)

    throw BraveTranslateError.otherError
  }

  @MainActor
  private func detectLanguage() async {
    guard let tab else { return }
    _ = try? await tab.evaluateJavaScript(
      functionName: "__gCrWeb.languageDetection.detectLanguage",
      contentWorld: BraveTranslateScriptHandler.scriptSandbox,
      asFunction: true
    )
  }

  @MainActor
  private func getPageSourceLanguage() async -> String {
    guard let tab else { return "" }

    do {
      let result = try await tab.evaluateJavaScript(
        functionName: "cr.googleTranslate.sourceLang",
        contentWorld: BraveTranslateScriptHandler.scriptSandbox,
        asFunction: false
      )
      return result as? String ?? ""
    } catch {
      Logger.module.error("cr.googleTranslate.sourceLang error: \(error)")
      return ""
    }
  }

  @MainActor
  private func translate(from pageLanguage: String, to currentLanguage: String) async -> Bool {
    guard let tab else {
      return false
    }

    do {
      let result = try await tab.evaluateJavaScript(
        functionName: "cr.googleTranslate.translate",
        args: [pageLanguage, currentLanguage],
        contentWorld: BraveTranslateScriptHandler.scriptSandbox,
        asFunction: true
      )
      return result as? Bool == true
    } catch {
      Logger.module.error("cr.googleTranslate.translate error: \(error)")
      return false
    }
  }

  private func undoTranslate() {
    guard let tab else {
      return
    }

    tab.evaluateJavaScript(
      functionName: "cr.googleTranslate.revert",
      contentWorld: BraveTranslateScriptHandler.scriptSandbox,
      asFunction: true
    )
  }

  @MainActor
  private func isTranslateLibAvailable() async -> Bool {
    guard let tab else {
      return false
    }

    do {
      let result = try await tab.evaluateJavaScript(
        functionName: """
          typeof cr != 'undefined' && typeof cr.googleTranslate != 'undefined' && 
          typeof cr.googleTranslate.translate == 'function' &&
          window.__firefox__.\(BraveTranslateScriptHandler.namespace).isTranslateScriptLoaded()
          """,
        contentWorld: BraveTranslateScriptHandler.scriptSandbox,
        asFunction: false
      )
      return result as? Bool == true
    } catch {
      Logger.module.error("cr.googleTranslate.translateLibAvailable error: \(error)")
      return false
    }
  }

  @MainActor
  private func isTranslateLibReady() async -> Bool {
    guard let tab else {
      return false
    }

    do {
      let result = try await tab.evaluateJavaScript(
        functionName: "cr.googleTranslate.libReady",
        contentWorld: BraveTranslateScriptHandler.scriptSandbox,
        asFunction: false
      )
      return result as? Bool == true
    } catch {
      Logger.module.error("cr.googleTranslate.libReady error: \(error)")
      return false
    }
  }

  @MainActor
  private func hasTranslationFinished() async -> Bool {
    guard let tab else {
      return true
    }

    do {
      let result = try await tab.evaluateJavaScript(
        functionName: "cr.googleTranslate.finished",
        contentWorld: BraveTranslateScriptHandler.scriptSandbox,
        asFunction: false
      )
      return result as? Bool ?? true
    } catch {
      Logger.module.error("cr.googleTranslate.finished error: \(error)")
      return true
    }
  }

  @MainActor
  private func hasTranslationFailed() async -> Bool {
    guard let tab else {
      return true
    }

    do {
      let result = try await tab.evaluateJavaScript(
        functionName: "cr.googleTranslate.error",
        contentWorld: BraveTranslateScriptHandler.scriptSandbox,
        asFunction: false
      )
      return result as? Bool ?? true
    } catch {
      Logger.module.error("cr.googleTranslate.error error: \(error)")
      return true
    }
  }

  @MainActor
  private func getTranslationErrorCode() async -> TranslateError {
    guard let tab else {
      return .errorMax
    }

    do {
      let result = try await tab.evaluateJavaScript(
        functionName: "cr.googleTranslate.errorCode",
        contentWorld: BraveTranslateScriptHandler.scriptSandbox,
        asFunction: false
      )
      if let result = result as? Int {
        return .init(rawValue: result) ?? .errorMax
      }
      return .errorMax
    } catch {
      Logger.module.error("cr.googleTranslate.errorCode error: \(error)")
      return .errorMax
    }
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
