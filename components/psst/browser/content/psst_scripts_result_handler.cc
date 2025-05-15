/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/content/psst_scripts_result_handler.h"

#include <memory>
#include <utility>

#include "content/public/browser/web_contents.h"

namespace psst {

PsstScriptsHandler::PsstScriptsHandler() = default;
PsstScriptsHandler::~PsstScriptsHandler() = default;

PsstScriptsHandlerImpl::PsstScriptsHandlerImpl(
    std::unique_ptr<PsstDialogDelegate> delegate,
    PrefService* prefs,
    content::WebContents* web_contents,
    const content::RenderFrameHost* render_frame_host,
    const int32_t world_id)
    : delegate_(std::move(delegate)),
      prefs_(prefs),
      render_frame_host_id_(render_frame_host->GetGlobalId()),
      web_contents_(web_contents),
      world_id_(world_id) {
  DCHECK(world_id_ > content::ISOLATED_WORLD_ID_CONTENT_END);
  DCHECK(render_frame_host);
  DCHECK(web_contents_);
  DCHECK(prefs_);
}

PsstScriptsHandlerImpl::~PsstScriptsHandlerImpl() = default;

void PsstScriptsHandlerImpl::Start() {
  // Here must be implemented the logic of starting the script handler
  // and showing the dialog.
}

PsstDialogDelegate* PsstScriptsHandlerImpl::GetPsstDialogDelegate() {
  return delegate_.get();
}

}  // namespace psst
