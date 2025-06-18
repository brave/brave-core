/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/constants.h"

#include <utility>

#include "base/strings/string_util.h"
#include "components/grit/brave_components_strings.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

std::vector<mojom::ActionGroupPtr> GetActionMenuList() {
  std::vector<mojom::ActionGroupPtr> action_list;

  {
    std::vector<mojom::ActionEntryPtr> entries;
    mojom::ActionGroupPtr group = mojom::ActionGroup::New(
        l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_QUICK_ACTIONS),
        std::move(entries));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_EXPLAIN),
            mojom::ActionType::EXPLAIN)));

    action_list.push_back(std::move(group));
  }

  {
    std::vector<mojom::ActionEntryPtr> entries;
    mojom::ActionGroupPtr group = mojom::ActionGroup::New(
        l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_REWRITE),
        std::move(entries));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_PARAPHRASE),
            mojom::ActionType::PARAPHRASE)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_IMPROVE),
            mojom::ActionType::IMPROVE)));

    // Subheading
    auto change_tone_subheading =
        l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_CHANGE_TONE);
    group->entries.push_back(
        mojom::ActionEntry::NewSubheading(change_tone_subheading));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_ACADEMICIZE)},
                " / "),
            mojom::ActionType::ACADEMICIZE)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_PROFESSIONALIZE)},
                " / "),
            mojom::ActionType::PROFESSIONALIZE)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_PERSUASIVE_TONE)},
                " / "),
            mojom::ActionType::PERSUASIVE_TONE)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_CASUALIZE)},
                " / "),
            mojom::ActionType::CASUALIZE)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_FUNNY_TONE)},
                " / "),
            mojom::ActionType::FUNNY_TONE)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_SHORTEN)},
                " / "),
            mojom::ActionType::SHORTEN)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_EXPAND)},
                " / "),
            mojom::ActionType::EXPAND)));

    action_list.push_back(std::move(group));
  }

  {
    std::vector<mojom::ActionEntryPtr> entries;
    mojom::ActionGroupPtr group = mojom::ActionGroup::New(
        l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_CREATE),
        std::move(entries));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_CREATE_TAGLINE),
            mojom::ActionType::CREATE_TAGLINE)));

    // Subheading
    auto social_media_subheading =
        l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_POST);
    group->entries.push_back(
        mojom::ActionEntry::NewSubheading(social_media_subheading));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {social_media_subheading,
                 l10n_util::GetStringUTF8(
                     IDS_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_SHORT)},
                " / "),
            mojom::ActionType::CREATE_SOCIAL_MEDIA_COMMENT_SHORT)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {social_media_subheading,
                 l10n_util::GetStringUTF8(
                     IDS_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_LONG)},
                " / "),
            mojom::ActionType::CREATE_SOCIAL_MEDIA_COMMENT_LONG)));

    action_list.push_back(std::move(group));
  }

  return action_list;
}
}  // namespace ai_chat
