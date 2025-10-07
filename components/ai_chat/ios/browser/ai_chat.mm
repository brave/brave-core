// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/ios/browser/ai_chat.h"

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "brave/base/apple/foundation_util.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/ios/ai_chat.mojom.objc+private.h"
#include "brave/components/ai_chat/core/common/mojom/ios/common.mojom.objc+private.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/ios/browser/associated_content_driver_ios.h"
#include "brave/components/ai_chat/ios/browser/conversation_client.h"
#include "components/prefs/pref_service.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"

@implementation AiChat
@end

@interface AIChat () {
  raw_ptr<ai_chat::AIChatService> _service;
  raw_ptr<ai_chat::ModelService> _modelService;
  raw_ptr<ai_chat::ConversationHandler> _currentConversation;
  raw_ptr<PrefService> _prefsService;

  // TODO(petemill): Pass the bindings to the UI ViewModel so we
  // can avoid simply proxying data and events through this class.
  std::unique_ptr<ai_chat::ConversationClient> _conversationClient;

  std::unique_ptr<ai_chat::AssociatedContentDriver> _currentContent;

  __weak id<AIChatDelegate> _delegate;
}
@end

@implementation AIChat
- (instancetype)initWithAIChatService:(ai_chat::AIChatService*)service
                         modelService:(ai_chat::ModelService*)modelService
                         profilePrefs:(PrefService*)prefsService
                sharedURLoaderFactory:
                    (scoped_refptr<network::SharedURLLoaderFactory>)
                        sharedURLoaderFactory
                             delegate:(id<AIChatDelegate>)delegate {
  if ((self = [super init])) {
    _delegate = delegate;

    _modelService = modelService;
    _service = service;
    _prefsService = prefsService;

    _currentContent = std::make_unique<ai_chat::AssociatedContentDriverIOS>(
        sharedURLoaderFactory, delegate);

    _conversationClient = std::make_unique<ai_chat::ConversationClient>(
        _service.get(), _delegate);

    [self createNewConversation];
  }
  return self;
}

- (void)dealloc {
  web::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(
                     ^(decltype(_currentContent) current_content) {
                       current_content.reset();
                     },
                     std::move(_currentContent)));
}

- (void)createNewConversation {
  _currentConversation = _service->CreateConversationHandlerForContent(
      _currentContent->content_id(), _currentContent->GetWeakPtr());
  _conversationClient->ChangeConversation(_currentConversation.get());
}

- (bool)isAgreementAccepted {
  return _service->HasUserOptedIn();
}

- (void)setIsAgreementAccepted:(bool)accepted {
  ai_chat::SetUserOptedIn(_prefsService, accepted);
}

- (void)changeModel:(NSString*)modelKey {
  _currentConversation->ChangeModel(base::SysNSStringToUTF8(modelKey));
}

- (NSArray<AiChatConversationTurn*>*)conversationHistory {
  NSMutableArray* history = [[NSMutableArray alloc] init];
  for (auto&& turn : _currentConversation->GetConversationHistory()) {
    [history addObject:[[AiChatConversationTurn alloc]
                           initWithConversationTurnPtr:turn->Clone()]];
  }
  return [history copy];
}

- (void)submitHumanConversationEntry:(NSString*)text {
  _currentConversation->SubmitHumanConversationEntry(
      base::SysNSStringToUTF8(text), std::nullopt);
}

- (void)submitSuggestion:(NSString*)text {
  _currentConversation->SubmitSuggestion(base::SysNSStringToUTF8(text));
}

- (void)submitSummarizationRequest {
  _currentConversation->SubmitSummarizationRequest();
}

- (void)retryAPIRequest {
  _currentConversation->RetryAPIRequest();
}

- (void)generateQuestions {
  _currentConversation->GenerateQuestions();
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
  return base::SysUTF8ToNSString(_modelService->GetDefaultModelKey());
}

- (void)setDefaultModelKey:(NSString*)modelKey {
  _modelService->SetDefaultModelKey(base::SysNSStringToUTF8(modelKey));
}

- (void)setShouldSendPageContents:(bool)should_send {
  if (should_send) {
    _currentConversation->associated_content_manager()->AddContent(
        _currentContent.get());
  } else {
    _currentConversation->associated_content_manager()->RemoveContent(
        _currentContent.get());
  }
}

- (void)clearErrorAndGetFailedMessage:
    (void (^)(AiChatConversationTurn*))completion {
  _currentConversation->ClearErrorAndGetFailedMessage(base::BindOnce(
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
  _currentConversation->GetState(base::BindOnce(
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
  _service->GetPremiumStatus(base::BindOnce(
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
  _currentConversation->SubmitSelectedText(
      base::SysNSStringToUTF8(selectedText),
      static_cast<ai_chat::mojom::ActionType>(actionType));
}

- (void)rateMessage:(bool)isLiked
             turnId:(NSString*)turnId
         completion:(void (^)(NSString* identifier))completion {
  _currentConversation->RateMessage(
      isLiked, base::SysNSStringToUTF8(turnId),
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
  _currentConversation->SendFeedback(base::SysNSStringToUTF8(category),
                                     base::SysNSStringToUTF8(feedback),
                                     base::SysNSStringToUTF8(ratingId),
                                     sendPageUrl, base::BindOnce(completion));
}

- (void)modifyConversation:(NSString*)turnId newText:(NSString*)newText {
  _currentConversation->ModifyConversation(base::SysNSStringToUTF8(turnId),
                                           base::SysNSStringToUTF8(newText));
}

- (void)dismissPremiumPrompt {
  _service->DismissPremiumPrompt();
}
@end
