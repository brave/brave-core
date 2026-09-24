// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_ACTIONS_OMNIBOX_ACTION_CLIENT_DELEGATOR_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_ACTIONS_OMNIBOX_ACTION_CLIENT_DELEGATOR_H_

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/commander/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/components/commander/browser/commander_frontend_delegate.h"
#endif  // BUILDFLAG(ENABLE_COMMANDER)

#include <components/omnibox/browser/actions/omnibox_action_client_delegator.h>  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_ACTIONS_OMNIBOX_ACTION_CLIENT_DELEGATOR_H_
