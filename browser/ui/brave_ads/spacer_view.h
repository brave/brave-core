/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ADS_SPACER_VIEW_H_
#define BRAVE_BROWSER_UI_BRAVE_ADS_SPACER_VIEW_H_

namespace views {
class View;
}  // namespace views

namespace brave_ads {

views::View* CreateFlexibleSpacerView(const int spacing);
views::View* CreateFixedSizeSpacerView(const int spacing);

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_BRAVE_ADS_SPACER_VIEW_H_
