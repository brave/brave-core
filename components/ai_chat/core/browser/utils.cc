/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/utils.h"

#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/task/bind_post_task.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "url/url_constants.h"

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
#include "brave/components/text_recognition/browser/text_recognition.h"
#endif

namespace ai_chat {

namespace {

bool IsDisabledByPolicy(PrefService* prefs) {
  DCHECK(prefs);
  return prefs->IsManagedPreference(prefs::kEnabledByPolicy) &&
         !prefs->GetBoolean(prefs::kEnabledByPolicy);
}

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
void OnGetTextFromImage(
    GetOCRTextCallback callback,
    const std::pair<bool, std::vector<std::string>>& supported_strs) {
  if (!supported_strs.first) {
    std::move(callback).Run("");
    return;
  }

  std::stringstream ss;
  auto& strs = supported_strs.second;
  for (size_t i = 0; i < strs.size(); ++i) {
    ss << base::TrimWhitespaceASCII(strs[i], base::TrimPositions::TRIM_ALL);
    if (i < strs.size() - 1) {
      ss << "\n";
    }
  }
  std::move(callback).Run(ss.str());
}
#endif

}  // namespace

bool IsAIChatEnabled(PrefService* prefs) {
  DCHECK(prefs);
  return features::IsAIChatEnabled() && !IsDisabledByPolicy(prefs);
}

bool HasUserOptedIn(PrefService* prefs) {
  if (!prefs) {
    return false;
  }

  base::Time last_accepted_disclaimer =
      prefs->GetTime(prefs::kLastAcceptedDisclaimer);
  return !last_accepted_disclaimer.is_null();
}

void SetUserOptedIn(PrefService* prefs, bool opted_in) {
  if (!prefs) {
    return;
  }

  if (opted_in) {
    prefs->SetTime(prefs::kLastAcceptedDisclaimer, base::Time::Now());
  } else {
    prefs->ClearPref(prefs::kLastAcceptedDisclaimer);
  }
}

bool IsBraveSearchSERP(const GURL& url) {
  if (!url.is_valid()) {
    return false;
  }

  // https://search.brave.com/search?q=test
  return url.SchemeIs(url::kHttpsScheme) &&
         url.host_piece() ==
             brave_domains::GetServicesDomain(kBraveSearchURLPrefix) &&
         url.path_piece() == "/search" &&
         base::StartsWith(url.query_piece(), "q=");
}

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
void GetOCRText(const SkBitmap& image, GetOCRTextCallback callback) {
#if BUILDFLAG(IS_MAC)
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&text_recognition::GetTextFromImage, image),
      base::BindOnce(&OnGetTextFromImage, std::move(callback)));
#endif
#if BUILDFLAG(IS_WIN)
  const std::string& locale = brave_l10n::GetDefaultLocaleString();
  const std::string language_code = brave_l10n::GetISOLanguageCode(locale);
  base::ThreadPool::CreateCOMSTATaskRunner({base::MayBlock()})
      ->PostTask(FROM_HERE,
                 base::BindOnce(
                     &text_recognition::GetTextFromImage, language_code, image,
                     base::BindPostTaskToCurrentDefault(base::BindOnce(
                         &OnGetTextFromImage, std::move(callback)))));

#endif
}
#endif

}  // namespace ai_chat
