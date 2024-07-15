// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Preferences
import Shared
import SpeechRecognition
import SwiftUI
import os.log

public struct AIChatView: View {
  @ObservedObject
  var model: AIChatViewModel

  @ObservedObject
  var speechRecognizer: SpeechRecognizer

  @State
  private var prompt: String = ""

  @Environment(\.dismiss)
  private var dismiss

  @Namespace
  private var lastMessageId

  @State
  private var customFeedbackInfo: AIChatFeedbackModelToast?

  @State
  private var isPremiumPaywallPresented = false

  @State
  private var isAdvancedSettingsPresented = false

  @State
  private var feedbackToast: AIChatFeedbackToastType = .none

  @State
  private var isShowingSlashTools = false

  @State
  private var slashToolsOption: (group: AiChat.ActionGroup, entry: AiChat.ActionEntry)?

  @ObservedObject
  private var hasSeenIntro = Preferences.AIChat.hasSeenIntro

  @ObservedObject
  private var shouldShowFeedbackPremiumAd = Preferences.AIChat.showPremiumFeedbackAd

  var openURL: ((URL) -> Void)

  public init(
    model: AIChatViewModel,
    speechRecognizer: SpeechRecognizer,
    openURL: @escaping (URL) -> Void
  ) {
    self.model = model
    self.speechRecognizer = speechRecognizer
    self.openURL = openURL
  }

