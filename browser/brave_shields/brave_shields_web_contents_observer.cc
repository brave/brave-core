/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"

#include <memory>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/renderer_configuration.mojom.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/buildflags/buildflags.h"
#include "ipc/ipc_message_macros.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/common/extensions/api/brave_shields.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_api_frame_id_map.h"

using extensions::Event;
using extensions::EventRouter;
#endif

#if !defined(OS_ANDROID)
#include "brave/browser/ui/brave_shields_data_controller.h"
#endif

using content::RenderFrameHost;
using content::WebContents;

namespace brave_shields {

namespace {

BraveShieldsWebContentsObserver* g_receiver_impl_for_testing = nullptr;

// Content Settings are only sent to the main frame currently. Chrome may fix
// this at some point, but for now we do this as a work-around. You can verify
// if this is fixed by running the following test: npm run test --
// brave_browser_tests --filter=BraveContentSettingsAgentImplBrowserTest.*
// Chrome seems to also have a bug with RenderFrameHostChanged not updating the
// content settings so this is fixed here too. That case is covered in tests by:
// npm run test -- brave_browser_tests
// --filter=BraveContentSettingsAgentImplBrowserTest.*
void UpdateContentSettingsToRendererFrames(content::WebContents* web_contents) {
  web_contents->ForEachRenderFrameHost(
      base::BindRepeating([](content::RenderFrameHost* frame) {
        if (!frame->IsRenderFrameLive())
          return;

        IPC::ChannelProxy* channel = frame->GetProcess()->GetChannel();
        // channel might be NULL in tests.
        if (channel) {
          const auto* map = HostContentSettingsMapFactory::GetForProfile(
              frame->GetBrowserContext());

          RendererContentSettingRules rules;
          GetRendererContentSettingRules(map, &rules);

          mojo::AssociatedRemote<chrome::mojom::RendererConfiguration>
              rc_interface;
          channel->GetRemoteAssociatedInterface(&rc_interface);
          rc_interface->SetContentSettingRules(rules);
        }
      }));
}

}  // namespace

BraveShieldsWebContentsObserver::~BraveShieldsWebContentsObserver() {
  brave_shields_remotes_.clear();
}

BraveShieldsWebContentsObserver::BraveShieldsWebContentsObserver(
    WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<BraveShieldsWebContentsObserver>(
          *web_contents),
      receivers_(web_contents, this) {}

void BraveShieldsWebContentsObserver::RenderFrameCreated(RenderFrameHost* rfh) {
  if (rfh && allowed_script_origins_.size()) {
    GetBraveShieldsRemote(rfh)->SetAllowScriptsFromOriginsOnce(
        allowed_script_origins_);
  }

  WebContents* web_contents = WebContents::FromRenderFrameHost(rfh);
  if (web_contents) {
    UpdateContentSettingsToRendererFrames(web_contents);
  }
}

void BraveShieldsWebContentsObserver::RenderFrameDeleted(RenderFrameHost* rfh) {
  brave_shields_remotes_.erase(rfh);
}

void BraveShieldsWebContentsObserver::RenderFrameHostChanged(
    RenderFrameHost* old_host,
    RenderFrameHost* new_host) {
  if (old_host) {
    RenderFrameDeleted(old_host);
  }
  if (new_host) {
    RenderFrameCreated(new_host);
  }
}

bool BraveShieldsWebContentsObserver::IsBlockedSubresource(
    const std::string& subresource) {
  return blocked_url_paths_.find(subresource) != blocked_url_paths_.end();
}

void BraveShieldsWebContentsObserver::AddBlockedSubresource(
    const std::string& subresource) {
  blocked_url_paths_.insert(subresource);
}

// static
void BraveShieldsWebContentsObserver::BindBraveShieldsHost(
    mojo::PendingAssociatedReceiver<brave_shields::mojom::BraveShieldsHost>
        receiver,
    content::RenderFrameHost* rfh) {
  if (g_receiver_impl_for_testing) {
    g_receiver_impl_for_testing->BindReceiver(std::move(receiver), rfh);
    return;
  }

  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents)
    return;

  auto* shields_host =
      BraveShieldsWebContentsObserver::FromWebContents(web_contents);
  if (!shields_host)
    return;
  shields_host->BindReceiver(std::move(receiver), rfh);
}

// static
void BraveShieldsWebContentsObserver::DispatchBlockedEvent(
    const GURL& request_url,
    int frame_tree_node_id,
    const std::string& block_type) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto subresource = request_url.spec();
  WebContents* web_contents =
      WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  DispatchBlockedEventForWebContents(block_type, subresource, web_contents);

