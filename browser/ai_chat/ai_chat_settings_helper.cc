/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ai_chat/ai_chat_settings_helper.h"

#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/browser/skus/skus_service_factory.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/model_validator.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/net/base/url_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/build/android/jni_headers/BraveLeoMojomHelper_jni.h"
#include "content/public/browser/android/browser_context_handle.h"
#endif

namespace ai_chat {

namespace {
constexpr char kAccountHostnamePart[] = "account";

std::vector<mojom::ModelPtr> GetCustomModelsFromService(
    ModelService* model_service) {
  const std::vector<mojom::ModelPtr>& models = model_service->GetModels();

  std::vector<mojom::ModelPtr> models_copy;
  for (auto& model : models) {
    if (model->options->is_custom_model_options()) {
      models_copy.push_back(model->Clone());
    }
  }

  return models_copy;
}

}  // namespace

AIChatSettingsHelper::AIChatSettingsHelper(content::BrowserContext* context) {
  // TODO(petemill): Just use AIChatService to get premium status
  auto skus_service_getter = base::BindRepeating(
      [](content::BrowserContext* context) {
        return skus::SkusServiceFactory::GetForContext(context);
      },
      context);
  pref_service_ = Profile::FromBrowserContext(context)->GetPrefs();
  model_service_ = ModelServiceFactory::GetForBrowserContext(context);
  models_observer_.Observe(model_service_.get());

  credential_manager_ = std::make_unique<ai_chat::AIChatCredentialManager>(
      skus_service_getter, g_browser_process->local_state());
}

AIChatSettingsHelper::~AIChatSettingsHelper() {
  models_observer_.Reset();
}

void AIChatSettingsHelper::GetPremiumStatus(GetPremiumStatusCallback callback) {
  credential_manager_->GetPremiumStatus(
      base::BindOnce(&AIChatSettingsHelper::OnPremiumStatusReceived,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AIChatSettingsHelper::OnPremiumStatusReceived(
    mojom::Service::GetPremiumStatusCallback parent_callback,
    mojom::PremiumStatus premium_status,
    mojom::PremiumInfoPtr premium_info) {
  std::move(parent_callback).Run(premium_status, std::move(premium_info));
}

void AIChatSettingsHelper::OnModelListUpdated() {
  if (client_page_.is_bound()) {
    client_page_->OnModelListChanged(
        GetCustomModelsFromService(model_service_));
  }
}

void AIChatSettingsHelper::OnDefaultModelChanged(const std::string& old_key,
                                                 const std::string& new_key) {
  if (client_page_.is_bound()) {
    client_page_->OnDefaultModelChanged(new_key);
  }
}

void AIChatSettingsHelper::GetModelsWithSubtitles(
    GetModelsWithSubtitlesCallback callback) {
  const auto& all_models = model_service_->GetModels();
  std::vector<mojom::ModelWithSubtitlePtr> models;

  for (const auto& model : all_models) {
    mojom::ModelWithSubtitle modelWithSubtitle;
    modelWithSubtitle.model = model->Clone();

    if (model->options->is_leo_model_options()) {
      if (model->key == "chat-basic") {
        modelWithSubtitle.subtitle =
            l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_BASIC_SUBTITLE);
      } else if (model->key == "chat-leo-expanded") {
        modelWithSubtitle.subtitle =
            l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_LEO_EXPANDED_SUBTITLE);
      } else if (model->key == "chat-claude-instant") {
        modelWithSubtitle.subtitle =
            l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_CLAUDE_INSTANT_SUBTITLE);
      } else if (model->key == "chat-claude-haiku") {
        modelWithSubtitle.subtitle =
            l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_CLAUDE_HAIKU_SUBTITLE);
      } else if (model->key == "chat-claude-sonnet") {
        modelWithSubtitle.subtitle =
            l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_CLAUDE_SONNET_SUBTITLE);
      }
    }

    if (model->options->is_custom_model_options()) {
      modelWithSubtitle.subtitle = "";
    }

    models.emplace_back(modelWithSubtitle.Clone());
  }

  std::move(callback).Run(std::move(models));
}

void AIChatSettingsHelper::GetManageUrl(GetManageUrlCallback callback) {
#if defined(OFFICIAL_BUILD)
  std::string domain = brave_domains::GetServicesDomain(kAccountHostnamePart);
#else
  std::string domain = brave_domains::GetServicesDomain(kAccountHostnamePart,
                                                        brave_domains::STAGING);
#endif

  std::move(callback).Run(
      base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator, domain}));
}

void AIChatSettingsHelper::GetCustomModels(GetCustomModelsCallback callback) {
  std::move(callback).Run(GetCustomModelsFromService(model_service_));
}

void AIChatSettingsHelper::AddCustomModel(mojom::ModelPtr model,
                                          AddCustomModelCallback callback) {
  CHECK(model->options->is_custom_model_options());

  ModelValidationResult result = ModelValidator::ValidateCustomModelOptions(
      *model->options->get_custom_model_options());
  if (result == ModelValidationResult::kInvalidUrl) {
    std::move(callback).Run(mojom::OperationResult::InvalidUrl);
    return;
  }

  model_service_->AddCustomModel(std::move(model));
  std::move(callback).Run(mojom::OperationResult::Success);
}

void AIChatSettingsHelper::SaveCustomModel(uint32_t index,
                                           mojom::ModelPtr model,
                                           SaveCustomModelCallback callback) {
  CHECK(model->options->is_custom_model_options());

  ModelValidationResult result = ModelValidator::ValidateCustomModelOptions(
      *model->options->get_custom_model_options());
  if (result == ModelValidationResult::kInvalidUrl) {
    const auto endpoint = model->options->get_custom_model_options()->endpoint;
    const bool valid_as_private_ip =
        ModelValidator::IsValidEndpoint(endpoint, true);
    // The URL is invalid, but may be valid as a private endpoint. Let's
    // examine the value more closely, and notify the user.
    std::move(callback).Run(
        valid_as_private_ip ? mojom::OperationResult::UrlValidAsPrivateEndpoint
                            : mojom::OperationResult::InvalidUrl);

    return;
  }

  model_service_->SaveCustomModel(index, std::move(model));
  std::move(callback).Run(mojom::OperationResult::Success);
}

void AIChatSettingsHelper::DeleteCustomModel(uint32_t index) {
  model_service_->DeleteCustomModel(index);
}

void AIChatSettingsHelper::SetDefaultModelKey(const std::string& model_key) {
  model_service_->SetDefaultModelKey(model_key);
}

void AIChatSettingsHelper::GetDefaultModelKey(
    GetDefaultModelKeyCallback callback) {
  std::move(callback).Run(model_service_->GetDefaultModelKey());
}

void AIChatSettingsHelper::SetClientPage(
    mojo::PendingRemote<mojom::SettingsPage> page) {
  client_page_.Bind(std::move(page));
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

void AIChatSettingsHelper::OnCreateOrderId(
    CreateOrderIdCallback callback,
    skus::mojom::SkusResultPtr response) {
  std::move(callback).Run(response->message);
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
    skus::mojom::SkusResultPtr response) {
  std::move(callback).Run(response->message);
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
                                          skus::mojom::SkusResultPtr response) {
  std::move(callback).Run(response->message);
}
#endif

}  // namespace ai_chat
