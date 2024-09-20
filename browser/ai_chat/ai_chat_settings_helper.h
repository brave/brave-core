/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_SETTINGS_HELPER_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_SETTINGS_HELPER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/ai_chat/content/browser/model_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/settings_helper.mojom.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#endif

class PrefService;
namespace content {
class BrowserContext;
}  // namespace content

namespace ai_chat {

class AIChatSettingsHelper : public mojom::AIChatSettingsHelper,
                             public ModelService::Observer {
 public:
  explicit AIChatSettingsHelper(content::BrowserContext* context);
  ~AIChatSettingsHelper() override;

  // mojom::AIChatSettingsHelper:
  void GetPremiumStatus(GetPremiumStatusCallback callback) override;
  void GetModelsWithSubtitles(GetModelsWithSubtitlesCallback callback) override;
  void GetManageUrl(GetManageUrlCallback callback) override;
  void GetCustomModels(GetCustomModelsCallback callback) override;
  void AddCustomModel(mojom::ModelPtr model,
                      AddCustomModelCallback callback) override;
  void SaveCustomModel(uint32_t index,
                       mojom::ModelPtr model,
                       SaveCustomModelCallback callback) override;
  void DeleteCustomModel(uint32_t index) override;
  void SetDefaultModelKey(const std::string& model_key) override;
  void GetDefaultModelKey(GetDefaultModelKeyCallback callback) override;
  void SetClientPage(mojo::PendingRemote<mojom::SettingsPage> page) override;

  void BindInterface(
      mojo::PendingReceiver<mojom::AIChatSettingsHelper> pending_receiver);

#if BUILDFLAG(IS_ANDROID)
  void Destroy(JNIEnv* env);
  jlong GetInterfaceToAndroidHelper(JNIEnv* env);
  void CreateOrderId(CreateOrderIdCallback callback) override;
  void FetchOrderCredentials(const std::string& order_id,
                             FetchOrderCredentialsCallback callback) override;
  void RefreshOrder(const std::string& order_id,
                    RefreshOrderCallback callback) override;
#endif

 private:
  void OnPremiumStatusReceived(
      mojom::Service::GetPremiumStatusCallback parent_callback,
      mojom::PremiumStatus premium_status,
      mojom::PremiumInfoPtr premium_info);

  // ModelService::Observer
  void OnModelListUpdated() override;
  void OnDefaultModelChanged(const std::string& old_key,
                             const std::string& new_key) override;

#if BUILDFLAG(IS_ANDROID)
  void OnCreateOrderId(CreateOrderIdCallback callback,
                       skus::mojom::SkusResultPtr response);
  void OnFetchOrderCredentials(FetchOrderCredentialsCallback callback,
                               const std::string& order_id,
                               skus::mojom::SkusResultPtr response);
  void OnRefreshOrder(RefreshOrderCallback callback,
                      const std::string& order_id,
                      skus::mojom::SkusResultPtr response);
#endif

  base::ScopedObservation<ModelService, ModelService::Observer>
      models_observer_{this};
  std::unique_ptr<AIChatCredentialManager> credential_manager_;
  mojo::ReceiverSet<mojom::AIChatSettingsHelper> receivers_;
  raw_ptr<PrefService> pref_service_ = nullptr;
  raw_ptr<ModelService> model_service_;
  mojo::Remote<mojom::SettingsPage> client_page_;
  base::WeakPtrFactory<AIChatSettingsHelper> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_SETTINGS_HELPER_H_
