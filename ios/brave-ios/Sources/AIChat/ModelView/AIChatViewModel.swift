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
  private var api: AIChat!
  private weak var webView: WKWebView?
  private let script: any AIChatJavascript.Type
  private let braveTalkScript: AIChatBraveTalkJavascript?
  var querySubmited: String?

  @Published var siteInfo: AiChat.SiteInfo?
  @Published var premiumStatus: AiChat.PremiumStatus = .inactive
  @Published var suggestedQuestions: [String] = []
  @Published var conversationHistory: [AiChat.ConversationTurn] = []
  @Published var models: [AiChat.Model] = []
  @Published var currentModel: AiChat.Model!

  @Published var requestInProgress: Bool = false
  @Published var apiError: AiChat.APIError = .none

  public var slashActions: [AiChat.ActionGroup] {
    return api.slashActions
  }

  public var isCurrentModelPremium: Bool {
    currentModel.options.tag == .leoModelOptions
      && currentModel.options.leoModelOptions?.access == .premium
  }

  public var isContentAssociationPossible: Bool {
    return webView?.url?.isWebPage(includeDataURIs: true) == true
  }

  public var shouldSendPageContents: Bool {
    get {
      return api.shouldSendPageContents
    }

    set {
      objectWillChange.send()
      api.shouldSendPageContents = newValue
    }
  }

  public var shouldShowPremiumPrompt: Bool {
    get {
      return api.canShowPremiumPrompt
    }

    set {  // swiftlint:disable:this unused_setter_value
      objectWillChange.send()
      if !newValue {
        api.dismissPremiumPrompt()
      }
    }
  }

  public var shouldShowTermsAndConditions: Bool {
    !api.conversationHistory.isEmpty && !api.isAgreementAccepted
  }

  public var shouldShowSuggestions: Bool {
    api.currentAPIError == .none && api.isAgreementAccepted && api.shouldSendPageContents
      && (!api.suggestedQuestions.isEmpty || api.suggestionsStatus == .canGenerate
        || api.suggestionsStatus == .isGenerating)
  }

  public var shouldShowGenerateSuggestionsButton: Bool {
    api.suggestionsStatus == .canGenerate || api.suggestionsStatus == .isGenerating
  }

  public var suggestionsStatus: AiChat.SuggestionGenerationStatus {
    api.suggestionsStatus
  }

  public var isAgreementAccepted: Bool {
    get {
      return api.isAgreementAccepted
    }

    set {
      objectWillChange.send()
      api.isAgreementAccepted = newValue
      api.setConversationActive(newValue)
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
    currentModel = api.currentModel
    models = api.models
    conversationHistory = api.conversationHistory
    suggestedQuestions = api.suggestedQuestions
    apiError = api.currentAPIError
    requestInProgress = api.isRequestInProgress

    if isAgreementAccepted {
      api.setConversationActive(true)
    }
  }

  // MARK: - API

  func changeModel(modelKey: String) {
    api.changeModel(modelKey)
  }

  func clearConversationHistory() {
    apiError = .none
    api.clearConversationHistory()
  }

  func generateSuggestions() {
    if api.suggestionsStatus != .isGenerating && api.suggestionsStatus != .hasGenerated {
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

  func submitSelectedText(
    _ text: String,
    action: AiChat.ActionType,
    onSuggestion: @escaping (String?) -> Void,
    onCompleted: @escaping (String?, AiChat.APIError) -> Void
  ) {
    apiError = .none
    api.submitSelectedText(
      text,
      actionType: action,
      onSuggestion: onSuggestion,
      onCompleted: onCompleted
    )
  }

  func submitSelectedText(
    _ text: String,
    question: String,
    action: AiChat.ActionType,
    onSuggestion: @escaping (String?) -> Void,
    onCompleted: @escaping (String?, AiChat.APIError) -> Void
  ) {
    apiError = .none
    api.submitSelectedText(
      text,
      question: question,
      actionType: action,
      onSuggestion: onSuggestion,
      onCompleted: onCompleted
    )
  }

  func retryLastRequest() {
    if !api.conversationHistory.isEmpty {
      api.retryAPIRequest()
    }
  }

  func clearAndResetData() {
    apiError = .none
    api.clearConversationHistory()
    api.setConversationActive(false)
    api.isAgreementAccepted = false
  }

  @MainActor
  func refreshPremiumStatus() async {
    self.premiumStatus = await api.premiumStatus()
  }

  @MainActor
  func clearErrorAndGetFailedMessage() -> AiChat.ConversationTurn? {
    api.clearErrorAndGetFailedMessage()
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
    defer { requestInProgress = api.isRequestInProgress }

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
  }

  public func onModelChanged(_ modelKey: String, modelList: [AiChat.Model]) {
    self.currentModel = self.models.first(where: { $0.key == modelKey })
  }

  public func onPageHasContent(_ siteInfo: AiChat.SiteInfo) {
    objectWillChange.send()
  }

  public func onConversationEntryPending() {
    objectWillChange.send()
  }
}
