// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ai_chat/ai_chat_settings_helper.h"

#include <memory>

#include "base/apple/foundation_util.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/ios/ai_chat.mojom.objc+private.h"
#include "brave/components/ai_chat/core/common/mojom/ios/common.mojom.objc+private.h"
#include "brave/ios/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/ios/browser/ai_chat/model_service_factory.h"
#include "brave/ios/browser/api/profile/profile_bridge_impl.h"

namespace {

class SettingsHelperDelegateBridge : public ai_chat::ModelService::Observer {
 public:
  id<AIChatSettingsHelperDelegate> delegate() { return delegate_; }
  void set_delegate(id<AIChatSettingsHelperDelegate> bridge) {
    delegate_ = bridge;
  }

 private:
  __weak id<AIChatSettingsHelperDelegate> delegate_ = nullptr;

  // ai_chat::ModelService::Observer
  void OnModelListUpdated() override { [delegate_ modelListUpdated]; }
  void OnDefaultModelChanged(const std::string& old_key,
                             const std::string& new_key) override {
    [delegate_ defaultModelChangedFromOldKey:base::SysUTF8ToNSString(old_key)
                                       toKey:base::SysUTF8ToNSString(new_key)];
  }
};

}  // namespace

@interface AIChatSettingsHelperImpl () {
  std::unique_ptr<SettingsHelperDelegateBridge> _bridge;
  raw_ptr<ai_chat::ModelService> _modelService;
  raw_ptr<ai_chat::AIChatService> _aiChatService;
}
@end

@implementation AIChatSettingsHelperImpl

- (instancetype)initWithProfile:(id<ProfileBridge>)profileBridge {
  if ((self = [super init])) {
    ProfileBridgeImpl* holder =
        base::apple::ObjCCastStrict<ProfileBridgeImpl>(profileBridge);
    ProfileIOS* profile = holder.profile;

    _bridge = std::make_unique<SettingsHelperDelegateBridge>();
    _modelService = ai_chat::ModelServiceFactory::GetForProfile(profile);
    _modelService->AddObserver(_bridge.get());

    _aiChatService = ai_chat::AIChatServiceFactory::GetForProfile(profile);
  }
  return self;
}

- (void)dealloc {
  _modelService->RemoveObserver(_bridge.get());
}

- (id<AIChatSettingsHelperDelegate>)delegate {
  return _bridge->delegate();
}

- (void)setDelegate:(id<AIChatSettingsHelperDelegate>)delegate {
  _bridge->set_delegate(delegate);
}

- (NSArray<AiChatModel*>*)models {
  const std::vector<ai_chat::mojom::ModelPtr>& models =
      _modelService->GetModels();
  NSMutableArray* bridgedModels = [[NSMutableArray alloc] init];
  for (const ai_chat::mojom::ModelPtr& model : models) {
    [bridgedModels
        addObject:[[AiChatModel alloc] initWithModelPtr:model->Clone()]];
  }
  return [bridgedModels copy];
}

- (NSString*)defaultModelKey {
  return base::SysUTF8ToNSString(_modelService->GetDefaultModelKey());
}

- (void)setDefaultModelKey:(NSString*)defaultModelKey {
  _modelService->SetDefaultModelKey(base::SysNSStringToUTF8(defaultModelKey));
}

- (void)fetchPremiumStatus:(void (^)(AiChatPremiumStatus status,
                                     AiChatPremiumInfo* info))handler {
  _aiChatService->GetPremiumStatus(base::BindOnce(^(
      ai_chat::mojom::PremiumStatus status,
      ai_chat::mojom::PremiumInfoPtr info) {
    handler(static_cast<AiChatPremiumStatus>(status),
            [[AiChatPremiumInfo alloc] initWithPremiumInfoPtr:std::move(info)]);
  }));
}

@end
