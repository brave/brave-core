// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/commander/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/components/commander/browser/commander_frontend_delegate.h"
#endif  // BUILDFLAG(ENABLE_COMMANDER)

#include <components/omnibox/browser/actions/omnibox_action_client_delegator.cc>

// Brave adds these methods to OmniboxAction::Client; forward them to the
// delegate like every other Client method. Declared via the plaster for
// omnibox_action_client_delegator.h.
#if BUILDFLAG(ENABLE_COMMANDER)
commander::CommanderFrontendDelegate*
OmniboxActionClientDelegator::GetCommanderDelegate() {
  return delegate_->GetCommanderDelegate();
}
#endif  // BUILDFLAG(ENABLE_COMMANDER)

#if BUILDFLAG(ENABLE_AI_CHAT)
void OmniboxActionClientDelegator::OpenLeo(const std::u16string& query) {
  delegate_->OpenLeo(query);
}

bool OmniboxActionClientDelegator::IsLeoProviderEnabled() {
  return delegate_->IsLeoProviderEnabled();
}
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
