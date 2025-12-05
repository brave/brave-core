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
public class AIChatWebUIHelper: NSObject, TabObserver, AIChatUIHandler,
  AIChatAssociatedContentPageFetcher
{
  private(set) weak var tab: (any TabState)?
  public var handler: ((any TabState, AIChatWebUIPageAction) -> Void)?
  public weak var associatedTab: (any TabState)?
  public var tabsForPrivateMode: (_ isPrivate: Bool) -> [any TabState] = { _ in [] }

  public init?(
    tab: some TabState,
    webDelegate: AIChatWebDelegate?,
    braveTalkJavascript: AIChatBraveTalkJavascript?
  ) {
    if !tab.isChromiumTab || !FeatureList.kAIChatWebUIEnabled.enabled {
      return nil
    }
    self.tab = tab
    self.webDelegate = webDelegate
    self.braveTalkJavascript = braveTalkJavascript
    super.init()
    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  private weak var webDelegate: AIChatWebDelegate?
  private weak var braveTalkJavascript: AIChatBraveTalkJavascript?

  // MARK: - TabObserver

  public func tabDidCreateWebView(_ tab: some TabState) {
    BraveWebView.from(tab: tab)?.aiChatUIHandler = self
  }

  public func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  // MARK: - AIChatUIHandler

  public func webViewForAssociatedContent() -> BraveWebView? {
    if let tab = associatedTab, tab.isChromiumTab {
      if !tab.isWebViewCreated {
        tab.createWebView()
      }
      return BraveWebView.from(tab: tab)
    }
    return nil
  }

  public func webViewForTab(withSessionID id: Int32) -> BraveWebView? {
    guard let tab else { return nil }
    let tabs = tabsForPrivateMode(tab.isPrivate).compactMap { BraveWebView.from(tab: $0) }
    return tabs.first(where: { $0.uniqueSessionID == id })
  }

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

  // MARK: - AIChatAssociatedContentPageFetcher

  @MainActor
  public func fetchPageContent() async -> (String?, Bool) {  // (content: String?, isVideo: Bool)
    guard let webDelegate else {
      return (nil, false)
    }

    if let transcript = await braveTalkJavascript?.getTranscript() {
      return (transcript, false)
    }

    if await webDelegate.getPageContentType() == "application/pdf" {
      if let base64EncodedPDF = await webDelegate.getPDFDocument() {
        return (await AIChatPDFRecognition.parse(pdfData: base64EncodedPDF), false)
      }

      // Attempt to parse the page as a PDF/Image
      guard let pdfData = await webDelegate.getPrintViewPDF() else {
        return (nil, false)
      }
      return (await AIChatPDFRecognition.parseToImage(pdfData: pdfData), false)
    }

    // Fetch regular page content
    let text = await webDelegate.getMainArticle()
    if let text = text, !text.isEmpty {
      return (text, false)
    }

    // No article text. Attempt to parse the page as a PDF/Image
    guard let pdfData = await webDelegate.getPrintViewPDF() else {
      return (nil, false)
    }
    return (await AIChatPDFRecognition.parseToImage(pdfData: pdfData), false)
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
