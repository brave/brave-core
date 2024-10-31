/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/utils.h"

#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/task/bind_post_task.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/url_constants.h"

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
#include "brave/components/text_recognition/browser/text_recognition.h"
#endif

namespace ai_chat {

namespace {

const base::flat_map<mojom::ActionType, std::string>&
GetActionTypeQuestionMap() {
  static const base::NoDestructor<
      base::flat_map<mojom::ActionType, std::string>>
      kMap({{mojom::ActionType::SUMMARIZE_PAGE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE)},
            {mojom::ActionType::SUMMARIZE_VIDEO,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_VIDEO)},
            {mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
             l10n_util::GetStringUTF8(
                 IDS_AI_CHAT_QUESTION_SUMMARIZE_SELECTED_TEXT)},
            {mojom::ActionType::EXPLAIN,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_EXPLAIN)},
            {mojom::ActionType::PARAPHRASE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_PARAPHRASE)},
            {mojom::ActionType::CREATE_TAGLINE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_CREATE_TAGLINE)},
            {mojom::ActionType::CREATE_SOCIAL_MEDIA_COMMENT_SHORT,
             l10n_util::GetStringUTF8(
                 IDS_AI_CHAT_QUESTION_CREATE_SOCIAL_MEDIA_COMMENT_SHORT)},
            {mojom::ActionType::CREATE_SOCIAL_MEDIA_COMMENT_LONG,
             l10n_util::GetStringUTF8(
                 IDS_AI_CHAT_QUESTION_CREATE_SOCIAL_MEDIA_COMMENT_LONG)},
            {mojom::ActionType::IMPROVE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_IMPROVE)},
            {mojom::ActionType::PROFESSIONALIZE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_PROFESSIONALIZE)},
            {mojom::ActionType::PERSUASIVE_TONE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_PERSUASIVE_TONE)},
            {mojom::ActionType::CASUALIZE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_CASUALIZE)},
            {mojom::ActionType::FUNNY_TONE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_FUNNY_TONE)},
            {mojom::ActionType::ACADEMICIZE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_ACADEMICIZE)},
            {mojom::ActionType::SHORTEN,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SHORTEN)},
            {mojom::ActionType::EXPAND,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_EXPAND)}});
  return *kMap;
}

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
         url.path_piece() == "/search" && url.query_piece().starts_with("q=");
}

bool IsPremiumStatus(mojom::PremiumStatus status) {
  return status == mojom::PremiumStatus::Active ||
         status == mojom::PremiumStatus::ActiveDisconnected;
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

const std::string& GetActionTypeQuestion(mojom::ActionType action_type) {
  const auto& map = GetActionTypeQuestionMap();
  auto iter = map.find(action_type);
  CHECK(iter != map.end());
  return iter->second;
}

EngineConsumer::GenerationDataCallback BindParseRewriteReceivedData(
    ConversationHandler::GeneratedTextCallback callback) {
  return base::BindRepeating(
      [](ConversationHandler::GeneratedTextCallback callback,
         mojom::ConversationEntryEventPtr rewrite_event) {
        // TODO(petemill): This probably should exist at the EngineConsumer
        // level and possibly only for the OAI engine since the others use
        // stop sequences to exclude the ending tag.
        constexpr char kResponseTagPattern[] =
            "<\\/?(response|respons|respon|respo|resp|res|re|r)?$";
        if (!rewrite_event->is_completion_event()) {
          return;
        }

        std::string suggestion =
            rewrite_event->get_completion_event()->completion;

        base::TrimWhitespaceASCII(suggestion, base::TRIM_ALL, &suggestion);
        if (suggestion.empty()) {
          return;
        }

        // Avoid showing the ending tag.
        if (RE2::PartialMatch(suggestion, kResponseTagPattern)) {
          return;
        }

        callback.Run(suggestion);
      },
      std::move(callback));
}

}  // namespace ai_chat
