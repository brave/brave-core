/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/tracking_protection_helper.h"

#include "base/task/post_task.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_switches.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"

using content::BrowserThread;
using content::NavigationHandle;
using content::RenderFrameHost;
using content::WebContents;

namespace {

void SetStartingSiteForRenderFrame(GURL starting_site,
                                   int render_process_id,
                                   int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  g_brave_browser_process->tracking_protection_service()
      ->SetStartingSiteForRenderFrame(starting_site, render_process_id,
                                      render_frame_id);
}

void DeleteRenderFrameKey(int render_process_id, int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  g_brave_browser_process->tracking_protection_service()->DeleteRenderFrameKey(
      render_process_id, render_frame_id);
}

void ModifyRenderFrameKey(int old_render_process_id,
                          int old_render_frame_id,
                          int new_render_process_id,
                          int new_render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  g_brave_browser_process->tracking_protection_service()->ModifyRenderFrameKey(
      old_render_process_id, old_render_frame_id, new_render_process_id,
      new_render_frame_id);
}

}  // namespace

namespace brave_shields {

bool TrackingProtectionHelper::IsSmartTrackingProtectionEnabled() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  return command_line.HasSwitch(switches::kEnableSmartTrackingProtection);
}

TrackingProtectionHelper::TrackingProtectionHelper(WebContents* web_contents)
    : WebContentsObserver(web_contents) {}

TrackingProtectionHelper::~TrackingProtectionHelper() {}

void TrackingProtectionHelper::ReadyToCommitNavigation(
    NavigationHandle* handle) {
  if (handle->IsInMainFrame() &&
      !ui::PageTransitionIsRedirect(handle->GetPageTransition())) {
    RenderFrameHost* rfh = web_contents()->GetMainFrame();

    // Here we post to the IO thread to avoid locks in
    // tracking_protection_service to avoid race conditions when accessing the
    // starting_site map during navigations and accessing storage access apis
    base::PostTaskWithTraits(
        FROM_HERE, {BrowserThread::IO},
        base::BindOnce(&SetStartingSiteForRenderFrame, handle->GetURL(),
                       rfh->GetProcess()->GetID(), rfh->GetRoutingID()));
  }
}

void TrackingProtectionHelper::RenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&DeleteRenderFrameKey,
                     render_frame_host->GetProcess()->GetID(),
                     render_frame_host->GetRoutingID()));
}

void TrackingProtectionHelper::RenderFrameHostChanged(
    RenderFrameHost* old_host,
    RenderFrameHost* new_host) {
  if (!old_host || old_host->GetParent() || new_host->GetParent()) {
    return;
  }
  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&ModifyRenderFrameKey, old_host->GetProcess()->GetID(),
                     old_host->GetRoutingID(), new_host->GetProcess()->GetID(),
                     new_host->GetRoutingID()));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(TrackingProtectionHelper)

}  // namespace brave_shields
