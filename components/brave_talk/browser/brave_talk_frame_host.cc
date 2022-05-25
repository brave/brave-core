/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_talk/browser/brave_talk_frame_host.h"

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "absl/types/optional.h"
#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/logging.h"
#include "brave/browser/brave_talk/brave_talk_service.h"
#include "brave/components/brave_talk/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/bad_message.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/browser/bad_message.h"
#include "content/browser/renderer_host/frame_tree_node.h"
#include "content/browser/renderer_host/navigation_controller_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/tokens/tokens.h"

namespace brave_talk {
class FrameFinder {
 public:
  FrameFinder(
      const ::blink::FrameToken& token,
      content::WebContents* contents,
      base::OnceCallback<void(const std::string&)> on_received_device_id)
      : token_(token),
        contents_(contents),
        on_received_device_id_(std::move(on_received_device_id)) {}

  ~FrameFinder() {
    if (!on_received_device_id_.is_null())
      std::move(on_received_device_id_).Run("");
  }

  void OnFoundFrame(content::RenderFrameHost* frame) {
    auto* service = BraveTalkService::GetInstance();
    service->GetDeviceID(contents_, frame->GetProcess()->GetID(),
                         frame->GetRoutingID(),
                         std::move(on_received_device_id_));
  }

  ::blink::FrameToken token() { return token_; }

 private:
  ::blink::FrameToken token_;
  content::WebContents* contents_;
  base::OnceCallback<void(const std::string&)> on_received_device_id_;
};

BraveTalkFrameHost::BraveTalkFrameHost(content::WebContents* contents,
                                       const std::string& host)
    : contents_(contents), host_(host) {}

BraveTalkFrameHost::~BraveTalkFrameHost() = default;

void BraveTalkFrameHost::BeginAdvertiseShareDisplayMedia(
    const absl::optional<::blink::FrameToken>& frame_token,
    BeginAdvertiseShareDisplayMediaCallback callback) {
  // If there is no frame token, the request is for the main frame.
  if (!frame_token) {
    BraveTalkService::GetInstance()
        ->GetDeviceID(
            contents_, contents_->GetMainFrame()->GetProcess()->GetID(),
            contents_->GetMainFrame()->GetRoutingID(), std::move(callback));
    return;
  }
  auto finder = std::make_unique<FrameFinder>(frame_token.value(), contents_,
                                              std::move(callback));

  contents_->ForEachRenderFrameHost(base::BindRepeating(
      [](FrameFinder* finder, content::RenderFrameHost* rfh) {
        if (finder->token().value() != rfh->GetFrameToken().value())
          return content::RenderFrameHost::FrameIterationAction::kContinue;
        finder->OnFoundFrame(rfh);
        return content::RenderFrameHost::FrameIterationAction::kStop;
      },
      base::Owned(std::move(finder))));
}

}  // namespace brave_talk
