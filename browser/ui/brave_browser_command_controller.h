/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_BROWSER_COMMAND_CONTROLLER_H_
#define BRAVE_BROWSER_UI_BRAVE_BROWSER_COMMAND_CONTROLLER_H_

#include "chrome/browser/ui/browser_command_controller.h"

// This namespace is needed for a chromium_src override
namespace chrome {

class BraveBrowserCommandController : public chrome::BrowserCommandController {
 public:
  explicit BraveBrowserCommandController(Browser* browser);

 private:
  // Overriden from CommandUpdater:
  bool SupportsCommand(int id) const override;
  bool IsCommandEnabled(int id) const override;
  bool ExecuteCommandWithDisposition(int id, WindowOpenDisposition disposition)
      override;
  void AddCommandObserver(int id, CommandObserver* observer) override;
  void RemoveCommandObserver(int id, CommandObserver* observer) override;
  void RemoveCommandObserver(CommandObserver* observer) override;
  bool UpdateCommandEnabled(int id, bool state) override;

  void InitBraveCommandState();
  void UpdateCommandForBraveRewards();
  void UpdateCommandForBraveAdblock();
  void UpdateCommandForTor();
  void UpdateCommandForBraveSync();

  bool ExecuteBraveCommandWithDisposition(int id,
                                          WindowOpenDisposition disposition);

  Browser* const browser_;

  CommandUpdaterImpl brave_command_updater_;

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserCommandController);
};

}   // namespace chrome

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_COMMAND_CONTROLLER_H_
