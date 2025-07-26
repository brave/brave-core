// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_wallet/brave_wallet_communication_tab_helper.h"

#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web/public/web_state_user_data.h"

class BraveWalletCommunicationTabHelper
    : public web::WebStateUserData<BraveWalletCommunicationTabHelper> {
 public:
  BraveWalletCommunicationTabHelper(const BraveWalletCommunicationTabHelper&) =
      delete;
  BraveWalletCommunicationTabHelper& operator=(
      const BraveWalletCommunicationTabHelper&) = delete;
  ~BraveWalletCommunicationTabHelper() override;

  BraveWalletCommunicationController* GetController() const;
  void SetController(BraveWalletCommunicationController* controller);

 private:
  friend class web::WebStateUserData<BraveWalletCommunicationTabHelper>;

  BraveWalletCommunicationController* controller_;

  explicit BraveWalletCommunicationTabHelper(web::WebState* web_state);

  base::WeakPtrFactory<BraveWalletCommunicationTabHelper> weak_ptr_factory_{
      this};
};

BraveWalletCommunicationTabHelper::BraveWalletCommunicationTabHelper(
    web::WebState* web_state)
    : controller_(nullptr) {}

BraveWalletCommunicationTabHelper::~BraveWalletCommunicationTabHelper() =
    default;

BraveWalletCommunicationController*
BraveWalletCommunicationTabHelper::GetController() const {
  return controller_;
}

void BraveWalletCommunicationTabHelper::SetController(
    BraveWalletCommunicationController* controller) {
  controller_ = controller;
}

#pragma mark :- Objective-C

@interface BraveWalletCommunicationController () {
  raw_ptr<web::WebState> web_state_;
}
@end

@implementation BraveWalletCommunicationController

- (instancetype)initWithWebState:(web::WebState*)webState {
  if ((self = [super init])) {
    web_state_ = webState;
  }
  return self;
}

+ (void)createForWebState:(web::WebState*)webState {
  BraveWalletCommunicationTabHelper::CreateForWebState(webState);
}

+ (BraveWalletCommunicationController*)fromWebState:(web::WebState*)webState {
  if (auto* tab_helper =
          BraveWalletCommunicationTabHelper::FromWebState(webState)) {
    if (!tab_helper->GetController()) {
      tab_helper->SetController([[BraveWalletCommunicationController alloc]
          initWithWebState:webState]);
    }
    return tab_helper->GetController();
  }
  return nullptr;
}
@end
