// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_INFOBAR_DELEGATE_H_

#include <string>

#include "base/auto_reset.h"
#include "base/callback_list.h"
#include "base/functional/callback.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_wayback_machine/wayback_state.h"
#include "components/infobars/core/infobar_delegate.h"

class BraveWaybackMachineTabHelper;

namespace content {
class WebContents;
}

// Delegate for the infobar shown when an automatic check finds an archived
// version of a missing page. Counts down and then loads the archived version.
// The infobar is removed when the wayback state leaves kFound.
class WaybackMachineInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  static constexpr int kCountdownSeconds = 5;

  // Adds the infobar to |contents|. The contents' wayback state must be
  // kFound.
  static void Create(content::WebContents* contents);

  // Overrides the one second interval between countdown ticks.
  [[nodiscard]] static base::AutoReset<base::TimeDelta>
  SetCountdownTickIntervalForTesting(base::TimeDelta interval);

  WaybackMachineInfoBarDelegate(const WaybackMachineInfoBarDelegate&) = delete;
  WaybackMachineInfoBarDelegate& operator=(
      const WaybackMachineInfoBarDelegate&) = delete;
  ~WaybackMachineInfoBarDelegate() override;

  // Returns the time of the archived version. Null if unknown.
  base::Time snapshot_time() const { return snapshot_time_; }

  // Returns the number of seconds until the archived version is loaded.
  int seconds_remaining() const { return seconds_remaining_; }

  // Sets a callback that is run whenever |seconds_remaining()| changes.
  void SetCountdownChangedCallback(base::RepeatingClosure callback);

  // Loads the archived version immediately and removes the infobar. May delete
  // |this|.
  void LoadNow();

  // Removes the infobar without loading the archived version. May delete
  // |this|.
  void Dismiss();

  // infobars::InfoBarDelegate:
  InfoBarIdentifier GetIdentifier() const override;
  ui::ImageModel GetIcon() const override;
  std::u16string GetLinkText() const override;
  GURL GetLinkURL() const override;
  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override;
  bool IsCloseable() const override;

 private:
  explicit WaybackMachineInfoBarDelegate(
      BraveWaybackMachineTabHelper& tab_helper);

  BraveWaybackMachineTabHelper* GetTabHelper();
  void OnCountdownTick();
  void OnWaybackStateChanged(WaybackState state);

  const base::Time snapshot_time_;
  int seconds_remaining_ = kCountdownSeconds;
  base::RepeatingTimer countdown_timer_;
  base::RepeatingClosure countdown_changed_callback_;
  base::CallbackListSubscription wayback_state_changed_subscription_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_INFOBAR_DELEGATE_H_
