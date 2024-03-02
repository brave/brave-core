// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import WebKit
import BraveCore
import Shared

public class AIChatViewModel: NSObject, ObservableObject {
  private var api: AIChat!
  private let webView: WKWebView?
  private let script: any AIChatJavascript.Type
  var querySubmited: String?
  
  @Published var siteInfo: AiChat.SiteInfo?
  @Published var premiumStatus: AiChat.PremiumStatus = .inactive
  @Published var suggestedQuestions: [String] = []
  @Published var conversationHistory: [AiChat.ConversationTurn] = []
  @Published var models: [AiChat.Model] = []
  @Published var currentModel: AiChat.Model!
  
  @Published var requestInProgress: Bool = false
  @Published var apiError: AiChat.APIError = .none
   
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
    api.currentAPIError == .none &&
    api.isAgreementAccepted &&
    api.shouldSendPageContents && (
      !api.suggestedQuestions.isEmpty ||
      api.suggestionsStatus == .canGenerate ||
      api.suggestionsStatus == .isGenerating
    )
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
  
  public init(braveCore: BraveCoreMain,
              webView: WKWebView?,
              script: any AIChatJavascript.Type,
              querySubmited: String? = nil) {
    self.webView = webView
    self.script = script
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
    
    Task { @MainActor in
      await getPremiumStatus()
    }
    
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
  
  func retryLastRequest() {
    if !conversationHistory.isEmpty {
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
  @discardableResult
  func getPremiumStatus() async -> AiChat.PremiumStatus {
    return await withCheckedContinuation { continuation in
      api.getPremiumStatus { status in
        self.premiumStatus = status
        continuation.resume(returning: status)
      }
    }
  }
  
  @MainActor
  func rateConversation(isLiked: Bool, turnId: UInt) async -> String? {
    return await withCheckedContinuation { continuation in
      api.rateMessage(isLiked, turnId: turnId, completion: { identifier in
        continuation.resume(returning: identifier)
      })
    }
  }
  
  @MainActor
  func submitFeedback(category: String, feedback: String, ratingId: String) async -> Bool {
    // TODO: Add UI for `sendPageURL`
    await api.sendFeedback(category, feedback: feedback, ratingId: ratingId, sendPageUrl: false)
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
      return URLFormatter.formatURLOrigin(forDisplayOmitSchemePathAndTrivialSubdomains: url.absoluteString)
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
  
  public func getPageContent(completion: @escaping (String?, Bool) -> Void) {
    guard let webView = webView else {
      completion(nil, false)
      return
    }
    
    requestInProgress = true
    
    Task { @MainActor in
      defer { requestInProgress = false }
      
      if await script.getPageContentType(webView: webView) == "application/pdf" {
        if let base64EncodedPDF = await script.getPDFDocument(webView: webView) {
          completion(await AIChatPDFRecognition.parse(pdfData: base64EncodedPDF), false)
          return
        }
        
        // Could not fetch PDF for parsing
        completion(nil, false)
        return
      }
      
      // Fetch regular page content
      let text = await script.getMainArticle(webView: webView)
      if let text = text, !text.isEmpty {
        completion(text, false)
        return
      }
      
      // No article text. Attempt to parse the page as a PDF/Image
      let render = UIPrintPageRenderer()
      render.addPrintFormatter(webView.viewPrintFormatter(), startingAtPageAt: 0)
      
      let page = CGRect(x: 0, y: 0, width: 595.2, height: 841.8) // A4, 72 dpi
      let printable = page.insetBy(dx: 0, dy: 0)
      
      render.setValue(NSValue(cgRect: page), forKey: "paperRect")
      render.setValue(NSValue(cgRect: printable), forKey: "printableRect")
      
      // 4. Create PDF context and draw
      let pdfData = NSMutableData()
      UIGraphicsBeginPDFContextToData(pdfData, CGRect.zero, nil)
      for i in 0..<render.numberOfPages {
        UIGraphicsBeginPDFPage()
        let bounds = UIGraphicsGetPDFContextBounds()
        render.drawPage(at: i, in: bounds)
      }
      
      UIGraphicsEndPDFContext()
      completion(await AIChatPDFRecognition.parseToImage(pdfData: pdfData.base64EncodedString()), false)
    }
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
  
  public func onSuggestedQuestionsChanged(_ questions: [String], status: AiChat.SuggestionGenerationStatus) {
    self.suggestedQuestions = questions
  }
  
  public func onModelChanged(_ modelKey: String) {
    self.currentModel = self.models.first(where: { $0.key == modelKey })
  }
  
  public func onPageHasContent(_ siteInfo: AiChat.SiteInfo) {
    objectWillChange.send()
  }
  
  public func onConversationEntryPending() {
    objectWillChange.send()
  }
}
