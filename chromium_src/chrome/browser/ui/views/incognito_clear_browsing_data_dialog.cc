/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/ui/views/incognito_clear_browsing_data_dialog.cc"

BraveIncognitoClearBrowsingDataDialog::BraveIncognitoClearBrowsingDataDialog(
    views::View* anchor_view,
    Profile* incognito_profile,
    Type type)
    : IncognitoClearBrowsingDataDialog(anchor_view, incognito_profile, type) {
  SetShowCloseButton(false);
}

// static
void BraveIncognitoClearBrowsingDataDialog::Show(views::View* anchor_view,
                                                 Profile* incognito_profile,
                                                 Type type) {
  g_incognito_cbd_dialog = new BraveIncognitoClearBrowsingDataDialog(
      anchor_view, incognito_profile, type);
  views::Widget* const widget =
      BubbleDialogDelegateView::CreateBubble(g_incognito_cbd_dialog);
  widget->Show();
}
