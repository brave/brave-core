// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_WINDOW_DIALOG_CLIENT_VIEW_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_WINDOW_DIALOG_CLIENT_VIEW_H_

class WebDiscoveryDialogClientView;

#define SetupLayout                            \
  SetupLayout_UnUsed() {}                      \
  friend class ::WebDiscoveryDialogClientView; \
  void SetupButtonsLayoutVertically();         \
  virtual void SetupLayout

#include "../../../../../ui/views/window/dialog_client_view.h"

#undef SetupLayout

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_WINDOW_DIALOG_CLIENT_VIEW_H_
