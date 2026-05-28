// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_PRESENTER_H_
#define BRAVE_BROWSER_PSST_PSST_UI_PRESENTER_H_

#include "base/functional/callback_forward.h"
#include "url/origin.h"

namespace psst {

enum class LocationBarIconStatus {
  kOnlyIcon,
  kIconWithBadge,
  kHidden,
};

// Interface for presenting PSST UI elements.
class PsstUiPresenter {
 public:
  using InfoBarCallback = base::OnceCallback<void(bool)>;
  using LocationBarMenuCallback = base::OnceCallback<void()>;

  virtual ~PsstUiPresenter() = default;

  virtual void SetLocationBarIconStatus(
      LocationBarIconStatus status,
      LocationBarMenuCallback on_dont_show_this_site_callback,
      LocationBarMenuCallback on_disable_privacy_settings_tuning_callback) = 0;
  virtual void ShowInfoBar(InfoBarCallback on_accept_callback) = 0;
  virtual void HideInfoBar() = 0;
  virtual void ShowConsentDialog() = 0;
  virtual bool IsDialogShown() const = 0;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_PRESENTER_H_
