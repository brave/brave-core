// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/webui/mojo_facade.h"

#include "ios/components/webui/web_ui_url_constants.h"
#include "url/gurl.h"
#include "url/origin.h"

#include <ios/web/webui/mojo_facade.mm>

// The methods below are declared via the mojo_facade.h plaster
// (rewrite/ios/web/webui/mojo_facade.h.yaml) and hooked into upstream's
// control flow via the mojo_facade.mm plaster
// (rewrite/ios/web/webui/mojo_facade.mm.yaml). They're defined here, after
// the #include above, so they can rely on the headers upstream's own .mm
// pulls in transitively.
namespace web {

// A facade constructed without a frame serves the main frame. Its identity
// can change over the WebState's life (a new document brings a new main
// frame), so it's tracked by IsMainFrame() rather than by id.
bool MojoFacade::IsMainFrameFacade() const {
  return served_frame_id_.empty();
}

bool MojoFacade::ServesFrame(WebFrame* frame) const {
  return IsMainFrameFacade() ? frame->IsMainFrame()
                             : frame->GetFrameId() == served_frame_id_;
}

void MojoFacade::EnsureSubFrameFacade(WebFrame* frame) {
  if (!IsMainFrameFacade() || !frame || frame->IsMainFrame()) {
    return;
  }
  // Only WebUI documents ever speak mojo. Polling anything else would spin
  // forever, since a frame without mojo_api.js throws on every poll and
  // OnAwaitNextMessageCompleted retries on error.
  const GURL origin = frame->GetSecurityOrigin().GetURL();
  if (!origin.SchemeIs(kChromeUIScheme) &&
      !origin.SchemeIs(kChromeUIUntrustedScheme)) {
    return;
  }
  if (!web_state_->GetInterfaceBinderForMainFrame()
           ->HasRegisteredInterfaces()) {
    return;
  }
  std::unique_ptr<MojoFacade>& facade = sub_frame_facades_[frame->GetFrameId()];
  if (!facade) {
    facade = std::make_unique<MojoFacade>(web_state_, frame);
  }
  // The constructor only polls if the WebState isn't mid-load, which it may
  // well have been when the frame first appeared.
  facade->AwaitNextMessage();
}

// Gates Mojo.bindInterface calls from chrome-untrusted:// origins against
// InterfaceBinder::IsAllowedForOrigin (brave/chromium_src/ios/web/public/
// web_state.h), which upstream never checks. The origin is taken from the
// frame this facade serves, established natively when it was constructed, so
// page script can't nominate a frame other than its own.
bool MojoFacade::IsBindInterfaceAllowedForFrame(const base::DictValue& args) {
  WebFrame* frame = GetMainWebFrame();
  if (!frame) {
    return true;
  }
  GURL origin = frame->GetSecurityOrigin().GetURL();
  if (!origin.SchemeIs(kChromeUIUntrustedScheme)) {
    return true;
  }
  const std::string* interface_name = args.FindString("interfaceName");
  return interface_name &&
         web_state_->GetInterfaceBinderForMainFrame()->IsAllowedForOrigin(
             origin, *interface_name);
}

// Records what this facade knew when a message named a pipe id it doesn't
// hold, so a report says which of these it was: the table was empty
// (something cleared it out from under the live JS context), the table was
// populated but missing the id (the handle was consumed, or never created),
// or the id belongs to a different frame.
// A frame whose document can't run scripts any more never recovers, and
// upstream re-posts the poll on every failure, so the loop has to give up on
// its own. The allowance is for genuinely transient failures; a live frame
// resets it as soon as one poll carries a message.
bool MojoFacade::ShouldRetryPoll() {
  static constexpr int kMaxConsecutivePollFailures = 3;
  return ++consecutive_poll_failures_ <= kMaxConsecutivePollFailures;
}

void MojoFacade::ReportUnknownPipe(const char* operation,
                                   std::optional<int> pipe_id) {
  SCOPED_CRASH_KEY_STRING32("MojoFacade", "operation", operation);
  SCOPED_CRASH_KEY_NUMBER("MojoFacade", "pipe_id", pipe_id.value_or(-1));
  SCOPED_CRASH_KEY_NUMBER("MojoFacade", "live_pipes",
                          static_cast<int>(pipes_.size()));
  SCOPED_CRASH_KEY_STRING32(
      "MojoFacade", "frame",
      served_frame_id_.empty() ? std::string("main") : served_frame_id_);
  base::debug::DumpWithoutCrashing();
}

}  // namespace web