  public var body: some View {
    VStack(spacing: 0.0) {
      AIChatNavigationView(
        isMenusAvailable: hasSeenIntro.value && model.isAgreementAccepted,
        premiumStatus: model.premiumStatus,
        onClose: {
          if !model.isAgreementAccepted {
            hasSeenIntro.value = false
          }

          dismiss()
        },
        onErase: {
          model.clearConversationHistory()
        },
        menuContent: {
          menuView
        }
      )

      Color(braveSystemName: .dividerSubtle)
        .frame(height: 1.0)

      VStack(spacing: 0.0) {
        if hasSeenIntro.value && !model.isAgreementAccepted {
          AIChatTermsAndConditionsView(
            termsAndConditionsAccepted: $model.isAgreementAccepted
          )
        } else {
          GeometryReader { geometry in
            ScrollViewReader { scrollViewReader in
              ScrollView {
                if hasSeenIntro.value {
                  VStack(spacing: 0.0) {
                    if model.shouldShowPremiumPrompt {
                      AIChatPremiumUpsellView(
                        upsellType: model.apiError == .rateLimitReached ? .rateLimit : .premium,
                        upgradeAction: {
                          isPremiumPaywallPresented = true
                        },
                        dismissAction: {
                          if model.apiError == .rateLimitReached {
                            if let turn = model.clearErrorAndGetFailedMessage() {
                              prompt = turn.text
                            }
                          } else {
                            model.shouldShowPremiumPrompt = false
                          }
                        }
                      )
                      .padding()
                    } else {
                      AIChatIntroMessageView(model: model.currentModel)
                        .padding()
                        .background(Color(braveSystemName: .containerBackground))

                      ForEach(Array(model.conversationHistory.enumerated()), id: \.offset) {
                        index,
                        turn in
                        if turn.characterType == .human {
                          AIChatUserMessageView(prompt: turn.text)
                            .padding()
                            .background(Color(braveSystemName: .containerBackground))

                          if index == 0, model.shouldSendPageContents,
                            let url = model.getLastCommittedURL()
                          {
                            AIChatPageInfoBanner(url: url, pageTitle: model.getPageTitle() ?? "")
                              .padding([.horizontal, .bottom])
                              .background(Color(braveSystemName: .containerBackground))
                          }

                          // Show loader view before first part of conversation reply
                          if model.apiError == .none, model.requestInProgress,
                            index == model.conversationHistory.count - 1
                          {
                            VStack(alignment: .leading) {
                              AIChatLoaderView()
                            }
                            .padding()
                            .frame(maxWidth: .infinity, alignment: .leading)
                          }
                        } else {
                          AIChatResponseMessageView(
                            turn: turn,
                            isEntryInProgress: index == model.conversationHistory.count - 1
                              && model.requestInProgress
                          )
                          .padding()
                          .background(Color(braveSystemName: .containerBackground))
                          .contextMenu {
                            responseContextMenuItems(for: index, turn: turn)
                          }

                          if let feedbackInfo = customFeedbackInfo, feedbackInfo.turnId == index {
                            feedbackView
                          }
                        }
                      }

                      apiErrorViews(for: model)

                      if model.shouldShowSuggestions && !model.requestInProgress
                        && model.apiError == .none
                      {
                        if model.suggestionsStatus != .isGenerating
                          && !model.suggestedQuestions.isEmpty
                        {
                          AIChatSuggestionsView(
                            geometry: geometry,
                            suggestions: model.suggestedQuestions
                          ) { suggestion in
                            hasSeenIntro.value = true
                            hideKeyboard()
                            model.submitSuggestion(suggestion)
                          }
                          .padding(.horizontal)
                          .padding(.bottom, 8.0)
                          .disabled(
                            model.suggestionsStatus == .isGenerating
                          )
                        }

                        if model.shouldShowGenerateSuggestionsButton {
                          AIChatSuggestionsButton(
                            title: Strings.AIChat.suggestionsGenerationButtonTitle,
                            isLoading: model.suggestionsStatus == .isGenerating
                          ) {
                            hideKeyboard()
                            model.generateSuggestions()
                          }
                          .padding(.horizontal)
                          .frame(maxWidth: .infinity, alignment: .leading)
                          .disabled(
                            model.suggestionsStatus == .isGenerating
                          )
                        }
                      }

                      Color.clear.id(lastMessageId)
                    }
                  }
                  .onChange(of: model.requestInProgress) { _ in
                    scrollViewReader.scrollTo(lastMessageId, anchor: .bottom)
                  }
                  .onChange(of: model.conversationHistory) { _ in
                    scrollViewReader.scrollTo(lastMessageId, anchor: .bottom)
                  }
                  .onChange(of: customFeedbackInfo) { _ in
                    hideKeyboard()
                    withAnimation {
                      scrollViewReader.scrollTo(lastMessageId, anchor: .bottom)
                    }
                  }
                } else {
                  AIChatIntroView(
                    onSummarizePage: model.isContentAssociationPossible
                      && model.shouldSendPageContents
                      ? {
                        hasSeenIntro.value = true
                        model.summarizePage()
                      } : nil
                  )
                  .frame(minHeight: geometry.size.height, alignment: .bottom)
                }
              }
            }
          }
        }

        if !isShowingSlashTools
          && (model.apiError == .none && model.isAgreementAccepted
            || (!hasSeenIntro.value && !model.isAgreementAccepted))
        {
          if model.conversationHistory.isEmpty && model.isContentAssociationPossible {
            AIChatPageContextView(
              isToggleOn: $model.shouldSendPageContents,
              url: model.getLastCommittedURL(),
              pageTitle: model.getPageTitle() ?? ""
            )
            .padding(.horizontal, 8.0)
            .padding(.bottom, 12.0)
            .disabled(model.shouldShowPremiumPrompt || !model.isContentAssociationPossible)
          }
        }
      }
      .overlay(alignment: .bottom) {
        if isShowingSlashTools {
          AIChatSlashToolsView(
            isShowing: $isShowingSlashTools,
            prompt: $prompt,
            slashActions: model.slashActions,
            selectedOption: $slashToolsOption
          )
        }
      }

      if model.isAgreementAccepted || (!hasSeenIntro.value && !model.isAgreementAccepted) {
        AIChatPromptInputView(
          prompt: $prompt,
          speechRecognizer: speechRecognizer,
          isShowingSlashTools: $isShowingSlashTools,
          slashToolsOption: $slashToolsOption
        ) { prompt in
          hasSeenIntro.value = true

          if let actionType = slashToolsOption?.entry.details?.type {
            model.submitSelectedText(
              prompt,
              action: actionType
            )

            slashToolsOption = nil
            isShowingSlashTools = false
          } else {
            model.submitQuery(prompt)
          }

          hideKeyboard()
        }
        .disabled(
          model.requestInProgress || model.suggestionsStatus == .isGenerating
            || model.apiError == .contextLimitReached
        )
        .padding([.horizontal, .bottom], 8.0)
      }
    }
    .background(Color(braveSystemName: .containerBackground))
    .sheet(isPresented: $isPremiumPaywallPresented) {
      AIChatPaywallView(
        premiumUpgrageSuccessful: { _ in
          Task { @MainActor in
            await model.refreshPremiumStatus()
          }
        },
        refreshCredentials: {
          openURL(.brave.braveLeoRefreshCredentials)
          dismiss()
        }
      )
    }
    .sheet(isPresented: $isAdvancedSettingsPresented) {
      AIChatAdvancedSettingsView(
        model: model,
        isModallyPresented: true
      )
    }
    .onAppear {
      Task { @MainActor in
        await model.refreshPremiumStatus()
      }

      if let query = model.querySubmited {
        model.querySubmited = nil
        hasSeenIntro.value = true
        model.submitQuery(query)
      }
    }
    .toastView($feedbackToast)
    .environment(
      \.openURL,
      OpenURLAction { url in
        openURL(url)
        dismiss()
        return .handled
      }
    )
  }

