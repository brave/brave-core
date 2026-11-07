// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_INFOBARS_WAYBACK_MACHINE_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_WAYBACK_MACHINE_INFOBAR_DELEGATE_H_

#include <cstddef>
#include <string>

#include "base/functional/callback.h"
#include "base/timer/timer.h"
#include "brave/components/brave_wayback_machine/wayback_snapshot_info.h"
#include "components/infobars/core/infobar_delegate.h"

namespace content {
class WebContents;
}

// Delegate for the infobar shown when an automatic check finds a snapshot of a
// missing page. Counts down and then loads the snapshot.
class WaybackMachineInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  // Adds the infobar for |snapshot_info| to |contents|, unless one is already
  // showing.
  static void Create(content::WebContents* contents,
                     const WaybackSnapshotInfo& snapshot_info);

  // Asynchronously removes the infobar from |contents|, if it is showing.
  static void Remove(content::WebContents* contents);

  WaybackMachineInfoBarDelegate(const WaybackMachineInfoBarDelegate&) = delete;
  WaybackMachineInfoBarDelegate& operator=(
      const WaybackMachineInfoBarDelegate&) = delete;
  ~WaybackMachineInfoBarDelegate() override;

  // The number of seconds counted down before the snapshot is loaded.
  static constexpr int kCountdownSeconds = 5;

  // Returns the snapshot this infobar is about to load.
  const WaybackSnapshotInfo& snapshot_info() const { return snapshot_info_; }

  // Returns the number of seconds until the snapshot is loaded.
  int seconds_remaining() const { return seconds_remaining_; }

  // Sets a callback that is run whenever |seconds_remaining()| decreases.
  void SetCountdownChangedCallback(base::RepeatingClosure callback);

  // Fires once per second of the countdown.
  base::OneShotTimer& countdown_timer_for_testing() { return countdown_timer_; }

  // Returns the heading text for the infobar.
  std::u16string GetTitleText() const;

  // Returns the body text for the infobar and sets |learn_more_offset| to the
  // start offset of the inline "Learn more." link.
  std::u16string GetDescriptionText(size_t* learn_more_offset) const;

  // Returns the inline "Learn more." link text.
  std::u16string GetLearnMoreText() const;

  // Returns the label for the primary countdown/redirect button.
  std::u16string GetRedirectButtonText() const;

  // Returns the label for the cancel button.
  std::u16string GetCancelButtonText() const;

  // Loads the snapshot immediately.
  void Accept();

  // Stops the countdown.
  void Cancel();

  // Opens the help page in a new tab and stops the countdown.
  void OpenLearnMore();

  // infobars::InfoBarDelegate:
  InfoBarIdentifier GetIdentifier() const override;
  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override;
  bool IsCloseable() const override;

 private:
  explicit WaybackMachineInfoBarDelegate(
      const WaybackSnapshotInfo& snapshot_info);

  void StartCountdownTimer();
  void OnCountdownTick();
  void NavigateToSnapshot();

  const WaybackSnapshotInfo snapshot_info_;
  int seconds_remaining_ = kCountdownSeconds;
  base::OneShotTimer countdown_timer_;
  base::RepeatingClosure countdown_changed_callback_;
};

#endif  // BRAVE_BROWSER_INFOBARS_WAYBACK_MACHINE_INFOBAR_DELEGATE_H_
