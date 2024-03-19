/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_SETTINGS_HELPER_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_SETTINGS_HELPER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
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

class AIChatSettingsHelper : public mojom::AIChatSettingsHelper {
 public:
  explicit AIChatSettingsHelper(content::BrowserContext* context);
  ~AIChatSettingsHelper() override;

  // mojom::AIChatSettingsHelper:
  void GetPremiumStatus(GetPremiumStatusCallback callback) override;
  void GetModelsWithSubtitles(GetModelsWithSubtitlesCallback callback) override;
  void GetManageUrl(GetManageUrlCallback callback) override;

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
      mojom::PageHandler::GetPremiumStatusCallback parent_callback,
      mojom::PremiumStatus premium_status,
      mojom::PremiumInfoPtr premium_info);

#if BUILDFLAG(IS_ANDROID)
  void OnCreateOrderId(CreateOrderIdCallback callback,
                       const std::string& response);
  void OnFetchOrderCredentials(FetchOrderCredentialsCallback callback,
                               const std::string& order_id,
                               const std::string& response);
  void OnRefreshOrder(RefreshOrderCallback callback,
                      const std::string& order_id,
                      const std::string& response);
#endif

  std::unique_ptr<AIChatCredentialManager> credential_manager_;
  mojo::ReceiverSet<mojom::AIChatSettingsHelper> receivers_;
  raw_ptr<PrefService> pref_service_ = nullptr;
  base::WeakPtrFactory<AIChatSettingsHelper> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_SETTINGS_HELPER_H_