  private var menuView: some View {
    ScrollView {
      AIChatMenuView(
        premiumStatus: model.premiumStatus,
        currentModel: model.currentModel,
        modelOptions: model.models,
        onModelChanged: { modelKey in
          model.changeModel(modelKey: modelKey)
        },
        onOptionSelected: { option in
          switch option {
          case .newChat:
            model.clearConversationHistory()
            break

          case .goPremium:
            isPremiumPaywallPresented = true

          case .managePremium:
            // The purchase was through the AppStore
            if BraveStoreSDK.shared.leoSubscriptionStatus?.state != nil {
              guard let url = URL.apple.manageSubscriptions else {
                return
              }

              // Opens Apple's "manage subscription" screen
              if UIApplication.shared.canOpenURL(url) {
                UIApplication.shared.open(url, options: [:])
              }
            } else {
              // The purchase was through the Brave Account Website
              if BraveStoreSDK.shared.environment != .production {
                openURL(.brave.braveLeoManageSubscriptionStaging)
              } else {
                openURL(.brave.braveLeoManageSubscriptionProd)
              }
            }
            dismiss()

          case .advancedSettings:
            isAdvancedSettingsPresented.toggle()
          }
        }
      )
      .frame(minWidth: min(300.0, UIScreen.main.bounds.width))
    }
  }

