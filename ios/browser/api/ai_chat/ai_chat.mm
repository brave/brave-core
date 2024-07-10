// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/ai_chat/ai_chat.h"

#include "ai_chat.mojom.objc+private.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "brave/base/mac/conversions.h"
#include "brave/components/ai_chat/core/browser/ai_chat_feedback_api.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/ios/browser/api/ai_chat/conversation_driver_ios.h"
#include "brave/ios/browser/api/ai_chat/model_service_factory.h"
#include "brave/ios/browser/application_context/brave_application_context_impl.h"
#include "brave/ios/browser/skus/skus_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "components/version_info/channel.h"
#include "components/version_info/version_info.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/chrome/common/channel_info.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

@interface AIChat () {
  raw_ptr<ChromeBrowserState> browser_state_;
  std::unique_ptr<ai_chat::AIChatMetrics> ai_chat_metrics_;
  std::unique_ptr<ai_chat::ConversationDriverIOS> driver_;
}
@end

@implementation AIChat
- (instancetype)initWithChromeBrowserState:(ChromeBrowserState*)browserState
                                  delegate:(id<AIChatDelegate>)delegate {
  if ((self = [super init])) {
    browser_state_ = browserState;

    PrefService* local_state_prefs = GetApplicationContext()->GetLocalState();

    ai_chat::ModelService* model_service =
        ai_chat::ModelServiceFactory::GetForBrowserState(browser_state_);

    auto skus_service_getter = base::BindRepeating(
        [](ChromeBrowserState* browser_state) {
          return skus::SkusServiceFactory::GetForBrowserState(browser_state);
        },
        base::Unretained(browser_state_));

    ai_chat_metrics_ =
        std::make_unique<ai_chat::AIChatMetrics>(local_state_prefs);

    BraveApplicationContextImpl* braveContext =
        static_cast<BraveApplicationContextImpl*>(GetApplicationContext());

    driver_ = std::make_unique<ai_chat::ConversationDriverIOS>(
        user_prefs::UserPrefs::Get(browser_state_), local_state_prefs,
        model_service, ai_chat_metrics_.get(),
        braveContext->leo_local_models_updater(), skus_service_getter,
        browser_state_->GetSharedURLLoaderFactory(),
        version_info::GetChannelString(::GetChannel()), delegate);
  }
  return self;
}

- (void)dealloc {
  web::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          ^(decltype(driver_) driver, decltype(ai_chat_metrics_) metrics) {
            driver.reset();
            metrics.reset();
          },
          std::move(driver_), std::move(ai_chat_metrics_)));
}

- (bool)isAgreementAccepted {
  return driver_->HasUserOptedIn();
}

- (void)setIsAgreementAccepted:(bool)accepted {
  driver_->SetUserOptedIn(accepted);
}

- (void)changeModel:(NSString*)modelKey {
  driver_->ChangeModel(base::SysNSStringToUTF8(modelKey));
}

- (AiChatModel*)currentModel {
  return [[AiChatModel alloc] initWithModel:driver_->GetCurrentModel()];
}

- (NSArray<AiChatModel*>*)models {
  const std::vector<ai_chat::mojom::ModelPtr>& models = driver_->GetModels();
  NSMutableArray* result =
      [[NSMutableArray alloc] initWithCapacity:models.size()];

  for (auto& model : models) {
    [result addObject:[[AiChatModel alloc] initWithModelPtr:model->Clone()]];
  }
  return [result copy];
}

- (NSArray<AiChatConversationTurn*>*)conversationHistory {
  NSMutableArray* history = [[NSMutableArray alloc] init];
  for (auto&& turn : driver_->GetVisibleConversationHistory()) {
    [history addObject:[[AiChatConversationTurn alloc]
                           initWithConversationTurnPtr:std::move(turn)]];
  }
  return [history copy];
}

- (void)submitHumanConversationEntry:(NSString*)text {
  driver_->SubmitHumanConversationEntry(ai_chat::mojom::ConversationTurn::New(
      ai_chat::mojom::CharacterType::HUMAN,
      ai_chat::mojom::ActionType::UNSPECIFIED,
      ai_chat::mojom::ConversationTurnVisibility::VISIBLE,
      base::SysNSStringToUTF8(text), std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt));
}

- (void)submitSummarizationRequest {
  driver_->SubmitSummarizationRequest();
}

- (void)setConversationActive:(bool)is_conversation_active {
  driver_->OnConversationActiveChanged(is_conversation_active);
}

- (void)retryAPIRequest {
  driver_->RetryAPIRequest();
}

- (bool)isRequestInProgress {
  return driver_->IsRequestInProgress();
}

- (void)generateQuestions {
  driver_->GenerateQuestions();
}

- (AiChatSuggestionGenerationStatus)suggestionsStatus {
  auto status = ai_chat::mojom::SuggestionGenerationStatus::None;
  driver_->GetSuggestedQuestions(status);
  return (AiChatSuggestionGenerationStatus)status;
}

- (NSArray<AiChatActionGroup*>*)slashActions {
  NSMutableArray* result = [[NSMutableArray alloc] init];
  for (auto&& group : ai_chat::GetActionMenuList()) {
    [result addObject:[[AiChatActionGroup alloc]
                          initWithActionGroupPtr:std::move(group)]];
  }
  return result;
}

- (NSArray<NSString*>*)suggestedQuestions {
  auto status = ai_chat::mojom::SuggestionGenerationStatus::None;
  std::vector<std::string> result = driver_->GetSuggestedQuestions(status);
  return brave::vector_to_ns(result);
}

