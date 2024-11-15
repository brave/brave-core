// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_ACTIONS_OMNIBOX_ACTION_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_ACTIONS_OMNIBOX_ACTION_H_

#include "brave/components/commander/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/components/commander/browser/commander_frontend_delegate.h"

#define OpenSharingHub  \
  OpenSharingHub() = 0; \
  virtual commander::CommanderFrontendDelegate* GetCommanderDelegate
#endif  // BUILDFLAG(ENABLE_COMMANDER)

#define NewIncognitoWindow                               \
  NewIncognitoWindow() = 0;                              \
  virtual void OpenLeo(const std::u16string& query) = 0; \
  virtual bool IsLeoProviderEnabled

#include "src/components/omnibox/browser/actions/omnibox_action.h"  // IWYU pragma: export
#undef NewIncognitoWindow
#undef OpenSharingHub

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_ACTIONS_OMNIBOX_ACTION_H_
