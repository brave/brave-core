/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/side_panel/side_panel_registry.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_web_view.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/browser/ui/views/side_panel/wallet/wallet_side_panel_web_view.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#endif

namespace brave {

// Register here for an entry that is used for all tabs and its life time is
// tied with tab. If it has specific life time, use dedicated manager for
// registering it.
void RegisterContextualSidePanel(SidePanelRegistry* registry,
                                 content::WebContents* web_contents) {
#if BUILDFLAG(ENABLE_AI_CHAT) || BUILDFLAG(ENABLE_BRAVE_WALLET)
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
  // Disable tab-scoped panel for content agent profiles.
  // TODO(https://github.com/brave/brave-browser/issues/48526): Remove
  // this when the feature flag is removed.
  if (ai_chat::AIChatServiceFactory::GetForBrowserContext(profile) &&
      !ai_chat::ShouldSidePanelBeGlobal(profile)) {
    // If |registry| already has it, it's no-op.
    registry->Register(std::make_unique<SidePanelEntry>(
        SidePanelEntry::Key(SidePanelEntry::Id::kChatUI),
        base::BindRepeating(&AIChatSidePanelWebView::CreateView, profile,
                            /*is_tab_associated=*/true),
        /*default_content_width_callback=*/base::NullCallback()));
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  if (brave_wallet::IsAllowed(profile->GetPrefs()) &&
      base::FeatureList::IsEnabled(
          brave_wallet::features::kBraveWalletSidePanel)) {
    registry->Register(std::make_unique<SidePanelEntry>(
        SidePanelEntry::Key(SidePanelEntry::Id::kWallet),
        base::BindRepeating(&WalletSidePanelWebView::CreateView, profile),
        /*default_content_width_callback=*/base::NullCallback()));
  }
#endif
}

}  // namespace brave
