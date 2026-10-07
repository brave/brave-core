/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/api/identity/identity_get_auth_token_function.h"

#include <optional>

#include "base/check.h"
#include "chrome/browser/extensions/api/identity/identity_token_cache.h"
#include "google_apis/google_api_keys.h"

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
#include <chrome/browser/extensions/api/identity/identity_get_auth_token_function.cc>
#undef CacheValueStatus
