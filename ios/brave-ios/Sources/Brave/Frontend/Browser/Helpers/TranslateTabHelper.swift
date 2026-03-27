// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
@_spi(ChromiumWebViewAccess) import Web

extension TabDataValues {
  private struct TranslateTabHelperKey: TabDataKey {
    static var defaultValue: TranslateTabHelper?
  }

  var translate: TranslateTabHelper? {
    get { self[TranslateTabHelperKey.self] }
    set { self[TranslateTabHelperKey.self] = newValue }
  }

  private struct TranslationStateKey: TabDataKey {
    static var defaultValue: TranslateURLBarButton.TranslateState = .unavailable
  }

  var translationState: TranslateURLBarButton.TranslateState {
    get { self[TranslationStateKey.self] }
    set { self[TranslationStateKey.self] = newValue }
  }
}

class TranslateTabHelper: TabObserver {
  private weak var tab: (any TabState)?
  weak var delegate: (any BraveTranslateScriptHandlerDelegate)?
  private let translationControllerDelegate: TranslationControllerDelegate = .init()

  private var translationState: TranslateURLBarButton.TranslateState = .unavailable {
    didSet {
      guard oldValue != translationState, let tab else { return }
      delegate?.updateTranslateURLBar(tab: tab, state: translationState)
    }
  }

  init(tab: some TabState, delegate: any BraveTranslateScriptHandlerDelegate) {
    self.tab = tab
    self.delegate = delegate

    tab.addObserver(self)

    translationControllerDelegate.translateDidBecomeAvailable = { [weak self] in
      self?.translateDidBecomeAvailable()
    }
    translationControllerDelegate.didStartTranslation = { [weak self] from, to in
      guard let self, let tab = self.tab else { return }
      self.delegate?.presentTranslateToast(
        tab: tab,
        languageInfo: .init(
          currentLanguage: .init(identifier: to),
          pageLanguage: .init(identifier: from)
        )
      )
    }
    translationControllerDelegate.didFinishTranslation = { [weak self] error in
      guard let self, let tab = self.tab else { return }
      if error != nil {
        self.delegate?.presentTranslateError(tab: tab)
        translationState = .available
      } else {
        translationState = .active
      }
    }
  }

  deinit {
    tab?.removeObserver(self)
  }

  private func translateDidBecomeAvailable() {
    guard let tab else { return }
    translationState = .available
    let canShowOnboarding = delegate?.canShowTranslateOnboarding(tab: tab) ?? false
    if canShowOnboarding {
      delegate?.showTranslateOnboarding(tab: tab) { [weak self] translateEnabled in
        self?.startTranslation()
      }
    }
  }

  /// Toggles translation on or off. If the page is currently translated, reverts it;
  /// otherwise initiates translation.
  func toggleTranslation() {
    if translationState == .active {
      revertTranslation()
    } else {
      startTranslation()
    }
  }

  /// Initiates translation of the current page from the page language to the users device language.
  ///
  /// Pass in custom language info if you want to translate a specific language pairing. Does
  /// nothing if a translation is already running.
  func startTranslation(with languageInfo: BraveTranslateLanguageInfo? = nil) {
    guard let tab, let translationController = BraveWebView.from(tab: tab)?.translationController,
      tab.translationState != .pending
    else { return }

    var fromLanguage = translationControllerDelegate.pageLanguage
    var toLanguage = translationControllerDelegate.userLanguage

    if let languageInfo {
      fromLanguage = translationController.supportedLanguages.first(where: {
        $0.languageCode == languageInfo.pageLanguage?.languageCode?.identifier
      })
      toLanguage = translationController.supportedLanguages.first(where: {
        $0.languageCode == languageInfo.currentLanguage.languageCode?.identifier
      })
    }

    guard let fromLanguage, let toLanguage else {
      delegate?.presentTranslateError(tab: tab)
      return
    }

    tab.translationState = .pending
    translationController.translatePage(from: fromLanguage, to: toLanguage, userInitiated: true)
  }

  /// Reverts the page to its original language. Does nothing if the tab's translation state is
  /// not `.active`.
  func revertTranslation() {
    guard let tab, let translationController = BraveWebView.from(tab: tab)?.translationController,
      tab.translationState == .active
    else { return }
    translationController.revertTranslation()
    translationState = .available
  }

  // MARK: - TabObserver

  func tabDidCreateWebView(_ tab: some TabState) {
    guard let webView = BraveWebView.from(tab: tab) else { return }
    webView.translationController.delegate = translationControllerDelegate
  }

  func tabDidStartNavigation(_ tab: some TabState) {
    translationState = .unavailable
    translationControllerDelegate.resetCachedLangugaes()
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  private class TranslationControllerDelegate: NSObject, CWVTranslationControllerDelegate {
    var translateDidBecomeAvailable: (() -> Void)?
    var didStartTranslation: ((String, String) -> Void)?
    var didFinishTranslation: ((Error?) -> Void)?

    private(set) var pageLanguage: CWVTranslationLanguage?
    private(set) var userLanguage: CWVTranslationLanguage?

    func resetCachedLangugaes() {
      pageLanguage = nil
      userLanguage = nil
    }

    func translationController(
      _ controller: CWVTranslationController,
      canOfferTranslationFrom pageLanguage: CWVTranslationLanguage,
      to userLanguage: CWVTranslationLanguage
    ) {
      self.pageLanguage = pageLanguage
      self.userLanguage = userLanguage
      translateDidBecomeAvailable?()
    }

    func translationController(
      _ controller: CWVTranslationController,
      didStartTranslationFrom sourceLanguage: CWVTranslationLanguage,
      to targetLanguage: CWVTranslationLanguage,
      userInitiated: Bool
    ) {
      didStartTranslation?(sourceLanguage.languageCode, targetLanguage.languageCode)
    }

    func translationController(
      _ controller: CWVTranslationController,
      didFinishTranslationFrom sourceLanguage: CWVTranslationLanguage,
      to targetLanguage: CWVTranslationLanguage,
      error: (any Error)?
    ) {
      // Always revert the translation policy for the target language. Calling TranslatePage in
      // Chromium with a target language that does not match the expected user language (i.e.
      // the user explicitly chooses a target language) will automatically add said target language
      // to the list of languages blocked from offering automatic translation (required for getting
      // the `translationController(_:canOfferTranslationFrom:to:)` delegate call on page load.
      //
      // If we ever decide to offer translate regardless (which would mean
      // `CWVTranslationController.requestTranslationOffer` directly on some user action) or add
      // some UI that allows the user to manage their Chromium translate prefs then we could remove
      // this in the future.
      controller.setTranslationPolicy(.translationPolicyAsk(), forPageLanguage: targetLanguage)

      didFinishTranslation?(error)
    }
  }
}
