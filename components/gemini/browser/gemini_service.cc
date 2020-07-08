/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/gemini/browser/gemini_service.h"

#include <string>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/containers/flat_set.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "base/time/time.h"
#include "base/token.h"
#include "brave/common/pref_names.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {

  const char oauth_host[] = "exchange.qa001.aurora7.net";
  const char api_host[] = "api.qa001.aurora7.net";
  const char oauth_callback[] = "com.brave.gemini://authorization";
  const char oauth_scope[] = "trader";
  const char oauth_url[] = "https://exchange.qa001.aurora7.net/auth";
  const unsigned int kRetriesCountOnNetworkChange = 1;

  net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
    return net::DefineNetworkTrafficAnnotation("gemini_service", R"(
        semantics {
          sender: "Gemini Service"
          description:
            "This service is used to communicate with Gemini "
            "on behalf of the user interacting with the Gemini widget."
          trigger:
            "Triggered by user connecting the Gemini widget."
          data:
            "Account information, balances"
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "You can enable or disable this feature on the new tab page."
          policy_exception_justification:
            "Not implemented."
        }
      )");
  }

}

GeminiService::GeminiService(content::BrowserContext* context)
    : client_id_(GEMINI_CLIENT_ID),
      client_secret_(GEMINI_CLIENT_SECRET),
      context_(context),
      url_loader_factory_(
          content::BrowserContext::GetDefaultStoragePartition(context_)
              ->GetURLLoaderFactoryForBrowserProcess()),
      weak_factory_(this) {
  LoadTokensFromPrefs();
}

GeminiService::~GeminiService() {
}

bool GeminiService::LoadTokensFromPrefs() {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  std::string encoded_encrypted_access_token =
      prefs->GetString(kGeminiAccessToken);
  std::string encoded_encrypted_refresh_token =
      prefs->GetString(kGeminiRefreshToken);

  std::string encrypted_access_token;
  std::string encrypted_refresh_token;
  if (!base::Base64Decode(encoded_encrypted_access_token,
                          &encrypted_access_token) ||
      !base::Base64Decode(encoded_encrypted_refresh_token,
                          &encrypted_refresh_token)) {
    LOG(ERROR) << "Could not decode Gemini Token info";
    return false;
  }

  if (!OSCrypt::DecryptString(encrypted_access_token, &access_token_)) {
    LOG(ERROR) << "Could not decrypt and save Gemini access token";
    return false;
  }
  if (!OSCrypt::DecryptString(encrypted_refresh_token, &refresh_token_)) {
    LOG(ERROR) << "Could not decrypt and save Gemini refresh token";
    return false;
  }

  return true;
}