- (NSInteger)contentUsedPercentage {
  return driver_->GetContentUsedPercentage();
}

- (bool)hasPendingConversationEntry {
  return driver_->HasPendingConversationEntry();
}

- (bool)shouldSendPageContents {
  return driver_->GetShouldSendPageContents();
}

- (void)setShouldSendPageContents:(bool)should_send {
  driver_->SetShouldSendPageContents(should_send);
}

- (NSString*)defaultModelKey {
  return base::SysUTF8ToNSString(driver_->GetDefaultModel());
}

- (void)setDefaultModelKey:(NSString*)modelKey {
  driver_->SetDefaultModel(base::SysNSStringToUTF8(modelKey));
}

- (void)clearConversationHistory {
  driver_->ClearConversationHistory();
}

- (AiChatConversationTurn*)clearErrorAndGetFailedMessage {
  ai_chat::mojom::ConversationTurnPtr turn =
      driver_->ClearErrorAndGetFailedMessage();
  if (turn) {
    return [[AiChatConversationTurn alloc]
        initWithConversationTurnPtr:std::move(turn)];
  }
  return nullptr;
}

- (AiChatAPIError)currentAPIError {
  return static_cast<AiChatAPIError>(driver_->GetCurrentAPIError());
}

- (void)getPremiumStatus:(void (^)(AiChatPremiumStatus))completion {
  driver_->GetPremiumStatus(base::BindOnce(
      [](void (^completion)(AiChatPremiumStatus),
         ai_chat::mojom::PremiumStatus status,
         ai_chat::mojom::PremiumInfoPtr info) {
        if (completion) {
          completion(static_cast<AiChatPremiumStatus>(status));
        }
      },
      completion));
}

- (void)submitSelectedText:(NSString*)selectedText
                actionType:(AiChatActionType)actionType {
  [self submitSelectedText:selectedText
                actionType:actionType
              onSuggestion:nil
               onCompleted:nil];
}

- (void)submitSelectedText:(NSString*)selectedText
                actionType:(AiChatActionType)actionType
              onSuggestion:(void (^)(NSString*))onSuggestion
               onCompleted:(void (^)(NSString* result,
                                     AiChatAPIError error))onCompleted {
  driver_->SubmitSelectedText(
      base::SysNSStringToUTF8(selectedText),
      static_cast<ai_chat::mojom::ActionType>(actionType),
      onSuggestion ? base::BindRepeating(^(const std::string& suggestion_text) {
        onSuggestion(base::SysUTF8ToNSString(suggestion_text));
      })
                   : base::NullCallback(),
      onCompleted
          ? base::BindOnce(^(ai_chat::EngineConsumer::GenerationResult result) {
              if (auto result_string = result; result_string.has_value()) {
                onCompleted(base::SysUTF8ToNSString(result_string.value()),
                            AiChatAPIErrorNone);
              } else {
                onCompleted(nil, static_cast<AiChatAPIError>(result.error()));
              }
            })
          : base::NullCallback());
}

- (void)submitSelectedText:(NSString*)selectedText
                  question:(NSString*)question
                actionType:(AiChatActionType)actionType
              onSuggestion:(void (^)(NSString*))onSuggestion
               onCompleted:(void (^)(NSString* result,
                                     AiChatAPIError error))onCompleted {
  driver_->SubmitSelectedTextWithQuestion(
      base::SysNSStringToUTF8(selectedText), base::SysNSStringToUTF8(question),
      static_cast<ai_chat::mojom::ActionType>(actionType),
      onSuggestion ? base::BindRepeating(^(const std::string& suggestion_text) {
        onSuggestion(base::SysUTF8ToNSString(suggestion_text));
      })
                   : base::NullCallback(),
      onCompleted
          ? base::BindOnce(^(ai_chat::EngineConsumer::GenerationResult result) {
              if (auto result_string = result; result_string.has_value()) {
                onCompleted(base::SysUTF8ToNSString(result_string.value()),
                            AiChatAPIErrorNone);
              } else {
                onCompleted(nil, static_cast<AiChatAPIError>(result.error()));
              }
            })
          : base::NullCallback());
}

- (void)rateMessage:(bool)isLiked
             turnId:(NSUInteger)turnId
         completion:(void (^)(NSString* identifier))completion {
  driver_->RateMessage(
      isLiked, turnId,
      base::BindOnce(
          [](void (^completion)(NSString*),
             const std::optional<std::string>& identifier) {
            if (completion) {
              completion(identifier ? base::SysUTF8ToNSString(*identifier)
                                    : nil);
            }
          },
          completion));
}

- (void)sendFeedback:(NSString*)category
            feedback:(NSString*)feedback
            ratingId:(NSString*)ratingId
         sendPageUrl:(bool)sendPageUrl
          completion:(void (^)(bool))completion {
  driver_->SendFeedback(base::SysNSStringToUTF8(category),
                        base::SysNSStringToUTF8(feedback),
                        base::SysNSStringToUTF8(ratingId), sendPageUrl,
                        base::BindOnce(completion));
}

- (void)modifyConversation:(NSUInteger)turnId newText:(NSString*)newText {
  driver_->ModifyConversation(turnId, base::SysNSStringToUTF8(newText));
}

- (bool)canShowPremiumPrompt {
  return driver_->GetCanShowPremium();
}

- (void)dismissPremiumPrompt {
  driver_->DismissPremiumPrompt();
}
@end
