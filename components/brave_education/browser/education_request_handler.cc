/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_education/browser/education_request_handler.h"

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace brave_education {

namespace {

content::WebContents* WebContentsFromHostID(
    content::GlobalRenderFrameHostId frame_id) {
  auto* host = content::RenderFrameHost::FromID(frame_id);
  if (!host) {
    return nullptr;
  }

  return content::WebContents::FromRenderFrameHost(host);
}

}  // namespace

EducationRequestHandler::EducationRequestHandler(
    content::GlobalRenderFrameHostId frame_id)
    : frame_id_(frame_id) {}

EducationRequestHandler::~EducationRequestHandler() = default;

void EducationRequestHandler::ShowSettingsPage(
    const std::string& relative_url,
    mojom::SettingsPageTarget target) {
  GURL settings_url = GURL("brave://settings").Resolve(relative_url);

  // Resolving `relative_url` must never produce a URL other than a settings
  // page URL.
  if (!settings_url.is_valid() || !settings_url.SchemeIs("brave") ||
      !settings_url.DomainIs("settings")) {
    return;
  }

  auto* web_contents = WebContentsFromHostID(frame_id_);
  if (!web_contents) {
    return;
  }

  web_contents->OpenURL({settings_url, content::Referrer(),
                         WindowOpenDisposition::NEW_FOREGROUND_TAB,
                         ui::PAGE_TRANSITION_LINK, false});
}

}  // namespace brave_education
