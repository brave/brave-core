/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_tab_helpers.h"

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/unguessable_token.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/content_settings/page_specific_content_settings_delegate.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "extensions/buildflags/buildflags.h"
#include "net/base/features.h"
#include "printing/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ai_chat/ai_chat_utils.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/content/browser/playlist_tab_helper.h"
#include "brave/components/playlist/core/browser/utils.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "brave/browser/screenshot/print_preview_extractor_factory.h"
#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/print_preview_extraction_delegate_impl.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/onion_location_tab_helper.h"
#include "brave/components/tor/tor_tab_helper.h"
#endif

namespace brave {

void AttachTabHelpers(content::WebContents* web_contents) {
  // If your TabHelper is related to privacy consider whether it belongs in this
  // method, so it can also be attached to background WebContents.
  AttachPrivacySensitiveTabHelpers(web_contents);

#if BUILDFLAG(ENABLE_AI_CHAT)
  content::BrowserContext* context = web_contents->GetBrowserContext();
  if (ai_chat::IsAllowedForContext(context)) {
    ai_chat::AIChatTabHelper::CreateForWebContents(
        web_contents,
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
        std::make_unique<ai_chat::PrintPreviewExtractionDelegateImpl>(
            web_contents,
            screenshot::CreatePrintPreviewExtractor(base::BindRepeating(
                []() -> base::flat_map<base::UnguessableToken, int>& {
                  return printing::PrintPreviewUI::
                      GetPrintPreviewUIRequestIdMap();
                })))
#else
        nullptr
#endif
    );
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  BraveWaybackMachineTabHelper::CreateIfNeeded(web_contents);
#endif

  brave_perf_predictor::PerfPredictorTabHelper::CreateForWebContents(
      web_contents);

#if BUILDFLAG(ENABLE_TOR)
  tor::TorTabHelper::MaybeCreateForWebContents(web_contents);
  tor::OnionLocationTabHelper::CreateForWebContents(web_contents);
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
  if (playlist::IsPlaylistAllowed(
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext()))) {
    if (auto* playlist_service =
            playlist::PlaylistServiceFactory::GetForBrowserContext(
                web_contents->GetBrowserContext())) {
      playlist::PlaylistTabHelper::CreateForWebContents(web_contents,
                                                        playlist_service);
    }
  }
#endif  // BUILDFLAG(ENABLE_PLAYLIST)
}

void AttachPrivacySensitiveTabHelpers(content::WebContents* web_contents) {
  content_settings::PageSpecificContentSettings::CreateForWebContents(
      web_contents,
      std::make_unique<PageSpecificContentSettingsDelegate>(web_contents));
  brave_shields::BraveShieldsWebContentsObserver::CreateForWebContents(
      web_contents);
  if (auto* ephemeral_storage_service =
          EphemeralStorageServiceFactory::GetForContext(
              web_contents->GetBrowserContext())) {
    ephemeral_storage::EphemeralStorageTabHelper::CreateForWebContents(
        web_contents, ephemeral_storage_service);
  }
}

}  // namespace brave
