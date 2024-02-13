/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "chrome/browser/extensions/api/identity/identity_get_auth_token_function.h"
#include "chrome/browser/extensions/api/identity/identity_token_cache.h"
#include "google_apis/google_api_keys.h"

// Use the embedded Google OAuth flow only if Google Chrome API key is used.
// Otherwise, fallback to the web OAuth flow.
#define BRAVE_RUN                                                           \
  if (!google_apis::IsGoogleChromeAPIKeyUsed()) {                           \
    StartMintTokenFlow(IdentityMintRequestQueue::MINT_TYPE_NONINTERACTIVE); \
    return RespondLater();                                                  \
  }

// clang-format off
#define BRAVE_START_MINT_TOKEN_FLOW_IF \
  if (google_apis::IsGoogleChromeAPIKeyUsed()) {
#define BRAVE_START_MINT_TOKEN_FLOW_ELSE       \
  } else {                                     \
    DCHECK(token_key_.account_info.IsEmpty()); \
  }
// clang-format on

#define CacheValueStatus                                                       \
  CreateRemoteConsentApproved("placeholder");                                  \
  if (cache_entry.status() ==                                                  \
      IdentityTokenCacheValue::CACHE_STATUS_NOTFOUND) {                        \
    if (type == IdentityMintRequestQueue::MINT_TYPE_INTERACTIVE) {             \
      std::optional<api::identity::GetAuthToken::Params> params(               \
          api::identity::GetAuthToken::Params::Create(args()));                \
      /* Forcing interactive mode if initial caller requested it. */           \
      bool interactive =                                                       \
          (params->details && params->details->interactive.value_or(false)) || \
          IsInteractionAllowed(interactivity_status_for_signin_);              \
      if (!google_apis::IsGoogleChromeAPIKeyUsed()) {                          \
        StartWebAuthFlow(                                                      \
            GetProfile(),                                                      \
            base::BindOnce(                                                    \
                &IdentityGetAuthTokenFunction::CompleteMintTokenFlow,          \
                weak_ptr_factory_.GetWeakPtr()),                               \
            base::BindOnce(                                                    \
                &IdentityGetAuthTokenFunction::CompleteFunctionWithError,      \
                weak_ptr_factory_.GetWeakPtr()),                               \
            base::BindOnce(                                                    \
                &IdentityGetAuthTokenFunction::CompleteFunctionWithResult,     \
                weak_ptr_factory_.GetWeakPtr()),                               \
            oauth2_client_id_, token_key_, interactive, user_gesture());       \
        return;                                                                \
      }                                                                        \
    } else {                                                                   \
      if (!google_apis::IsGoogleChromeAPIKeyUsed()) {                          \
        CompleteMintTokenFlow();                                               \
        /* Forcing interactive mode to try WebAuthFlow interactively. */       \
        interactivity_status_for_consent_ =                                    \
            InteractivityStatus::kAllowedWithActivity;                         \
        StartMintTokenFlow(IdentityMintRequestQueue::MINT_TYPE_INTERACTIVE);   \
        return;                                                                \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  IdentityTokenCacheValue::CacheValueStatus
#include "src/chrome/browser/extensions/api/identity/identity_get_auth_token_function.cc"
#undef CacheValueStatus
#undef BRAVE_START_MINT_TOKEN_FLOW_ELSE
#undef BRAVE_START_MINT_TOKEN_FLOW_IF
#undef BRAVE_RUN
