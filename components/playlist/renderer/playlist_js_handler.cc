/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_js_handler.h"

#include "base/functional/callback.h"
#include "base/logging.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"

namespace playlist {

PlaylistJSHandler::PlaylistJSHandler(content::RenderFrame* render_frame)
    : render_frame_(render_frame) {
  EnsureConnectedToMediaHandler();
}

PlaylistJSHandler::~PlaylistJSHandler() {}

bool PlaylistJSHandler::EnsureConnectedToMediaHandler() {
  if (!media_handler_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        media_handler_.BindNewPipeAndPassReceiver());
    media_handler_.set_disconnect_handler(
        base::BindOnce(&PlaylistJSHandler::OnMediaHandlerDisconnect,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  return media_handler_.is_bound();
}

void PlaylistJSHandler::OnMediaHandlerDisconnect() {
  media_handler_.reset();
  EnsureConnectedToMediaHandler();
}

void PlaylistJSHandler::OnMediaUpdated(const std::string& page_url) {
  if (!GURL(page_url).SchemeIsHTTPOrHTTPS()) {
    return;
  }

  DVLOG(2) << __FUNCTION__ << " " << page_url;

  if (!EnsureConnectedToMediaHandler()) {
    return;
  }

  media_handler_->OnMediaUpdatedFromRenderFrame();
}

}  // namespace playlist
