// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
@_spi(ChromiumWebViewAccess) import Web

public enum AIChatWebUIPageAction {
  public enum FileUploadMode {
    case camera
    case photos
  }
  case handleVoiceRecognitionRequest(_ completion: (String?) -> Void)
  case handleFileUploadRequest(FileUploadMode, _ completion: ([AiChat.UploadedFile]?) -> Void)
  case presentSettings
  case presentPremiumPaywall
  case presentManagePremium
  case closeTab
  case openURL(_ url: URL)
}

/// A tab helper for interacting with Leo AI Chat running in WebUI
public class AIChatWebUIHelper: NSObject, TabObserver, AIChatUIHandler {
  private(set) weak var tab: (any TabState)?
  public var handler: ((any TabState, AIChatWebUIPageAction) -> Void)?

  public init?(tab: some TabState) {
    if !tab.isChromiumTab || !FeatureList.kAIChatWebUIEnabled.enabled {
      return nil
    }
    self.tab = tab
    super.init()
    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  // MARK: - TabObserver

  public func tabDidCreateWebView(_ tab: some TabState) {
    BraveWebView.from(tab: tab)?.aiChatUIHandler = self
  }

  public func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  // MARK: - AIChatUIHandler

  public func handleVoiceRecognitionRequest(_ completion: @escaping (String?) -> Void) {
    guard let tab else {
      completion(nil)
      return
    }
    handler?(tab, .handleVoiceRecognitionRequest(completion))
  }

  public func handleFileUploadRequest(
    _ useMediaCapture: Bool,
    completionHandler completion: @escaping ([AiChat.UploadedFile]?) -> Void
  ) {
    guard let tab else {
      completion(nil)
      return
    }
    handler?(tab, .handleFileUploadRequest(useMediaCapture ? .camera : .photos, completion))
  }

  public func openAIChatSettings() {
    guard let tab else { return }
    handler?(tab, .presentSettings)
  }

  public func goPremium() {
    guard let tab else { return }
    handler?(tab, .presentPremiumPaywall)
  }

  public func managePremium() {
    guard let tab else { return }
    handler?(tab, .presentManagePremium)
  }

  public func closeUI() {
    guard let tab else { return }
    handler?(tab, .closeTab)
  }

  public func open(_ url: URL) {
    guard let tab else { return }
    handler?(tab, .openURL(url))
  }
}

extension TabDataValues {
  private struct AIChatWebUIHelperKey: TabDataKey {
    static var defaultValue: AIChatWebUIHelper? = nil
  }

  public var aiChatWebUIHelper: AIChatWebUIHelper? {
    get { self[AIChatWebUIHelperKey.self] }
    set { self[AIChatWebUIHelperKey.self] = newValue }
  }
}
