// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_COMMANDER_COMMANDER_SERVICE_H_
#define BRAVE_BROWSER_UI_COMMANDER_COMMANDER_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/commander/command_source.h"
#include "brave/browser/ui/commander/ranker.h"
#include "brave/components/commander/browser/commander_frontend_delegate.h"
#include "brave/components/commander/browser/commander_item_model.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "components/keyed_service/core/keyed_service.h"

class OmniboxView;
class Profile;
class BrowserList;

namespace commander {

// Returns true if the commander UI should be made available.
bool IsEnabled();

class CommanderService : public CommanderFrontendDelegate,
                         public KeyedService,
                         public BrowserListObserver {
 public:
  using CommandSources = std::vector<std::unique_ptr<CommandSource>>;

  explicit CommanderService(Profile* profile);
  CommanderService(const CommanderService&) = delete;
  CommanderService& operator=(const CommanderService&) = delete;
  ~CommanderService() override;

  void Show();
  void Reset();
  bool IsShowing() const;

  // CommanderFrontendDelegate:
  void Toggle() override;
  void Hide() override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  void UpdateText(const std::u16string& text) override;
  void SelectCommand(uint32_t command_index, uint32_t result_set_id) override;
  std::vector<CommandItemModel> GetItems() override;
  int GetResultSetId() override;
  const std::u16string& GetPrompt() override;

  // KeyedService:
  void Shutdown() override;

  // overrides BrowserListObserver:
  void OnBrowserClosing(Browser* browser) override;

 private:
  void UpdateTextFromCurrentBrowserOmnibox();
  void UpdateText(const std::u16string& text, bool force);

  OmniboxView* GetOmnibox() const;
  void UpdateCommands();
  void NotifyObservers();

  void ShowCommander();
  void HideCommander();

  CommandSources command_sources_;

  std::u16string last_searched_;
  std::u16string prompt_;
  std::vector<std::unique_ptr<CommandItem>> items_;
  uint32_t current_result_set_id_ = 0;
  raw_ptr<Browser, DanglingUntriaged> last_browser_;
  raw_ptr<Profile> profile_;

  // Some commands have multiple steps (like move tab to window, pick a
  // window). This allows commands to specify a command provider for a
  // subsequent step (in the window example, this would be a list of all
  // available windows).
  CommandItem::CompositeCommandProvider composite_command_provider_;

  Ranker ranker_;

  base::ObserverList<Observer> observers_;
  base::ScopedObservation<BrowserList, BrowserListObserver>
      browser_list_observation_{this};
  base::WeakPtrFactory<CommanderService> weak_ptr_factory_{this};
};
}  // namespace commander

#endif  // BRAVE_BROWSER_UI_COMMANDER_COMMANDER_SERVICE_H_