  @ViewBuilder
  private func responseContextMenuItems(
    for turnIndex: Int,
    turn: AiChat.ConversationTurn
  ) -> some View {
    AIChatResponseMessageViewContextMenuButton(
      title: Strings.AIChat.responseContextMenuRegenerateTitle,
      icon: Image(braveSystemName: "leo.refresh"),
      onSelected: {
        if turnIndex == model.conversationHistory.count - 1 {
          model.retryLastRequest()
        } else if let query = model.conversationHistory[safe: turnIndex - 1]?.text {
          model.submitQuery(query)
        }
      }
    )

    AIChatResponseMessageViewContextMenuButton(
      title: Strings.AIChat.responseContextMenuCopyTitle,
      icon: Image(braveSystemName: "leo.copy"),
      onSelected: {
        UIPasteboard.general.setValue(turn.text, forPasteboardType: "public.plain-text")
      }
    )

    AIChatResponseMessageViewContextMenuButton(
      title: Strings.AIChat.responseContextMenuLikeAnswerTitle,
      icon: Image(braveSystemName: "leo.thumb.up"),
      onSelected: {
        Task { @MainActor in
          let ratingId = await model.rateConversation(isLiked: true, turnId: UInt(turnIndex))
          if ratingId != nil {
            feedbackToast = .success(isLiked: true)
          } else {
            feedbackToast = .error(message: Strings.AIChat.rateAnswerActionErrorText)
          }
        }
      }
    )

    AIChatResponseMessageViewContextMenuButton(
      title: Strings.AIChat.responseContextMenuDislikeAnswerTitle,
      icon: Image(braveSystemName: "leo.thumb.down"),
      onSelected: {
        Task { @MainActor in
          let ratingId = await model.rateConversation(isLiked: false, turnId: UInt(turnIndex))
          if let ratingId = ratingId {
            feedbackToast = .success(
              isLiked: false,
              onAddFeedback: {
                customFeedbackInfo = AIChatFeedbackModelToast(turnId: turnIndex, ratingId: ratingId)
              }
            )
          } else {
            feedbackToast = .error(message: Strings.AIChat.rateAnswerActionErrorText)
          }
        }
      }
    )
  }

  @ViewBuilder
  private func apiErrorViews(for model: AIChatViewModel) -> some View {
    let isPremiumAccess = model.currentModel.access == .premium
    let isPremium = model.premiumStatus == .active
    let isPremiumDisconnected = model.premiumStatus == .activeDisconnected

    if isPremiumAccess && isPremiumDisconnected {
      if let subscriptionState = BraveStoreSDK.shared.leoSubscriptionStatus?.state,
        subscriptionState != .expired && subscriptionState != .revoked
      {
        // Purchased via AppStore
        AIChatBusyErrorView {
          Task { @MainActor in
            if let orderId = Preferences.AIChat.subscriptionOrderId.value {
              try? await BraveSkusSDK.shared.fetchCredentials(orderId: orderId, for: .leo)
            }

            model.retryLastRequest()
          }
        }
      } else {
        // Purchased via Desktop
        AIChatSessionExpiredErrorView {
          openURL(URL.Brave.braveLeoRefreshCredentials)
          dismiss()
        }
      }
    } else if isPremiumAccess && !isPremium {
      AIChatPremiumUpsellView(
        upsellType: .premium,
        upgradeAction: {
          isPremiumPaywallPresented = true
        },
        dismissAction: {
          Task { @MainActor in
            await model.refreshPremiumStatus()

            if let turn = model.clearErrorAndGetFailedMessage() {
              prompt = turn.text
            }
          }
        }
      )
      .padding()
    } else {
      switch model.apiError {
      case .connectionIssue:
        AIChatNetworkErrorView {
          if !model.conversationHistory.isEmpty {
            model.retryLastRequest()
          }
        }
        .padding()
      case .rateLimitReached:
        AIChatPremiumUpsellView(
          upsellType: .rateLimit,
          upgradeAction: {
            isPremiumPaywallPresented = true
          },
          dismissAction: {
            Task { @MainActor in
              await model.refreshPremiumStatus()

              if let turn = model.clearErrorAndGetFailedMessage() {
                prompt = turn.text
              }
            }
          }
        )
        .padding()
      case .contextLimitReached:
        AIChatContextLimitErrorView {
          model.clearConversationHistory()
        }
        .padding()
      case .none:
        EmptyView()
      @unknown default:
        EmptyView()
      }
    }
  }

