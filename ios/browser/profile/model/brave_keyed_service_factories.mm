/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/profile/model/brave_keyed_service_factories.h"

#include "brave/ios/browser/api/ai_chat/ai_chat_service_factory.h"
#include "brave/ios/browser/api/ai_chat/model_service_factory.h"
#include "brave/ios/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_ipfs_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/meld_integration_service_factory.h"
#include "brave/ios/browser/brave_wallet/swap_service_factory.h"
#include "brave/ios/browser/debounce/debounce_service_factory+private.h"
#include "brave/ios/browser/favicon/brave_ios_favicon_loader_factory.h"
#include "brave/ios/browser/skus/skus_service_factory.h"
#include "brave/ios/browser/url_sanitizer/url_sanitizer_service_factory+private.h"
#import "ios/web_view/internal/autofill/web_view_autofill_log_router_factory.h"
#import "ios/web_view/internal/autofill/web_view_personal_data_manager_factory.h"
#import "ios/web_view/internal/language/web_view_accept_languages_service_factory.h"
#import "ios/web_view/internal/language/web_view_language_model_manager_factory.h"
#import "ios/web_view/internal/language/web_view_url_language_histogram_factory.h"
#import "ios/web_view/internal/passwords/web_view_account_password_store_factory.h"
#import "ios/web_view/internal/passwords/web_view_bulk_leak_check_service_factory.h"
#import "ios/web_view/internal/passwords/web_view_password_manager_log_router_factory.h"
#import "ios/web_view/internal/passwords/web_view_password_requirements_service_factory.h"
#import "ios/web_view/internal/passwords/web_view_profile_password_store_factory.h"
#import "ios/web_view/internal/signin/web_view_identity_manager_factory.h"
#import "ios/web_view/internal/signin/web_view_signin_client_factory.h"
#import "ios/web_view/internal/sync/web_view_gcm_profile_service_factory.h"
#import "ios/web_view/internal/sync/web_view_model_type_store_service_factory.h"
#import "ios/web_view/internal/sync/web_view_profile_invalidation_provider_factory.h"
#import "ios/web_view/internal/sync/web_view_sync_service_factory.h"
#import "ios/web_view/internal/translate/web_view_translate_ranker_factory.h"
#import "ios/web_view/internal/web_view_download_manager.h"
#import "ios/web_view/internal/web_view_url_request_context_getter.h"
#import "ios/web_view/internal/webdata_services/web_view_web_data_service_wrapper_factory.h"

namespace brave {

void EnsureProfileKeyedServiceFactoriesBuilt() {
  ai_chat::ModelServiceFactory::GetInstance();
  ai_chat::AIChatServiceFactory::GetInstance();
  brave_favicon::BraveIOSFaviconLoaderFactory::GetInstance();
  brave_wallet::AssetRatioServiceFactory::GetInstance();
  brave_wallet::BraveWalletIpfsServiceFactory::GetInstance();
  brave_wallet::BraveWalletServiceFactory::GetInstance();
  brave_wallet::MeldIntegrationServiceFactory::GetInstance();
  brave_wallet::SwapServiceFactory::GetInstance();
  skus::SkusServiceFactory::GetInstance();
  brave::URLSanitizerServiceFactory::GetInstance();
  debounce::DebounceServiceFactory::GetInstance();

  ios_web_view::WebViewLanguageModelManagerFactory::GetInstance();
  ios_web_view::WebViewTranslateRankerFactory::GetInstance();
  ios_web_view::WebViewUrlLanguageHistogramFactory::GetInstance();
  ios_web_view::WebViewAcceptLanguagesServiceFactory::GetInstance();
  autofill::WebViewAutofillLogRouterFactory::GetInstance();
  ios_web_view::WebViewPersonalDataManagerFactory::GetInstance();
  ios_web_view::WebViewWebDataServiceWrapperFactory::GetInstance();
  ios_web_view::WebViewPasswordManagerLogRouterFactory::GetInstance();
  ios_web_view::WebViewAccountPasswordStoreFactory::GetInstance();
  ios_web_view::WebViewProfilePasswordStoreFactory::GetInstance();
  ios_web_view::WebViewPasswordRequirementsServiceFactory::GetInstance();
  ios_web_view::WebViewSigninClientFactory::GetInstance();
  ios_web_view::WebViewIdentityManagerFactory::GetInstance();
  ios_web_view::WebViewGCMProfileServiceFactory::GetInstance();
  ios_web_view::WebViewProfileInvalidationProviderFactory::GetInstance();
  ios_web_view::WebViewSyncServiceFactory::GetInstance();
  ios_web_view::WebViewModelTypeStoreServiceFactory::GetInstance();
  ios_web_view::WebViewBulkLeakCheckServiceFactory::GetInstance();
}

}  // namespace brave
