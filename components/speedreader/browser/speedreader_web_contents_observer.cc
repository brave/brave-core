/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/browser/speedreader_web_contents_observer.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/debug/stack_trace.h"
#include "url/gurl.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/brave_isolated_worlds.h"
#include "brave/common/pref_names.h"
#include "brave/common/render_messages.h"
#include "brave/components/speedreader/common/speedreader_constants.h"
#include "brave/components/speedreader/resources/grit/speedreader_resources.h"
#include "components/grit/brave_components_resources.h"
#include "brave/content/common/frame_messages.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/renderer_configuration.mojom.h"
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
#include "extensions/buildflags/buildflags.h"
#include "ipc/ipc_message_macros.h"
#include "ui/base/resource/resource_bundle.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/common/extensions/api/brave_shields.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_api_frame_id_map.h"

using extensions::Event;
using extensions::EventRouter;
#endif

using content::Referrer;
using content::RenderFrameHost;
using content::WebContents;

namespace {

// Content Settings are only sent to the main frame currently.
// Chrome may fix this at some point, but for now we do this as a work-around.
// You can verify if this is fixed by running the following test:
// npm run test -- brave_browser_tests --filter=BraveContentSettingsObserverBrowserTest.*  // NOLINT
// Chrome seems to also have a bug with RenderFrameHostChanged not updating
// the content settings so this is fixed here too. That case is covered in
// tests by:
// npm run test -- brave_browser_tests --filter=BraveContentSettingsObserverBrowserTest.*  // NOLINT
void UpdateContentSettingsToRendererFrames(content::WebContents* web_contents) {
  for (content::RenderFrameHost* frame : web_contents->GetAllFrames()) {
    Profile* profile =
        Profile::FromBrowserContext(web_contents->GetBrowserContext());
    const HostContentSettingsMap* map =
        HostContentSettingsMapFactory::GetForProfile(profile);
    RendererContentSettingRules rules;
    GetRendererContentSettingRules(map, &rules);

    // Add `speedreader` resource identifier rules to the map here, avoiding to `GetRendererContentSettingRules`
    map->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_PLUGINS,
      "speedreader",
      &(rules.speedreader_rules));

    IPC::ChannelProxy* channel =
        frame->GetProcess()->GetChannel();
    // channel might be NULL in tests.
    if (channel) {
      chrome::mojom::RendererConfigurationAssociatedPtr rc_interface;
      channel->GetRemoteAssociatedInterface(&rc_interface);
      rc_interface->SetContentSettingRules(rules);
    }
  }
}

}  // namespace

namespace speedreader {

SpeedreaderWebContentsObserver::~SpeedreaderWebContentsObserver() {
}

SpeedreaderWebContentsObserver::SpeedreaderWebContentsObserver(
    WebContents* web_contents)
    : WebContentsObserver(web_contents) {
}

void SpeedreaderWebContentsObserver::RenderFrameCreated(
    RenderFrameHost* rfh) {
  if (rfh && disabled_speedreader_origins_.size()) {
    rfh->Send(new BraveFrameMsg_DisableSpeedreaderOnce(
          rfh->GetRoutingID(), disabled_speedreader_origins_));
  }

  WebContents* web_contents = WebContents::FromRenderFrameHost(rfh);
  if (web_contents) {
    UpdateContentSettingsToRendererFrames(web_contents);
  }
}

bool SpeedreaderWebContentsObserver::OnMessageReceived(
    const IPC::Message& message, RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(SpeedreaderWebContentsObserver,
        message, render_frame_host)
    IPC_MESSAGE_HANDLER(BraveViewHostMsg_SpeedreaderTransformed,
        OnSpeedreaderTransformed)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void SpeedreaderWebContentsObserver::OnSpeedreaderTransformed(
    RenderFrameHost* render_frame_host) {
  WebContents* web_contents =
      WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents) {
    return;
  }
  {
    std::string data = ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
        IDR_SPEEDREADER_JS_STYLESHEET_INJECT).as_string();
    render_frame_host->ExecuteJavaScriptInIsolatedWorld(
          base::UTF8ToUTF16(data), base::DoNothing(),
          ISOLATED_WORLD_ID_SPEEDREADER);
  }
  {
    std::string data = ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
        IDR_SPEEDREADER_STYLE_DESKTOP).as_string();
    render_frame_host->ExecuteJavaScriptInIsolatedWorld(
          base::UTF8ToUTF16("var style = `" + data + "`; addStyleString(style);"), base::DoNothing(),
          ISOLATED_WORLD_ID_SPEEDREADER);
  }
}

void SpeedreaderWebContentsObserver::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  // when the main frame navigate away
  if (navigation_handle->IsInMainFrame() &&
      !navigation_handle->IsSameDocument() &&
      navigation_handle->GetReloadType() == content::ReloadType::NONE) {
    disabled_speedreader_origins_.clear();
  }

  navigation_handle->GetWebContents()->SendToAllFrames(
      new BraveFrameMsg_DisableSpeedreaderOnce(
        MSG_ROUTING_NONE, disabled_speedreader_origins_));
}

void SpeedreaderWebContentsObserver::DisableSpeedreaderOnce(
    const std::vector<std::string>& origins, WebContents* contents) {
  disabled_speedreader_origins_ = std::move(origins);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SpeedreaderWebContentsObserver)

}  // namespace speedreader
