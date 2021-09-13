/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/renderer/speedreader_js_handler.h"

#include "base/bind.h"
#include "base/logging.h"
#include "brave/components/speedreader/common/speedreader_ui_prefs.mojom-shared.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace {

// TODO(karenliu): OK these values for accessibility
// constexpr float kMinFontScale = 0.5f;
// constexpr float kMaxFontScale = 2.5f;

const char kResizeFontScript[] =
    R"((function() {
          document.documentElement.style.fontSize = '30px';
        })();)";

}  // anonymous namespace

namespace speedreader {

SpeedreaderJsHandler::SpeedreaderJsHandler(content::RenderFrame* render_frame,
                                           const int32_t isolated_world_id)
    : render_frame_(render_frame), isolated_world_id_(isolated_world_id) {
  EnsureConnected();
}

SpeedreaderJsHandler::~SpeedreaderJsHandler() = default;

void SpeedreaderJsHandler::EnsureConnected() {
  if (!receiver_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        remote_.BindNewPipeAndPassReceiver());
    remote_->AddObserver(receiver_.BindNewPipeAndPassRemote());
    DCHECK(receiver_.is_bound());
  }
}

void SpeedreaderJsHandler::OnFontScaleChanged(float scale) {
  EnsureConnected();
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  if (!web_frame->IsProvisional()) {
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_, blink::WebString::FromUTF8(kResizeFontScript),
        blink::BackForwardCacheAware::kAllow);
  }
}

}  // namespace speedreader
