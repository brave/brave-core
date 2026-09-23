/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_CONFIRM_INFOBAR_CREATOR_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_CONFIRM_INFOBAR_CREATOR_H_

#include <memory>

class BraveConfirmInfoBarDelegate;

namespace infobars {
class InfoBar;
}  // namespace infobars

std::unique_ptr<infobars::InfoBar> CreateBraveConfirmInfoBar(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate);

#endif  // BRAVE_BROWSER_INFOBARS_BRAVE_CONFIRM_INFOBAR_CREATOR_H_
