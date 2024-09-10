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
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/ios/browser/api/ai_chat/ai_chat_service_factory.h"
#include "brave/ios/browser/api/ai_chat/associated_content_driver_ios.h"
#include "brave/ios/browser/api/ai_chat/conversation_client.h"
#include "brave/ios/browser/api/ai_chat/model_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/common/channel_info.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"

@interface AIChat () {
  raw_ptr<ChromeBrowserState> browser_state_;
  raw_ptr<ai_chat::AIChatService> service_;
  raw_ptr<ai_chat::ModelService> model_service_;
  raw_ptr<ai_chat::ConversationHandler> current_conversation_;

  // TODO(petemill): Pass the bindings to the UI ViewModel so we
  // can avoid simply proxying data and events through this class.
  std::unique_ptr<ai_chat::ConversationClient> conversation_client_;

  std::unique_ptr<ai_chat::AssociatedContentDriver> current_content_;

  __weak id<AIChatDelegate> delegate_;
}
@end

@implementation AIChat
- (instancetype)initWithChromeBrowserState:(ChromeBrowserState*)browserState
                                  delegate:(id<AIChatDelegate>)delegate {
  if ((self = [super init])) {
    browser_state_ = browserState;
    delegate_ = delegate;

    model_service_ =
        ai_chat::ModelServiceFactory::GetForBrowserState(browser_state_);
    service_ =
        ai_chat::AIChatServiceFactory::GetForBrowserState(browser_state_);

    current_content_ = std::make_unique<ai_chat::AssociatedContentDriverIOS>(
        browser_state_->GetSharedURLLoaderFactory(), delegate);

    [self createNewConversation];
  }
  return self;
}

- (void)dealloc {
  web::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(
                     ^(decltype(current_content_) current_content) {
                       current_content.reset();
                     },
                     std::move(current_content_)));
}

- (void)createNewConversation {
  current_conversation_ = service_->CreateConversationHandlerForContent(
      current_content_->GetContentId(), current_content_->GetWeakPtr());
  conversation_client_ = std::make_unique<ai_chat::ConversationClient>(
      current_conversation_.get(), delegate_);
}

- (bool)isAgreementAccepted {
  return service_->HasUserOptedIn();
}

- (void)setIsAgreementAccepted:(bool)accepted {
  ai_chat::SetUserOptedIn(user_prefs::UserPrefs::Get(browser_state_), accepted);
}

- (void)changeModel:(NSString*)modelKey {
  current_conversation_->ChangeModel(base::SysNSStringToUTF8(modelKey));
}

- (NSArray<AiChatConversationTurn*>*)conversationHistory {
  NSMutableArray* history = [[NSMutableArray alloc] init];
  for (auto&& turn : current_conversation_->GetConversationHistory()) {
    [history addObject:[[AiChatConversationTurn alloc]
                           initWithConversationTurnPtr:turn->Clone()]];
  }
  return [history copy];
}

- (void)submitHumanConversationEntry:(NSString*)text {
  current_conversation_->SubmitHumanConversationEntry(
      ai_chat::mojom::ConversationTurn::New(
          ai_chat::mojom::CharacterType::HUMAN,
          ai_chat::mojom::ActionType::UNSPECIFIED,
          ai_chat::mojom::ConversationTurnVisibility::VISIBLE,
          base::SysNSStringToUTF8(text), std::nullopt, std::nullopt,
          base::Time::Now(), std::nullopt, false));
}

- (void)submitSummarizationRequest {
  current_conversation_->SubmitSummarizationRequest();
}

- (void)retryAPIRequest {
  current_conversation_->RetryAPIRequest();
}

- (void)generateQuestions {
  current_conversation_->GenerateQuestions();
}

- (NSArray<AiChatActionGroup*>*)slashActions {
  NSMutableArray* result = [[NSMutableArray alloc] init];
  for (auto&& group : ai_chat::GetActionMenuList()) {
    [result addObject:[[AiChatActionGroup alloc]
                          initWithActionGroupPtr:std::move(group)]];
  }
  return result;
}

- (NSString*)defaultModelKey {
  return base::SysUTF8ToNSString(model_service_->GetDefaultModelKey());
}

- (void)setDefaultModelKey:(NSString*)modelKey {
  model_service_->SetDefaultModelKey(base::SysNSStringToUTF8(modelKey));
}

- (void)setShouldSendPageContents:(bool)should_send {
  current_conversation_->SetShouldSendPageContents(should_send);
}

- (void)clearErrorAndGetFailedMessage:
    (void (^)(AiChatConversationTurn*))completion {
  current_conversation_->ClearErrorAndGetFailedMessage(base::BindOnce(
      [](void (^completion)(AiChatConversationTurn*),
         ai_chat::mojom::ConversationTurnPtr turn) {
        if (completion) {
          completion([[AiChatConversationTurn alloc]
              initWithConversationTurnPtr:std::move(turn)]);
        }
      },
      completion));
}

- (void)getState:(void (^)(AiChatConversationState*))completion {
  current_conversation_->GetState(base::BindOnce(
      [](void (^completion)(AiChatConversationState*),
         ai_chat::mojom::ConversationStatePtr state) {
        if (completion) {
          completion([[AiChatConversationState alloc]
              initWithConversationStatePtr:std::move(state)]);
        }
      },
      completion));
}

- (void)getPremiumStatus:(void (^)(AiChatPremiumStatus))completion {
  service_->GetPremiumStatus(base::BindOnce(
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
  current_conversation_->SubmitSelectedText(
      base::SysNSStringToUTF8(selectedText),
      static_cast<ai_chat::mojom::ActionType>(actionType));
}

- (void)rateMessage:(bool)isLiked
             turnId:(NSUInteger)turnId
         completion:(void (^)(NSString* identifier))completion {
  current_conversation_->RateMessage(
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
  current_conversation_->SendFeedback(base::SysNSStringToUTF8(category),
                                      base::SysNSStringToUTF8(feedback),
                                      base::SysNSStringToUTF8(ratingId),
                                      sendPageUrl, base::BindOnce(completion));
}

- (void)modifyConversation:(NSUInteger)turnId newText:(NSString*)newText {
  current_conversation_->ModifyConversation(turnId,
                                            base::SysNSStringToUTF8(newText));
}

- (void)getCanShowPremiumPrompt:(void (^_Nullable)(bool))completion {
  service_->GetCanShowPremiumPrompt(base::BindOnce(completion));
}

- (void)dismissPremiumPrompt {
  service_->DismissPremiumPrompt();
}
@end
