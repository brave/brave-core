// Copyright (c) 2019 The Brave Authors. All rights reserved.

#include "brave/browser/renderer_host/brave_render_message_filter.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/browser/buildflags/buildflags.h"  // For STP
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "brave/components/content_settings/core/browser/brave_cookie_settings.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/render_messages.h"

using base::string16;
using brave_shields::BraveShieldsWebContentsObserver;
using content_settings::CookieSettings;

BraveRenderMessageFilter::BraveRenderMessageFilter(int render_process_id,
  Profile* profile)
  : ChromeRenderMessageFilter(render_process_id, profile),
    host_content_settings_map_(HostContentSettingsMapFactory::GetForProfile(
      profile)) {
}

BraveRenderMessageFilter::~BraveRenderMessageFilter() {}

bool BraveRenderMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(BraveRenderMessageFilter, message)
    IPC_MESSAGE_HANDLER(ChromeViewHostMsg_AllowDatabase, OnAllowDatabase);
    IPC_MESSAGE_HANDLER(ChromeViewHostMsg_AllowDOMStorage, OnAllowDOMStorage);
    IPC_MESSAGE_HANDLER(ChromeViewHostMsg_AllowIndexedDB, OnAllowIndexedDB);
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return ChromeRenderMessageFilter::OnMessageReceived(message);
}

void BraveRenderMessageFilter::ShouldStoreState(int render_frame_id,
                                                const GURL& origin_url,
                                                const GURL& top_origin_url,
                                                bool* allowed) {
  GURL tab_origin =
      BraveShieldsWebContentsObserver::GetTabURLFromRenderFrameInfo(
          render_process_id_, render_frame_id, -1)
          .GetOrigin();
  *allowed = g_brave_browser_process->tracking_protection_service()->ShouldStoreState(
      cookie_settings_.get(), host_content_settings_map_, render_process_id_, render_frame_id,
      origin_url, top_origin_url, tab_origin);
}

void BraveRenderMessageFilter::OnAllowDatabase(int render_frame_id,
                                               const GURL& origin_url,
                                               const GURL& top_origin_url,
                                               bool* allowed) {
  ShouldStoreState(render_frame_id, origin_url, top_origin_url, allowed);
  if (*allowed) {
    ChromeRenderMessageFilter::OnAllowDatabase(render_frame_id,
                                               origin_url,
                                               top_origin_url,
                                               allowed);
  }
}

void BraveRenderMessageFilter::OnAllowDOMStorage(int render_frame_id,
                                                 const GURL& origin_url,
                                                 const GURL& top_origin_url,
                                                 bool local,
                                                 bool* allowed) {
  ShouldStoreState(render_frame_id, origin_url, top_origin_url, allowed);
  if (*allowed) {
    ChromeRenderMessageFilter::OnAllowDOMStorage(render_frame_id,
                                                 origin_url,
                                                 top_origin_url,
                                                 local,
                                                 allowed);
  }
}

void BraveRenderMessageFilter::OnAllowIndexedDB(int render_frame_id,
                                                const GURL& origin_url,
                                                const GURL& top_origin_url,
                                                bool* allowed) {
  ShouldStoreState(render_frame_id, origin_url, top_origin_url, allowed);
  if (*allowed) {
    ChromeRenderMessageFilter::OnAllowIndexedDB(render_frame_id,
                                                origin_url,
                                                top_origin_url,
                                                allowed);
  }
}