  if (web_contents) {
    BraveShieldsWebContentsObserver* observer =
        BraveShieldsWebContentsObserver::FromWebContents(web_contents);
    if (observer && !observer->IsBlockedSubresource(subresource)) {
      observer->AddBlockedSubresource(subresource);
      PrefService* prefs =
          Profile::FromBrowserContext(web_contents->GetBrowserContext())
              ->GetOriginalProfile()
              ->GetPrefs();

      if (block_type == kAds) {
        prefs->SetUint64(kAdsBlocked, prefs->GetUint64(kAdsBlocked) + 1);
      } else if (block_type == kHTTPUpgradableResources) {
        prefs->SetUint64(kHttpsUpgrades, prefs->GetUint64(kHttpsUpgrades) + 1);
      } else if (block_type == kJavaScript) {
        prefs->SetUint64(kJavascriptBlocked,
                         prefs->GetUint64(kJavascriptBlocked) + 1);
      } else if (block_type == kFingerprintingV2) {
        prefs->SetUint64(kFingerprintingBlocked,
                         prefs->GetUint64(kFingerprintingBlocked) + 1);
      }
    }
  }
  brave_perf_predictor::PerfPredictorTabHelper::DispatchBlockedEvent(
      request_url.spec(), frame_tree_node_id);
}

#if !defined(OS_ANDROID)
// static
void BraveShieldsWebContentsObserver::DispatchBlockedEventForWebContents(
    const std::string& block_type,
    const std::string& subresource,
    WebContents* web_contents) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (!web_contents) {
    return;
  }
  EventRouter* event_router =
      EventRouter::Get(web_contents->GetBrowserContext());
  if (event_router) {
    extensions::api::brave_shields::OnBlocked::Details details;
    details.tab_id = extensions::ExtensionTabUtil::GetTabId(web_contents);
    details.block_type = block_type;
    details.subresource = subresource;
    std::unique_ptr<Event> event(
        new Event(extensions::events::BRAVE_AD_BLOCKED,
                  extensions::api::brave_shields::OnBlocked::kEventName,
                  extensions::api::brave_shields::OnBlocked::Create(details)));
    event_router->BroadcastEvent(std::move(event));
  }
#endif
  if (base::FeatureList::IsEnabled(
          brave_shields::features::kBraveShieldsPanelV2)) {
#if !defined(OS_ANDROID)
    if (!web_contents)
      return;
    brave_shields::BraveShieldsDataController::FromWebContents(web_contents)
        ->HandleItemBlocked(block_type, subresource);
#endif
  }
}
#endif

void BraveShieldsWebContentsObserver::OnJavaScriptBlocked(
    const std::u16string& details) {
  WebContents* web_contents =
      WebContents::FromRenderFrameHost(receivers_.GetCurrentTargetFrame());
  if (!web_contents)
    return;

  DispatchBlockedEventForWebContents(brave_shields::kJavaScript,
                                     base::UTF16ToUTF8(details), web_contents);
}

// static
void BraveShieldsWebContentsObserver::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterUint64Pref(kAdsBlocked, 0);
  registry->RegisterUint64Pref(kTrackersBlocked, 0);
  registry->RegisterUint64Pref(kJavascriptBlocked, 0);
  registry->RegisterUint64Pref(kHttpsUpgrades, 0);
  registry->RegisterUint64Pref(kFingerprintingBlocked, 0);
}

void BraveShieldsWebContentsObserver::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  // when the main frame navigate away
  content::ReloadType reload_type = navigation_handle->GetReloadType();
  if (navigation_handle->IsInMainFrame() &&
      !navigation_handle->IsSameDocument()) {
    if (reload_type == content::ReloadType::NONE) {
      // For new loads, we reset the counters for both blocked scripts and URLs.
      allowed_script_origins_.clear();
      blocked_url_paths_.clear();
    } else if (reload_type == content::ReloadType::NORMAL) {
      // For normal reloads (or loads to the current URL, internally converted
      // into reloads i.e see NavigationControllerImpl::NavigateWithoutEntry),
      // we only reset the counter for blocked URLs, not the one for scripts.
      blocked_url_paths_.clear();
    }
  }

  navigation_handle->GetWebContents()->ForEachRenderFrameHost(
      base::BindRepeating(
          [](BraveShieldsWebContentsObserver* observer,
             content::RenderFrameHost* rfh) {
            observer->GetBraveShieldsRemote(rfh)
                ->SetAllowScriptsFromOriginsOnce(
                    observer->allowed_script_origins_);
          },
          base::Unretained(this)));
}

void BraveShieldsWebContentsObserver::AllowScriptsOnce(
    const std::vector<std::string>& origins,
    WebContents* contents) {
  allowed_script_origins_ = std::move(origins);
}

// static
void BraveShieldsWebContentsObserver::SetReceiverImplForTesting(
    BraveShieldsWebContentsObserver* impl) {
  g_receiver_impl_for_testing = impl;
}

void BraveShieldsWebContentsObserver::BindReceiver(
    mojo::PendingAssociatedReceiver<brave_shields::mojom::BraveShieldsHost>
        receiver,
    content::RenderFrameHost* rfh) {
  receivers_.Bind(rfh, std::move(receiver));
}

mojo::AssociatedRemote<brave_shields::mojom::BraveShields>&
BraveShieldsWebContentsObserver::GetBraveShieldsRemote(
    content::RenderFrameHost* rfh) {
  if (!brave_shields_remotes_.contains(rfh)) {
    rfh->GetRemoteAssociatedInterfaces()->GetInterface(
        &brave_shields_remotes_[rfh]);
  }

  DCHECK(brave_shields_remotes_[rfh].is_bound());
  return brave_shields_remotes_[rfh];
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveShieldsWebContentsObserver);

}  // namespace brave_shields
