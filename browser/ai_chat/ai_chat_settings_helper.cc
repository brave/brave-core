/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ai_chat/ai_chat_settings_helper.h"

#include <utility>
#include <vector>
#include "base/strings/strcat.h"

#include "brave/browser/skus/skus_service_factory.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/models.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "ui/base/l10n/l10n_util.h"
#include "brave/brave_domains/service_domains.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/build/android/jni_headers/BraveLeoMojomHelper_jni.h"
#include "content/public/browser/android/browser_context_handle.h"
#endif

namespace ai_chat {

namespace {
  constexpr char kAccountHostnamePart[] = "account";
}

AIChatSettingsHelper::AIChatSettingsHelper(content::BrowserContext* context) {
  auto skus_service_getter = base::BindRepeating(
      [](content::BrowserContext* context) {
        return skus::SkusServiceFactory::GetForContext(context);
      },
      context);
  pref_service_ = Profile::FromBrowserContext(context)->GetPrefs();
  credential_manager_ = std::make_unique<ai_chat::AIChatCredentialManager>(
      skus_service_getter, g_browser_process->local_state());
}

AIChatSettingsHelper::~AIChatSettingsHelper() = default;

void AIChatSettingsHelper::GetPremiumStatus(GetPremiumStatusCallback callback) {
  credential_manager_->GetPremiumStatus(
      base::BindOnce(&AIChatSettingsHelper::OnPremiumStatusReceived,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AIChatSettingsHelper::OnPremiumStatusReceived(
    mojom::PageHandler::GetPremiumStatusCallback parent_callback,
    mojom::PremiumStatus premium_status,
    mojom::PremiumInfoPtr premium_info) {
  std::move(parent_callback).Run(premium_status, std::move(premium_info));
}

void AIChatSettingsHelper::GetModelsWithSubtitles(
    GetModelsWithSubtitlesCallback callback) {
  auto all_models = GetAllModels();
  std::vector<mojom::ModelWithSubtitlePtr> models(all_models.size());
  for (size_t i = 0; i < all_models.size(); i++) {
    mojom::ModelWithSubtitle modelWithSubtitle;
    modelWithSubtitle.model = all_models[i].Clone();

    bool is_key_handled = false; // Flag to track if the key is recognized.

    if (modelWithSubtitle.model->key == "chat-basic") {
      modelWithSubtitle.subtitle =
          l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_BASIC_SUBTITLE);
      is_key_handled = true;
    } else if (modelWithSubtitle.model->key == "chat-leo-expanded") {
      modelWithSubtitle.subtitle =
          l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_LEO_EXPANDED_SUBTITLE);
      is_key_handled = true;
    } else if (modelWithSubtitle.model->key == "chat-claude-instant") {
      modelWithSubtitle.subtitle =
          l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_CLAUDE_INSTANT_SUBTITLE);
      is_key_handled = true;
    } else if (modelWithSubtitle.model->key == "chat-claude-haiku") {
      modelWithSubtitle.subtitle =
          l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_CLAUDE_HAIKU_SUBTITLE);
      is_key_handled = true;
    } else if (modelWithSubtitle.model->key == "chat-claude-sonnet") {
      modelWithSubtitle.subtitle =
          l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_CLAUDE_SONNET_SUBTITLE);
      is_key_handled = true;
    }

    DCHECK(is_key_handled) << "Unhandled model key: " << modelWithSubtitle.model->key;

    models[i] = modelWithSubtitle.Clone();
  }

  std::move(callback).Run(std::move(models));
}

void AIChatSettingsHelper::GetManageUrl(GetManageUrlCallback callback) {
#if defined(OFFICIAL_BUILD)
  std::string domain = brave_domains::GetServicesDomain(kAccountHostnamePart);
#else
  std::string domain = brave_domains::GetServicesDomain(kAccountHostnamePart, brave_domains::STAGING);
#endif

  std::move(callback).Run(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator, domain}));
}

void AIChatSettingsHelper::BindInterface(
    mojo::PendingReceiver<mojom::AIChatSettingsHelper> pending_receiver) {
  receivers_.Add(this, std::move(pending_receiver));
}

#if BUILDFLAG(IS_ANDROID)
static jlong JNI_BraveLeoMojomHelper_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jbrowser_context_handle) {
  content::BrowserContext* browser_context =
      content::BrowserContextFromJavaHandle(jbrowser_context_handle);
  AIChatSettingsHelper* settings_helper =
      new AIChatSettingsHelper(browser_context);
  return reinterpret_cast<intptr_t>(settings_helper);
}

void AIChatSettingsHelper::Destroy(JNIEnv* env) {
  delete this;
}

jlong AIChatSettingsHelper::GetInterfaceToAndroidHelper(JNIEnv* env) {
  mojo::PendingRemote<mojom::AIChatSettingsHelper> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());

  return static_cast<jlong>(remote.PassPipe().release().value());
}

void AIChatSettingsHelper::CreateOrderId(CreateOrderIdCallback callback) {
  std::string purchase_token_string;
  auto* purchase_token =
      pref_service_->FindPreference(prefs::kBraveChatPurchaseTokenAndroid);
  if (purchase_token && !purchase_token->IsDefaultValue()) {
    purchase_token_string =
        pref_service_->GetString(prefs::kBraveChatPurchaseTokenAndroid);
  }
  std::string package_string;
  auto* package =
      pref_service_->FindPreference(prefs::kBraveChatPackageNameAndroid);
  if (package && !package->IsDefaultValue()) {
    package_string =
        pref_service_->GetString(prefs::kBraveChatPackageNameAndroid);
  }
  std::string subscription_id_string;
  auto* subscription_id =
      pref_service_->FindPreference(prefs::kBraveChatProductIdAndroid);
  if (subscription_id && !subscription_id->IsDefaultValue()) {
    subscription_id_string =
        pref_service_->GetString(prefs::kBraveChatProductIdAndroid);
  }
  if (purchase_token_string.empty() || package_string.empty() ||
      subscription_id_string.empty()) {
    std::move(callback).Run("");
    return;
  }
  credential_manager_->CreateOrderFromReceipt(
      purchase_token_string, package_string, subscription_id_string,
      base::BindOnce(&AIChatSettingsHelper::OnCreateOrderId,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AIChatSettingsHelper::OnCreateOrderId(CreateOrderIdCallback callback,
                                           const std::string& response) {
  std::move(callback).Run(response);
}

void AIChatSettingsHelper::FetchOrderCredentials(
    const std::string& order_id,
    FetchOrderCredentialsCallback callback) {
  credential_manager_->FetchOrderCredentials(
      order_id, base::BindOnce(&AIChatSettingsHelper::OnFetchOrderCredentials,
                               weak_ptr_factory_.GetWeakPtr(),
                               std::move(callback), order_id));
}

void AIChatSettingsHelper::OnFetchOrderCredentials(
    FetchOrderCredentialsCallback callback,
    const std::string& order_id,
    const std::string& response) {
  std::move(callback).Run(response);
}

void AIChatSettingsHelper::RefreshOrder(const std::string& order_id,
                                        RefreshOrderCallback callback) {
  credential_manager_->RefreshOrder(
      order_id, base::BindOnce(&AIChatSettingsHelper::OnRefreshOrder,
                               weak_ptr_factory_.GetWeakPtr(),
                               std::move(callback), order_id));
}

void AIChatSettingsHelper::OnRefreshOrder(RefreshOrderCallback callback,
                                          const std::string& order_id,
                                          const std::string& response) {
  std::move(callback).Run(response);
}
#endif

}  // namespace ai_chat
