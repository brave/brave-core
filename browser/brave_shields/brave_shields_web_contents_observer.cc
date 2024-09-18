/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/renderer_configuration.mojom.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#endif

using content::NavigationHandle;
using content::RenderFrameHost;
using content::WebContents;

namespace brave_shields {

namespace {

BraveShieldsWebContentsObserver* g_receiver_impl_for_testing = nullptr;

}  // namespace

BraveShieldsWebContentsObserver::~BraveShieldsWebContentsObserver() = default;

BraveShieldsWebContentsObserver::BraveShieldsWebContentsObserver(
    WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<BraveShieldsWebContentsObserver>(
          *web_contents),
      receivers_(web_contents, this) {}

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
  if (!web_contents) {
    return;
  }

  auto* shields_host =
      BraveShieldsWebContentsObserver::FromWebContents(web_contents);
  if (!shields_host) {
    return;
  }
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

#if !BUILDFLAG(IS_ANDROID)
// static
void BraveShieldsWebContentsObserver::DispatchBlockedEventForWebContents(
    const std::string& block_type,
    const std::string& subresource,
    WebContents* web_contents) {
  if (!web_contents) {
    return;
  }
  auto* shields_data_ctrlr =
      brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents);
  // |shields_data_ctrlr| can be null if the |web_contents| is generated in
  // component layer - We don't attach any tab helpers in this case.
  if (!shields_data_ctrlr) {
    return;
  }
  shields_data_ctrlr->HandleItemBlocked(block_type, subresource);
}
// static
void BraveShieldsWebContentsObserver::DispatchAllowedOnceEventForWebContents(
    const std::string& block_type,
    const std::string& subresource,
    WebContents* web_contents) {
  if (!web_contents) {
    return;
  }
  auto* shields_data_ctrlr =
      brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents);
  // |shields_data_ctrlr| can be null if the |web_contents| is generated in
  // component layer - We don't attach any tab helpers in this case.
  if (!shields_data_ctrlr) {
    return;
  }
  shields_data_ctrlr->HandleItemAllowedOnce(block_type, subresource);
}
// static
void BraveShieldsWebContentsObserver::
    DispatchWebcompatFeatureInvokedForWebContents(
        ContentSettingsType webcompat_content_settings,
        WebContents* web_contents) {
  if (!web_contents) {
    return;
  }
  auto* shields_data_ctrlr =
      brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents);
  // |shields_data_ctrlr| can be null if the |web_contents| is generated in
  // component layer - We don't attach any tab helpers in this case.
  if (!shields_data_ctrlr) {
    return;
  }
  shields_data_ctrlr->HandleWebcompatFeatureInvoked(webcompat_content_settings);
}
#endif

void BraveShieldsWebContentsObserver::OnJavaScriptAllowedOnce(
    const std::u16string& details) {
#if !BUILDFLAG(IS_ANDROID)
  WebContents* web_contents =
      WebContents::FromRenderFrameHost(receivers_.GetCurrentTargetFrame());
  if (!web_contents) {
    return;
  }
  DispatchAllowedOnceEventForWebContents(
      brave_shields::kJavaScript, base::UTF16ToUTF8(details), web_contents);
#endif
}

void BraveShieldsWebContentsObserver::OnWebcompatFeatureInvoked(
    ContentSettingsType webcompat_settings_type) {
#if !BUILDFLAG(IS_ANDROID)
  WebContents* web_contents =
      WebContents::FromRenderFrameHost(receivers_.GetCurrentTargetFrame());
  if (!web_contents) {
    return;
  }
  DispatchWebcompatFeatureInvokedForWebContents(webcompat_settings_type,
                                                web_contents);
#endif
}

void BraveShieldsWebContentsObserver::OnJavaScriptBlocked(
    const std::u16string& details) {
  WebContents* web_contents =
      WebContents::FromRenderFrameHost(receivers_.GetCurrentTargetFrame());
  if (!web_contents) {
    return;
  }
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
    NavigationHandle* navigation_handle) {
  // Ignore same document navigations.
  if (navigation_handle->IsSameDocument()) {
    return;
  }

  // When the main frame navigates away.
  content::ReloadType reload_type = navigation_handle->GetReloadType();
  if (navigation_handle->IsInMainFrame()) {
    if (reload_type == content::ReloadType::NONE) {
      // For new loads, we reset the counters for both blocked scripts and URLs.
      allowed_scripts_.clear();
      blocked_url_paths_.clear();
    } else if (reload_type == content::ReloadType::NORMAL) {
      // For normal reloads (or loads to the current URL, internally converted
      // into reloads i.e see NavigationControllerImpl::NavigateWithoutEntry),
      // we only reset the counter for blocked URLs, not the one for scripts.
      blocked_url_paths_.clear();
    }
  }

  SendShieldsSettings(navigation_handle);
}

void BraveShieldsWebContentsObserver::BlockAllowedScripts(
    const std::vector<std::string>& scripts) {
  for (const auto& script : scripts) {
    auto origin = url::Origin::Create(GURL(script));
    bool is_origin = origin.Serialize() == script;
    std::erase_if(allowed_scripts_, [is_origin, script,
                                     origin](const std::string& value) {
      // scripts array may have both origins or full scripts paths.
      return is_origin ? url::Origin::Create(GURL(value)) == origin
                       : value == script;
    });
  }
}

void BraveShieldsWebContentsObserver::AllowScriptsOnce(
    const std::vector<std::string>& origins) {
  allowed_scripts_.insert(std::end(allowed_scripts_), std::begin(origins),
                          std::end(origins));
}

// static
void BraveShieldsWebContentsObserver::SetReceiverImplForTesting(
    BraveShieldsWebContentsObserver* impl) {
  g_receiver_impl_for_testing = impl;
}

void BraveShieldsWebContentsObserver::SendShieldsSettings(
    NavigationHandle* navigation_handle) {
  DCHECK(navigation_handle);
  RenderFrameHost* rfh = navigation_handle->GetRenderFrameHost();

  const GURL& primary_url =
      navigation_handle->GetParentFrameOrOuterDocument()
          ? navigation_handle->GetParentFrameOrOuterDocument()
                ->GetOutermostMainFrame()
                ->GetLastCommittedURL()
          : navigation_handle->GetURL();

  const brave_shields::mojom::FarblingLevel farbling_level =
      brave_shields::GetFarblingLevel(
          HostContentSettingsMapFactory::GetForProfile(
              rfh->GetBrowserContext()),
          primary_url);

  PrefService* pref_service =
      user_prefs::UserPrefs::Get(rfh->GetBrowserContext());

  mojo::AssociatedRemote<brave_shields::mojom::BraveShields> agent;
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&agent);
  agent->SetShieldsSettings(brave_shields::mojom::ShieldsSettings::New(
      farbling_level, allowed_scripts_,
      brave_shields::IsReduceLanguageEnabledForProfile(pref_service)));
}

void BraveShieldsWebContentsObserver::BindReceiver(
    mojo::PendingAssociatedReceiver<brave_shields::mojom::BraveShieldsHost>
        receiver,
    content::RenderFrameHost* rfh) {
  receivers_.Bind(rfh, std::move(receiver));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveShieldsWebContentsObserver);

}  // namespace brave_shields
