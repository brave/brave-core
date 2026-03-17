// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_PRESENTER_H_
#define BRAVE_BROWSER_PSST_PSST_UI_PRESENTER_H_

#include "brave/browser/psst/psst_infobar_delegate.h"

namespace psst {

class PsstUiPresenter {
 public:
  virtual ~PsstUiPresenter() = default;

  virtual void ShowInfoBar(
      PsstInfoBarDelegate::AcceptCallback on_accept_callback) = 0;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_PRESENTER_H_
