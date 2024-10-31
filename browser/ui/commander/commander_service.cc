// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commander/commander_service.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/commander/bookmark_command_source.h"
#include "brave/browser/ui/commander/command_source.h"
#include "brave/browser/ui/commander/ranker.h"
#include "brave/browser/ui/commander/simple_command_source.h"
#include "brave/browser/ui/commander/tab_command_source.h"
#include "brave/browser/ui/commander/window_command_source.h"
#include "brave/components/commander/browser/commander_frontend_delegate.h"
#include "brave/components/commander/browser/commander_item_model.h"
#include "brave/components/commander/common/constants.h"
#include "brave/components/commander/common/features.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "components/omnibox/browser/omnibox_view.h"

namespace commander {
namespace {
constexpr size_t kMaxResults = 8;
CommandItemModel FromCommand(const std::unique_ptr<CommandItem>& item) {
  return CommandItemModel(item->title, item->matched_ranges, item->annotation);
}
}  // namespace

bool IsEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveCommander);
}

CommanderService::CommanderService(Profile* profile)
    : profile_(profile), ranker_(profile->GetPrefs()) {
  command_sources_.push_back(std::make_unique<SimpleCommandSource>());
  command_sources_.push_back(std::make_unique<BookmarkCommandSource>());
  command_sources_.push_back(std::make_unique<WindowCommandSource>());
  command_sources_.push_back(std::make_unique<TabCommandSource>());
}

CommanderService::~CommanderService() = default;

void CommanderService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
  observer->OnCommanderUpdated();
}

void CommanderService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void CommanderService::UpdateText(const std::u16string& text) {
  UpdateText(text, false);
}

void CommanderService::SelectCommand(uint32_t command_index,
                                     uint32_t result_set_id) {
  if (command_index >= items_.size() ||
      result_set_id != current_result_set_id_) {
    return;
  }

  // Increment the current result set id - we don't want any commands from this
  // set to be reused after we've selected a command.
  // Note: This needs to happen before beginning a composite command, to ensure
  // that the generated model uses right right |result_set_id|.
  current_result_set_id_++;

  auto* item = items_[command_index].get();

  // Record that we selected this command to increase it's rank next time.
  ranker_.Visit(*item);

  if (item->GetType() == CommandItem::Type::kOneShot) {
    std::move(absl::get<base::OnceClosure>(item->command)).Run();
    Hide();
  } else {
    auto composite_command =
        absl::get<CommandItem::CompositeCommand>(item->command);
    std::tie(prompt_, composite_command_provider_) = composite_command;
    Show();
  }
}

std::vector<CommandItemModel> CommanderService::GetItems() {
  std::vector<CommandItemModel> result;
  base::ranges::transform(items_, std::back_inserter(result), FromCommand);
  return result;
}

int CommanderService::GetResultSetId() {
  return current_result_set_id_;
}

const std::u16string& CommanderService::GetPrompt() {
  return prompt_;
}

void CommanderService::Shutdown() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  items_.clear();
}

void CommanderService::OnBrowserClosing(Browser* browser) {
  if (last_browser_ == browser) {
    last_browser_ = nullptr;
    browser_list_observation_.Reset();
  }
}

void CommanderService::Toggle() {
  if (IsShowing()) {
    Hide();
  } else {
    Show();
  }
}

void CommanderService::Show() {
  // Note: This posts a task because we can't change the Omnibox text while
  // autocompleting.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&CommanderService::ShowCommander,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CommanderService::Hide() {
  // Note: This posts a task because we can't change the Omnibox text while
  // autocompleting.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&CommanderService::HideCommander,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CommanderService::Reset() {
  current_result_set_id_++;
  items_.clear();
  prompt_.clear();
  last_searched_.clear();
  last_browser_ = nullptr;
  if (composite_command_provider_) {
    composite_command_provider_.Reset();
  }
  NotifyObservers();
}

void CommanderService::UpdateTextFromCurrentBrowserOmnibox() {
  auto* browser = chrome::FindLastActiveWithProfile(profile_);

  // The last active browser can have no tabs, if we're in the process of moving
  // the last tab from the current window into another one.
  if (!browser || browser->tab_strip_model()->empty()) {
    return;
  }

  auto* window = browser->window();
  CHECK(window);

  auto text = window->GetLocationBar()->GetOmniboxView()->GetText();
  UpdateText(text, /*force=*/true);
}

void CommanderService::UpdateText(const std::u16string& text, bool force) {
  auto* browser = chrome::FindLastActiveWithProfile(profile_);
  if (!browser) {
    return;
  }

  auto has_prefix = text.starts_with(kCommandPrefix);
  if (!has_prefix && !browser->profile()->GetPrefs()->GetBoolean(
                         omnibox::kCommanderSuggestionsEnabled)) {
    return;
  }

  if (text.empty()) {
    return;
  }

  std::u16string trimmed_text(
      has_prefix ? base::TrimWhitespace(text.substr(kCommandPrefix.size()),
                                        base::TRIM_LEADING)
                 : text);

  // If nothing has changed (and we aren't forcing things), don't update the
  // commands.
  if (trimmed_text == last_searched_ && browser == last_browser_ && !force) {
    return;
  }
  last_searched_ = trimmed_text;
  last_browser_ = browser;
  if (!browser_list_observation_.IsObserving()) {
    browser_list_observation_.Observe(BrowserList::GetInstance());
  }

  UpdateCommands();
}

OmniboxView* CommanderService::GetOmnibox() const {
  auto* browser = chrome::FindLastActiveWithProfile(profile_);
  if (!browser) {
    return nullptr;
  }

  auto* window = browser->window();
  CHECK(window);
  return window->GetLocationBar()->GetOmniboxView();
}

bool CommanderService::IsShowing() const {
  auto* omnibox = GetOmnibox();
  return omnibox && omnibox->GetText().starts_with(kCommandPrefix.data());
}

void CommanderService::UpdateCommands() {
  std::vector<std::unique_ptr<CommandItem>> items;
  if (composite_command_provider_) {
    items = composite_command_provider_.Run(last_searched_);
  } else {
    for (auto& source : command_sources_) {
      auto commands = source->GetCommands(last_searched_, last_browser_);
      items.insert(items.end(), std::make_move_iterator(commands.begin()),
                   std::make_move_iterator(commands.end()));
    }
  }

  ranker_.Rank(items, kMaxResults);
  if (items.size() > kMaxResults) {
    items.resize(kMaxResults);
  }
  items_ = std::move(items);

  // Increment the current result set id, so we don't confuse these results with
  // a prior set before notifying observers.
  current_result_set_id_++;
  NotifyObservers();
}

void CommanderService::NotifyObservers() {
  for (auto& observer : observers_) {
    observer.OnCommanderUpdated();
  }
}

void CommanderService::ShowCommander() {
  if (auto* omnibox = GetOmnibox()) {
    omnibox->SetFocus(true);

    auto text = base::StrCat({commander::kCommandPrefix, u" "});
    omnibox->SetUserText(text);
    omnibox->SetCaretPos(text.size());
    UpdateTextFromCurrentBrowserOmnibox();
  }
}

void CommanderService::HideCommander() {
  Reset();

  if (auto* omnibox = GetOmnibox(); omnibox && IsShowing()) {
    omnibox->RevertAll();
  }
}

}  // namespace commander
