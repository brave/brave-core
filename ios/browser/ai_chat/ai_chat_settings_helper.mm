// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ai_chat/ai_chat_settings_helper.h"

#include <memory>
#include <string>

#include "base/apple/foundation_util.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/model_validator.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/ios/ai_chat.mojom.objc+private.h"
#include "brave/components/ai_chat/core/common/mojom/ios/common.mojom.objc+private.h"
#include "brave/components/ai_chat/core/common/mojom/ios/settings_helper.mojom.objc+private.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "brave/ios/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/ios/browser/ai_chat/model_service_factory.h"
#include "brave/ios/browser/api/profile/profile_bridge_impl.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ui/base/l10n/l10n_util.h"

NSInteger const AIChatDefaultCustomModelContextSize =
    ai_chat::kDefaultCustomModelContextSize;

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
  raw_ptr<PrefService> _profilePrefs;
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

    _profilePrefs = profile->GetPrefs();
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

- (NSArray<AiChatModelWithSubtitle*>*)modelsWithSubtitles {
  std::vector<ai_chat::mojom::ModelWithSubtitlePtr> models =
      _modelService->GetModelsWithSubtitles();
  NSMutableArray* bridgedModels = [[NSMutableArray alloc] init];
  for (const ai_chat::mojom::ModelWithSubtitlePtr& model : models) {
    [bridgedModels addObject:[[AiChatModelWithSubtitle alloc]
                                 initWithModelWithSubtitlePtr:model->Clone()]];
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
  _aiChatService->GetPremiumStatus(
      base::BindOnce(^(ai_chat::mojom::PremiumStatus status,
                       ai_chat::mojom::PremiumInfoPtr info) {
        handler(static_cast<AiChatPremiumStatus>(status),
                info ? [[AiChatPremiumInfo alloc]
                           initWithPremiumInfoPtr:std::move(info)]
                     : nil);
      }));
}

- (NSArray<AiChatModel*>*)customModels {
  const std::vector<ai_chat::mojom::ModelPtr>& models =
      _modelService->GetModels();
  NSMutableArray<AiChatModel*>* bridgedModels = [[NSMutableArray alloc] init];
  for (auto& model : models) {
    if (model->options->is_custom_model_options()) {
      [bridgedModels
          addObject:[[AiChatModel alloc] initWithModelPtr:model->Clone()]];
    }
  }
  return [bridgedModels copy];
}

- (void)addCustomModel:(AiChatModel*)bridgedModel
     completionHandler:(void (^)(AiChatOperationResult result))handler {
  ai_chat::mojom::ModelPtr model = bridgedModel.cppObjPtr;
  CHECK(model->options->is_custom_model_options());

  ai_chat::ModelValidationResult result =
      ai_chat::ModelValidator::ValidateCustomModelOptions(
          *model->options->get_custom_model_options());
  if (result == ai_chat::ModelValidationResult::kInvalidUrl) {
    const auto endpoint = model->options->get_custom_model_options()->endpoint;
    const bool valid_as_private_ip =
        ai_chat::ModelValidator::IsValidEndpoint(endpoint, true);
    // The URL is invalid, but may be valid as a private endpoint. Let's
    // examine the value more closely, and notify the user.
    handler(valid_as_private_ip ? AiChatOperationResultUrlValidAsPrivateEndpoint
                                : AiChatOperationResultInvalidUrl);
    return;
  }

  _modelService->AddCustomModel(std::move(model));
  handler(AiChatOperationResultSuccess);
}

- (void)updateCustomModelAtIndex:(NSInteger)index
                           model:(AiChatModel*)bridgedModel
               completionHandler:
                   (void (^)(AiChatOperationResult result))handler {
  ai_chat::mojom::ModelPtr model = bridgedModel.cppObjPtr;
  CHECK(model->options->is_custom_model_options());

  ai_chat::ModelValidationResult result =
      ai_chat::ModelValidator::ValidateCustomModelOptions(
          *model->options->get_custom_model_options());
  if (result == ai_chat::ModelValidationResult::kInvalidUrl) {
    const auto endpoint = model->options->get_custom_model_options()->endpoint;
    const bool valid_as_private_ip =
        ai_chat::ModelValidator::IsValidEndpoint(endpoint, true);
    // The URL is invalid, but may be valid as a private endpoint. Let's
    // examine the value more closely, and notify the user.
    handler(valid_as_private_ip ? AiChatOperationResultUrlValidAsPrivateEndpoint
                                : AiChatOperationResultInvalidUrl);
    return;
  }

  _modelService->SaveCustomModel(index, std::move(model));
  handler(AiChatOperationResultSuccess);
}

- (void)deleteCustomModelAtIndex:(NSInteger)index {
  _modelService->DeleteCustomModel(index);
}

- (NSString*)defaultCustomModelSystemPrompt {
  std::string prompt = base::ReplaceStringPlaceholders(
      l10n_util::GetStringUTF8(IDS_AI_CHAT_DEFAULT_CUSTOM_MODEL_SYSTEM_PROMPT),
      {"%datetime%"}, nullptr);
  return base::SysUTF8ToNSString(prompt);
}

- (void)resetLeoData {
  _aiChatService->DeleteConversations();
  ai_chat::SetUserOptedIn(_profilePrefs, false);
  ai_chat::prefs::DeleteAllMemoriesFromPrefs(*_profilePrefs);
  ai_chat::prefs::ResetCustomizationsPref(*_profilePrefs);
}

@end
