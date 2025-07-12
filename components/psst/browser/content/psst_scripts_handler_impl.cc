// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_scripts_handler_impl.h"

#include <string>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace psst {

PsstScriptsHandlerImpl::PsstScriptsHandlerImpl(
    content::WebContents* web_contents,
    const int32_t world_id)
    : web_contents_(web_contents), world_id_(world_id) {
  CHECK(world_id_ > content::ISOLATED_WORLD_ID_CONTENT_END);
  CHECK(web_contents_);
}

PsstScriptsHandlerImpl::~PsstScriptsHandlerImpl() = default;

void PsstScriptsHandlerImpl::InsertScriptInPage(
    const std::string& script,
    PsstTabWebContentsObserver::InsertScriptInPageCallback cb) {
  web_contents_->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      base::UTF8ToUTF16(script), std::move(cb), world_id_);
}

}  // namespace psst
