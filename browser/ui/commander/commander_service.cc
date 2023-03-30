// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commander/commander_service.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/commander/common/commander_frontend_delegate.h"
#include "brave/components/commander/common/commander_item_model.h"
#include "brave/components/commander/common/constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/commander/bookmark_command_source.h"
#include "chrome/browser/ui/commander/command_source.h"
#include "chrome/browser/ui/commander/commander.h"
#include "chrome/browser/ui/commander/commander_view_model.h"
#include "chrome/browser/ui/commander/open_url_command_source.h"
#include "chrome/browser/ui/commander/simple_command_source.h"
#include "chrome/browser/ui/commander/tab_command_source.h"
#include "chrome/browser/ui/commander/window_command_source.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/omnibox/browser/omnibox_view.h"

namespace commander {
namespace {
constexpr size_t kMaxResults = 8;
CommandItemModel FromCommand(const std::unique_ptr<CommandItem>& item) {
  return CommandItemModel(item->title, item->matched_ranges, item->annotation);
}
}  // namespace

CommanderService::CommanderService(Profile* profile) : profile_(profile) {
  command_sources_.push_back(std::make_unique<SimpleCommandSource>());
  command_sources_.push_back(std::make_unique<OpenURLCommandSource>());
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

void CommanderService::UpdateText(bool force) {
  auto* browser = chrome::FindLastActiveWithProfile(profile_);

  // The last active browser can have no tabs, if we're in the process of moving
  // the last tab from the current window into another one.
  if (!browser || browser->tab_strip_model()->empty()) {
    return;
  }

  auto text = browser->window()->GetLocationBar()->GetOmniboxView()->GetText();
  if (!base::StartsWith(text, kCommandPrefix)) {
    return;
  }

  std::u16string trimmed_text(base::TrimWhitespace(
      text.substr(kCommandPrefixLength), base::TRIM_LEADING));

  // If nothing has changed (and we aren't forcing things), don't update the
  // commands.
  if (trimmed_text == last_searched_ && browser == last_browser_ && !force) {
    return;
  }
  last_searched_ = trimmed_text;
  last_browser_ = browser;

  UpdateCommands();
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
  // that the generated model uses right right result_set_id.
  current_result_set_id_++;

  auto* item = items_[command_index].get();
  if (item->GetType() == CommandItem::Type::kOneShot) {
    std::move(absl::get<base::OnceClosure>(item->command)).Run();
    Hide();
  } else {
    auto composite_command =
        absl::get<CommandItem::CompositeCommand>(item->command);
    composite_command_provider_ = composite_command.second;
    prompt_ = composite_command.first;
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
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<CommanderService> service) {
                       if (auto* omnibox = service->GetOmnibox()) {
                         omnibox->SetFocus(true);

                         auto text =
                             commander::kCommandPrefix + std::u16string(u" ");
                         omnibox->SetUserText(text);
                         omnibox->SetCaretPos(text.size());
                         service->UpdateText(true);
                       }
                     },
                     weak_ptr_factory_.GetWeakPtr()));
}

void CommanderService::Hide() {
  // Note: This posts a task because we can't change the Omnibox text while
  // autocompleting.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<CommanderService> service) {
                       service->Reset();

                       if (auto* omnibox = service->GetOmnibox();
                           omnibox && service->IsShowing()) {
                         omnibox->RevertAll();
                       }
                     },
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

OmniboxView* CommanderService::GetOmnibox() const {
  auto* browser = chrome::FindLastActiveWithProfile(profile_);
  if (!browser) {
    return nullptr;
  }
  return browser->window()->GetLocationBar()->GetOmniboxView();
}

bool CommanderService::IsShowing() const {
  return GetOmnibox() && GetOmnibox()->GetText().starts_with(kCommandPrefix);
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

  // Sort at most |kMaxResults| by score and then alphabetically.
  auto max_elements = std::min(items.size(), kMaxResults);
  std::partial_sort(
      std::begin(items), std::begin(items) + max_elements, std::end(items),
      [](const std::unique_ptr<CommandItem>& left,
         const std::unique_ptr<CommandItem>& right) {
        return (left->score == right->score) ? left->title < right->title
                                             : left->score > right->score;
      });
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

}  // namespace commander
