// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Preferences
import Shared
import SwiftUI
import WebKit

public enum AIChatModelKey: String {
  case chatBasic = "chat-basic"
  case chatExpanded = "chat-leo-expanded"
  case chatClaudeHaiku = "chat-claude-haiku"
  case chatClaudeSonnet = "chat-claude-sonnet"
}

public class AIChatViewModel: NSObject, ObservableObject {
  // TODO(petemill): AIChatService, ConversationHandler refs should
  // be passed directly to this object instead of proxied through the
  // AIChat class.
  private var api: AIChat!
  private weak var webView: WKWebView?
  private let script: any AIChatJavascript.Type
  private let braveTalkScript: AIChatBraveTalkJavascript?
  var querySubmited: String?

  @Published var siteInfo: AiChat.SiteInfo?
  @Published var _shouldSendPageContents: Bool = true
  @Published var canShowPremiumPrompt: Bool = false
  @Published var premiumStatus: AiChat.PremiumStatus = .inactive
  @Published var suggestedQuestions: [String] = []
  @Published var suggestionsStatus: AiChat.SuggestionGenerationStatus = .none
  @Published var conversationHistory: [AiChat.ConversationTurn] = []
  @Published var models: [AiChat.Model] = []
  @Published var currentModel: AiChat.Model?

  @Published var requestInProgress: Bool = false
  @Published var apiError: AiChat.APIError = .none

  public var slashActions: [AiChat.ActionGroup] {
    return api.slashActions
  }

  public var isCurrentModelPremium: Bool {
    guard let currentModel = currentModel else { return false }

    return currentModel.options.tag == .leoModelOptions
      && currentModel.options.leoModelOptions?.access == .premium
  }

  public var isContentAssociationPossible: Bool {
    return webView?.url?.isWebPage(includeDataURIs: true) == true
  }

  public var shouldSendPageContents: Bool {
    get {
      return _shouldSendPageContents
    }

    set {
      objectWillChange.send()
      api.setShouldSendPageContents(newValue)
    }
  }

  public var shouldShowPremiumPrompt: Bool {
    get {
      return canShowPremiumPrompt
    }

    set {  // swiftlint:disable:this unused_setter_value
      self.canShowPremiumPrompt = newValue
      if !newValue {
        api.dismissPremiumPrompt()
      }
    }
  }

  public var shouldShowTermsAndConditions: Bool {
    !self.conversationHistory.isEmpty && !api.isAgreementAccepted
  }

  public var shouldShowSuggestions: Bool {
    self.apiError == .none && api.isAgreementAccepted && self.shouldSendPageContents
      && (!self.suggestedQuestions.isEmpty || self.suggestionsStatus == .canGenerate
        || self.suggestionsStatus == .isGenerating)
  }

  public var shouldShowGenerateSuggestionsButton: Bool {
    self.suggestionsStatus == .canGenerate || self.suggestionsStatus == .isGenerating
  }

  public var isAgreementAccepted: Bool {
    get {
      return api.isAgreementAccepted
    }

    set {
      objectWillChange.send()
      api.isAgreementAccepted = newValue
    }
  }

  public var defaultAIModelKey: String {
    get {
      return api.defaultModelKey
    }

    set {
      objectWillChange.send()
      api.defaultModelKey = newValue
    }
  }

  public init(
    braveCore: BraveCoreMain,
    webView: WKWebView?,
    script: any AIChatJavascript.Type,
    braveTalkScript: AIChatBraveTalkJavascript?,
    querySubmited: String? = nil
  ) {
    self.webView = webView
    self.script = script
    self.braveTalkScript = braveTalkScript
    self.querySubmited = querySubmited

    super.init()

    // Initialize
    api = braveCore.aiChatAPI(with: self)
  }

  // MARK: - API

  func changeModel(modelKey: String) {
    api.changeModel(modelKey)
  }

  func generateSuggestions() {
    if self.suggestionsStatus != .isGenerating && self.suggestionsStatus != .hasGenerated {
      objectWillChange.send()
      api.generateQuestions()
    }
  }

  func summarizePage() {
    api.submitSummarizationRequest()
  }

  func submitSuggestion(_ suggestion: String) {
    apiError = .none
    submitQuery(suggestion)
  }

  func submitQuery(_ text: String) {
    apiError = .none
    api.submitHumanConversationEntry(text)
  }

  func submitSelectedText(_ text: String, action: AiChat.ActionType) {
    apiError = .none
    api.submitSelectedText(text, actionType: action)
  }

  func retryLastRequest() {
    if !self.conversationHistory.isEmpty {
      api.retryAPIRequest()
    }
  }

