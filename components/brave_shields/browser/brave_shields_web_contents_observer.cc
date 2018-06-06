/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/common/extensions/api/brave_shields.h"
#include "brave/common/pref_names.h"
#include "brave/common/render_messages.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/content/common/frame_messages.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/navigator.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "ipc/ipc_message_macros.h"

using extensions::Event;
using extensions::EventRouter;
using content::Referrer;
using content::RenderFrameHost;
using content::WebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    brave_shields::BraveShieldsWebContentsObserver);

namespace {

WebContents* GetWebContents(
    int render_process_id,
    int render_frame_id,
    int frame_tree_node_id) {
  WebContents* web_contents =
      WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  if (!web_contents) {
    RenderFrameHost* rfh =
        RenderFrameHost::FromID(render_process_id, render_frame_id);
    if (!rfh) {
      return nullptr;
    }
    web_contents =
        WebContents::FromRenderFrameHost(rfh);
  }
  return web_contents;
}

}  // namespace

namespace brave_shields {

base::Lock BraveShieldsWebContentsObserver::frame_data_map_lock_;
std::map<BraveShieldsWebContentsObserver::RenderFrameIdKey, GURL>
    BraveShieldsWebContentsObserver::render_frame_key_to_tab_url;

BraveShieldsWebContentsObserver::RenderFrameIdKey::RenderFrameIdKey()
    : render_process_id(content::ChildProcessHost::kInvalidUniqueID),
      frame_routing_id(MSG_ROUTING_NONE) {}

BraveShieldsWebContentsObserver::RenderFrameIdKey::RenderFrameIdKey(
    int render_process_id,
    int frame_routing_id)
    : render_process_id(render_process_id),
      frame_routing_id(frame_routing_id) {}

bool BraveShieldsWebContentsObserver::RenderFrameIdKey::operator<(
    const RenderFrameIdKey& other) const {
  return std::tie(render_process_id, frame_routing_id) <
         std::tie(other.render_process_id, other.frame_routing_id);
}

bool BraveShieldsWebContentsObserver::RenderFrameIdKey::operator==(
    const RenderFrameIdKey& other) const {
  return render_process_id == other.render_process_id &&
         frame_routing_id == other.frame_routing_id;
}

BraveShieldsWebContentsObserver::~BraveShieldsWebContentsObserver() {
}

BraveShieldsWebContentsObserver::BraveShieldsWebContentsObserver(
    WebContents* web_contents)
    : WebContentsObserver(web_contents) {
}

void BraveShieldsWebContentsObserver::RenderFrameCreated(
    RenderFrameHost* rfh) {
  if (!rfh->IsRenderFrameLive())
    return;

  // Look up the extension API frame ID to force the mapping to be cached.
  // This is needed so that cached information is available for tabId in the
  // filtering callbacks.
  extensions::ExtensionApiFrameIdMap::Get()->InitializeRenderFrameData(rfh);

  WebContents* web_contents = WebContents::FromRenderFrameHost(rfh);
  if (web_contents) {
    base::AutoLock lock(frame_data_map_lock_);
    const RenderFrameIdKey key(rfh->GetProcess()->GetID(), rfh->GetRoutingID());
    std::map<RenderFrameIdKey, GURL>::iterator iter =
        render_frame_key_to_tab_url.find(key);
    if (iter != render_frame_key_to_tab_url.end()) {
      render_frame_key_to_tab_url.erase(key);
    }
    render_frame_key_to_tab_url.insert(
        std::pair<RenderFrameIdKey, GURL>(key, web_contents->GetURL()));
  }
}

void BraveShieldsWebContentsObserver::RenderFrameDeleted(
    RenderFrameHost* rfh) {
  base::AutoLock lock(frame_data_map_lock_);
  const RenderFrameIdKey key(rfh->GetProcess()->GetID(), rfh->GetRoutingID());
  std::map<RenderFrameIdKey, GURL>::iterator iter =
      render_frame_key_to_tab_url.find(key);
  if (iter != render_frame_key_to_tab_url.end()) {
    render_frame_key_to_tab_url.erase(key);
  }
}

void BraveShieldsWebContentsObserver::RenderFrameHostChanged(
    RenderFrameHost* old_host, RenderFrameHost* new_host) {
  if (old_host) {
    RenderFrameDeleted(old_host);
  }
  if (new_host) {
    RenderFrameCreated(new_host);
  }
}

// static
GURL BraveShieldsWebContentsObserver::GetTabURLFromRenderFrameInfo(
    int render_process_id, int render_frame_id) {
  base::AutoLock lock(frame_data_map_lock_);
  const RenderFrameIdKey key(render_process_id, render_frame_id);
  std::map<RenderFrameIdKey, GURL>::iterator iter =
      render_frame_key_to_tab_url.find(key);
  if (iter != render_frame_key_to_tab_url.end()) {
    return iter->second;
  }
  return GURL();
}

void BraveShieldsWebContentsObserver::DispatchBlockedEvent(
    const std::string& block_type,
    const std::string& subresource,
    int render_process_id,
    int render_frame_id,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  WebContents* web_contents = GetWebContents(render_process_id,
    render_frame_id, frame_tree_node_id);
  DispatchBlockedEventForWebContents(block_type, subresource, web_contents);

  if (web_contents) {
    PrefService* prefs = Profile::FromBrowserContext(
        web_contents->GetBrowserContext())->
        GetOriginalProfile()->
        GetPrefs();

    if (block_type == kAds) {
      prefs->SetUint64(kAdsBlocked, prefs->GetUint64(kAdsBlocked) + 1);
    } else if (block_type == kTrackers) {
      prefs->SetUint64(kTrackersBlocked,
          prefs->GetUint64(kTrackersBlocked) + 1);
    } else if (block_type == kHTTPUpgradableResources) {
      prefs->SetUint64(kHttpsUpgrades, prefs->GetUint64(kHttpsUpgrades) + 1);
    } else if (block_type == kJavaScript) {
      prefs->SetUint64(kJavascriptBlocked,
          prefs->GetUint64(kJavascriptBlocked) + 1);
    } else if (block_type == kFingerprinting) {
      prefs->SetUint64(kFingerprintingBlocked,
          prefs->GetUint64(kFingerprintingBlocked) + 1);
    }
  }
}

void BraveShieldsWebContentsObserver::DispatchBlockedEventForWebContents(
    const std::string& block_type, const std::string& subresource,
    WebContents* web_contents) {
  if (!web_contents) {
    return;
  }
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  EventRouter* event_router = EventRouter::Get(profile);
  if (profile && event_router) {
    extensions::api::brave_shields::OnBlocked::Details details;
    details.tab_id = extensions::ExtensionTabUtil::GetTabId(web_contents);
    details.block_type = block_type;
    details.subresource = subresource;
    std::unique_ptr<base::ListValue> args(
        extensions::api::brave_shields::OnBlocked::Create(details)
          .release());
    std::unique_ptr<Event> event(
        new Event(extensions::events::BRAVE_AD_BLOCKED,
          extensions::api::brave_shields::OnBlocked::kEventName,
          std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

bool BraveShieldsWebContentsObserver::OnMessageReceived(
    const IPC::Message& message, RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(BraveShieldsWebContentsObserver,
        message, render_frame_host)
    IPC_MESSAGE_HANDLER(BraveViewHostMsg_JavaScriptBlocked,
        OnJavaScriptBlockedWithDetail)
    IPC_MESSAGE_HANDLER(BraveViewHostMsg_FingerprintingBlocked,
        OnFingerprintingBlockedWithDetail)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void BraveShieldsWebContentsObserver::OnJavaScriptBlockedWithDetail(
    RenderFrameHost* render_frame_host,
    const base::string16& details) {
  WebContents* web_contents =
      WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents) {
    return;
  }
  DispatchBlockedEventForWebContents(brave_shields::kJavaScript,
      base::UTF16ToUTF8(details), web_contents);
}

void BraveShieldsWebContentsObserver::OnFingerprintingBlockedWithDetail(
    RenderFrameHost* render_frame_host,
    const base::string16& details) {
  WebContents* web_contents =
      WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents) {
    return;
  }
  DispatchBlockedEventForWebContents(brave_shields::kFingerprinting,
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
  registry->RegisterStringPref(kAdBlockCurrentRegion, "");
}

void BraveShieldsWebContentsObserver::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  auto frame_tree_node_id = navigation_handle->GetFrameTreeNodeId();
  auto *frame_tree_node = content::FrameTreeNode::GloballyFindByID(
      frame_tree_node_id);
  auto* navigation_entry =
    frame_tree_node->navigator()->GetController()->GetPendingEntry();

  if (!navigation_entry) {
    navigation_entry =
      frame_tree_node->navigator()->GetController()->GetLastCommittedEntry();
  }
  GURL target_origin = navigation_handle->GetURL().GetOrigin();
  if (!navigation_entry) {
    return;
  }
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  GURL tab_origin(navigation_entry->GetURL().GetOrigin());

  std::unique_ptr<base::Value> referrer_value =
      HostContentSettingsMapFactory::GetForProfile(profile)
      ->GetWebsiteSetting(
          tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kReferrers, NULL);
  ContentSetting referrer_setting =
      content_settings::ValueToContentSetting(referrer_value.get());

  std::unique_ptr<base::Value> shields_value =
      HostContentSettingsMapFactory::GetForProfile(profile)
      ->GetWebsiteSetting(
          tab_origin, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kBraveShields, NULL);
  ContentSetting shields_setting =
      content_settings::ValueToContentSetting(shields_value.get());

  content::Referrer original_referrer = navigation_handle->GetReferrer();
  content::Referrer new_referrer;
  if (ShouldSetReferrer(referrer_setting == CONTENT_SETTING_ALLOW,
          shields_setting != CONTENT_SETTING_BLOCK,
          original_referrer.url,
          tab_origin,
          navigation_handle->GetURL(),
          navigation_handle->GetURL().GetOrigin(),
          original_referrer.policy, &new_referrer)) {
    navigation_entry->SetReferrer(new_referrer);
  }

  // when the main frame navigate away
  if (navigation_handle->IsInMainFrame() &&
      !navigation_handle->IsSameDocument() &&
      navigation_handle->GetReloadType() == content::ReloadType::NONE) {
    allowed_script_origins_.clear();
  }

  navigation_handle->GetWebContents()->SendToAllFrames(
      new BraveFrameMsg_AllowScriptsOnce(
        MSG_ROUTING_NONE, allowed_script_origins_));
}

void BraveShieldsWebContentsObserver::AllowScriptsOnce(
    const std::vector<std::string>& origins, WebContents* contents) {
  allowed_script_origins_ = std::move(origins);
}

}  // namespace brave_shields
