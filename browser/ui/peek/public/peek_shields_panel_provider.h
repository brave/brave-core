/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_PEEK_PUBLIC_PEEK_SHIELDS_PANEL_PROVIDER_H_
#define BRAVE_BROWSER_UI_PEEK_PUBLIC_PEEK_SHIELDS_PANEL_PROVIDER_H_

// Opens the window's own Shields panel, so that Peek doesn't depend on the
// browser view or the Shields UI.
class PeekShieldsPanelProvider {
 public:
  virtual ~PeekShieldsPanelProvider() = default;

  // Shows the Shields panel for the window's active tab, as if its Shields
  // button was pressed. Returns false if the window has no Shields button.
  virtual bool ShowShieldsPanelForActiveTab() = 0;
};

#endif  // BRAVE_BROWSER_UI_PEEK_PUBLIC_PEEK_SHIELDS_PANEL_PROVIDER_H_