  private var feedbackView: some View {
    AIChatFeedbackView(
      speechRecognizer: speechRecognizer,
      premiumStatus: model.premiumStatus,
      shouldShowPremiumAd: shouldShowFeedbackPremiumAd.value,
      onSubmit: { category, feedback in
        guard let feedbackInfo = customFeedbackInfo else {
          feedbackToast = .error(message: Strings.AIChat.feedbackSubmittedErrorTitle)
          return
        }

        Task { @MainActor in
          let success = await model.submitFeedback(
            category: category,
            feedback: feedback,
            ratingId: feedbackInfo.ratingId
          )

          feedbackToast =
            success
            ? .submitted : .error(message: Strings.AIChat.feedbackSubmittedErrorTitle)
        }

        customFeedbackInfo = nil
      },
      onCancel: {
        customFeedbackInfo = nil
        feedbackToast = .none
      }
    )
    .padding()
    .environment(
      \.openURL,
      OpenURLAction { url in
        if url.host == "dismiss" {
          shouldShowFeedbackPremiumAd.value = false
        } else {
          openURL(url)
          dismiss()
        }
        return .handled
      }
    )
  }

  private func hideKeyboard() {
    UIApplication.shared.sendAction(
      #selector(UIResponder.resignFirstResponder),
      to: nil,
      from: nil,
      for: nil
    )
  }
}

#if DEBUG
struct AIChatView_Preview: PreviewProvider {
  static var previews: some View {
    return VStack(spacing: 0.0) {
      AIChatNavigationView(
        isMenusAvailable: true,
        premiumStatus: .active,
        onClose: {
          print("Closed Chat")
        },
        onErase: {
          print("Erased Chat History")
        },
        menuContent: {
          EmptyView()
        }
      )

      Color(braveSystemName: .dividerSubtle)
        .frame(height: 1.0)

      GeometryReader { geometry in
        ScrollView {
          VStack(spacing: 0.0) {
            AIChatUserMessageView(prompt: "Does it work with Apple devices?")
              .padding()
              .background(Color(braveSystemName: .pageBackground))

            AIChatPageInfoBanner(
              url: nil,
              pageTitle:
                "Sonos Era 300 and Era 100...'s Editors’Choice Awards: The Best AIs and Services for 2023"
            )
            .padding([.horizontal, .bottom])
            .background(Color(braveSystemName: .pageBackground))

            AIChatResponseMessageView(
              turn:
                .init(
                  characterType: .assistant,
                  actionType: .response,
                  visibility: .visible,
                  text:
                    "After months of leaks and some recent coordinated teases from the company itself, Sonos is finally officially announcing the Era 300 and Era 100 speakers. Both devices go up for preorder today — the Era 300 costs $449 and the Era 100 is $249 — and they’ll be available to purchase in stores beginning March 28th.\n\nAs its unique design makes clear, the Era 300 represents a completely new type of speaker for the company; it’s designed from the ground up to make the most of spatial audio music and challenge competitors like the HomePod and Echo Studio.",
                  selectedText: nil,
                  events: nil
                ),
              isEntryInProgress: false
            )
            .padding()
            .background(Color(braveSystemName: .containerBackground))

            AIChatFeedbackView(
              speechRecognizer: SpeechRecognizer(),
              premiumStatus: .inactive,
              shouldShowPremiumAd: true,
              onSubmit: {
                print("Submitted Feedback: \($0) -- \($1)")
              },
              onCancel: {
                print("Cancelled Feedback")
              }
            )
            .padding()

            AIChatSuggestionsView(
              geometry: geometry,
              suggestions: [
                "What Bluetooth version does it use?",
                "Summarize this page?",
                "What is Leo?",
                "What can the Leo assistant do for me?",
              ]
            )
            .padding()
          }
        }
        .frame(maxHeight: geometry.size.height)
      }

      Spacer()

      AIChatPageContextView(
        isToggleOn: .constant(true),
        url: URL(string: "https://brave.com"),
        pageTitle: "Brave Private Browser"
      )
      .padding()

      AIChatPromptInputView(
        prompt: .constant(""),
        speechRecognizer: SpeechRecognizer(),
        isShowingSlashTools: .constant(false),
        slashToolsOption: .constant(nil)
      ) {
        print("Prompt Submitted: \($0)")
      }
    }
    .background(Color(braveSystemName: .containerBackground))
    .previewLayout(.sizeThatFits)
  }
}
#endif
