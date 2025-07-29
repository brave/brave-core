// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_communication_tab_helper.h"

#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web/public/web_state_user_data.h"

class AIChatCommunicationTabHelper
    : public web::WebStateUserData<AIChatCommunicationTabHelper> {
 public:
  AIChatCommunicationTabHelper(const AIChatCommunicationTabHelper&) = delete;
  AIChatCommunicationTabHelper& operator=(const AIChatCommunicationTabHelper&) =
  delete;
  ~AIChatCommunicationTabHelper() override;
      
  AIChatCommunicationController* GetController() const;
  void SetController(AIChatCommunicationController* controller);

 private:
  friend class web::WebStateUserData<AIChatCommunicationTabHelper>;
      
  AIChatCommunicationController* controller_;

  explicit AIChatCommunicationTabHelper(web::WebState* web_state);
      
  base::WeakPtrFactory<AIChatCommunicationTabHelper> weak_ptr_factory_{this};
};

AIChatCommunicationTabHelper::AIChatCommunicationTabHelper(
    web::WebState* web_state) : controller_(nullptr) {}

AIChatCommunicationTabHelper::~AIChatCommunicationTabHelper() = default;

#pragma mark :- Objective-C

@interface AIChatCommunicationController()
{
  raw_ptr<web::WebState> web_state_;
}
@end

@implementation AIChatCommunicationController

- (instancetype)initWithWebState:(web::WebState*)webState {
  if ((self = [super init])) {
    web_state_ = webState;
  }
  return self;
}

- (void)CreateForWebState:(web::WebState)webState {
  AIChatCommunicationTabHelper::CreateForWebState(webState);
}

+ (AIChatCommunicationController*)FromWebState:(web::WebState*)webState {
  if (auto* tab_helper = AIChatCommunicationTabHelper::FromWebState(webState)) {
    if (!tab_helper->GetController()) {
      tab_helper->SetController([[AIChatCommunicationController alloc] initWithWebState:webState]);
    }
    return tab_helper->GetController();
  }
  return nullptr;
}
@end