  @MainActor
  func clearConversationHistory() async {
    api.createNewConversation()
    await self.getInitialState()
  }

  @MainActor
  func clearAndResetData() async {
    await self.clearConversationHistory()
    api.isAgreementAccepted = false
  }

  @MainActor
  func refreshPremiumStatus() async {
    self.premiumStatus = await api.premiumStatus()
  }

  @MainActor
  func getInitialState() async {
    let state = await api.state()
    self.requestInProgress = state.isRequestInProgress
    self.suggestedQuestions = state.suggestedQuestions
    self.suggestionsStatus = state.suggestionStatus
    self.siteInfo = state.associatedContentInfo
    self._shouldSendPageContents = state.shouldSendContent
    self.apiError = state.error
    self.models = state.allModels

    self.currentModel = self.models.first(where: { $0.key == state.currentModelKey })
    self.conversationHistory = api.conversationHistory
  }

  @MainActor
  func clearErrorAndGetFailedMessage() async -> AiChat.ConversationTurn? {
    return await api.clearErrorAndGetFailedMessage()
  }

  @MainActor
  func rateConversation(isLiked: Bool, turnId: UInt) async -> String? {
    return await api.rateMessage(isLiked, turnId: turnId)
  }

  @MainActor
  func submitFeedback(category: String, feedback: String, ratingId: String) async -> Bool {
    // TODO: Add UI for `sendPageURL`
    return await api.sendFeedback(
      category,
      feedback: feedback,
      ratingId: ratingId,
      sendPageUrl: false
    )
  }

  @MainActor
  func modifyConversation(turnId: UInt, newText: String) {
    api.modifyConversation(turnId, newText: newText)
  }
}

extension AIChatViewModel: AIChatDelegate {
  public func getPageTitle() -> String? {
    guard let webView = webView else {
      return nil
    }

    // Return the Page Title
    if let title = webView.title, !title.isEmpty {
      return title
    }

    guard let url = getLastCommittedURL() else {
      return nil
    }

    // Return the URL domain/host
    if url.pathExtension.isEmpty {
      return URLFormatter.formatURLOrigin(
        forDisplayOmitSchemePathAndTrivialSubdomains: url.absoluteString
      )
    }

    // Return the file name with extension
    return url.lastPathComponent
  }

  public func getLastCommittedURL() -> URL? {
    if let url = webView?.url {
      return InternalURL.isValid(url: url) ? nil : url
    }
    return nil
  }

  @MainActor
  public func pageContent() async -> (String?, Bool) {
    guard let webView = webView else {
      return (nil, false)
    }

    requestInProgress = true
    defer { requestInProgress = self.requestInProgress }

    if let transcript = await braveTalkScript?.getTranscript() {
      return (transcript, false)
    }

    if await script.getPageContentType(webView: webView) == "application/pdf" {
      if let base64EncodedPDF = await script.getPDFDocument(webView: webView) {
        return (await AIChatPDFRecognition.parse(pdfData: base64EncodedPDF), false)
      }

      // Attempt to parse the page as a PDF/Image
      let pdfData = await script.getPrintViewPDF(webView: webView)
      return (await AIChatPDFRecognition.parseToImage(pdfData: pdfData), false)
    }

    // Fetch regular page content
    let text = await script.getMainArticle(webView: webView)
    if let text = text, !text.isEmpty {
      return (text, false)
    }

    // No article text. Attempt to parse the page as a PDF/Image
    let pdfData = await script.getPrintViewPDF(webView: webView)
    return (await AIChatPDFRecognition.parseToImage(pdfData: pdfData), false)
  }

  public func isDocumentOnLoadCompletedInPrimaryFrame() -> Bool {
    return webView?.isLoading == false
  }

  public func onHistoryUpdate() {
    self.conversationHistory = api.conversationHistory
  }

  public func onAPIRequest(inProgress: Bool) {
    self.requestInProgress = inProgress
  }

  public func onAPIResponseError(_ error: AiChat.APIError) {
    self.apiError = error
  }

  public func onSuggestedQuestionsChanged(
    _ questions: [String],
    status: AiChat.SuggestionGenerationStatus
  ) {
    self.suggestedQuestions = questions
    self.suggestionsStatus = status
  }

  public func onModelChanged(_ modelKey: String, modelList: [AiChat.Model]) {
    self.currentModel = self.models.first(where: { $0.key == modelKey })
    self.models = modelList
  }

  public func onPageHasContent(
    _ siteInfo: AiChat.SiteInfo,
    shouldSendContent shouldSendPageContents: Bool
  ) {
    self.siteInfo = siteInfo
    self._shouldSendPageContents = shouldSendPageContents
  }
}
