/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_BROWSER_COMMAND_CONTROLLER_H_
#define BRAVE_BROWSER_UI_BRAVE_BROWSER_COMMAND_CONTROLLER_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "components/prefs/pref_change_registrar.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/browser/brave_vpn_service_observer.h"
#endif

class BraveAppMenuBrowserTest;
class BraveAppMenuModelBrowserTest;
class BraveBrowserCommandControllerTest;
enum class TabChangeType;

namespace content {
class WebContents;
}

// This namespace is needed for a chromium_src override
namespace chrome {

class BraveBrowserCommandController : public chrome::BrowserCommandController
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    ,
                                      public brave_vpn::BraveVPNServiceObserver
#endif
{
 public:
  explicit BraveBrowserCommandController(Browser* browser);
  BraveBrowserCommandController(const BraveBrowserCommandController&) = delete;
  BraveBrowserCommandController& operator=(
      const BraveBrowserCommandController&) = delete;
  ~BraveBrowserCommandController() override;

#if BUILDFLAG(ENABLE_TOR)
  void UpdateCommandForTor();
#endif

 protected:
  void TabChangedAt(content::WebContents* contents,
                    int index,
                    TabChangeType change) override;
  void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int index) override;
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

 private:
  friend class ::BraveAppMenuBrowserTest;
  friend class ::BraveAppMenuModelBrowserTest;
  friend class ::BraveBrowserCommandControllerTest;

  // Overriden from CommandUpdater:
  bool SupportsCommand(int id) const override;
  bool IsCommandEnabled(int id) const override;
  bool ExecuteCommandWithDisposition(
      int id,
      WindowOpenDisposition disposition,
      base::TimeTicks time_stamp = base::TimeTicks::Now()) override;
  void AddCommandObserver(int id, CommandObserver* observer) override;
  void RemoveCommandObserver(int id, CommandObserver* observer) override;
  void RemoveCommandObserver(CommandObserver* observer) override;
  bool UpdateCommandEnabled(int id, bool state) override;

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  // brave_vpn::BraveVPNServiceObserver overrides:
  void OnPurchasedStateChanged(
      brave_vpn::mojom::PurchasedState state,
      const absl::optional<std::string>& description) override;
#endif

  void InitBraveCommandState();
  void UpdateCommandForBraveRewards();
  void UpdateCommandForWebcompatReporter();
  void UpdateCommandForBraveSync();
  void UpdateCommandForBraveWallet();
  void UpdateCommandForSidebar();
  void UpdateCommandForBraveVPN();
  void UpdateCommandForPlaylist();
  void UpdateCommandsFroGroups();
  void UpdateCommandsForMute();
  void UpdateCommandsForSend();
  void UpdateCommandsForPin();

  bool ExecuteBraveCommandWithDisposition(int id,
                                          WindowOpenDisposition disposition,
                                          base::TimeTicks time_stamp);
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  PrefChangeRegistrar brave_vpn_pref_change_registrar_;
#endif
  const raw_ref<Browser> browser_;

  CommandUpdaterImpl brave_command_updater_;
};

}  // namespace chrome

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_COMMAND_CONTROLLER_H_
