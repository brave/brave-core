// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/webui/mojo_facade.h"

#include "base/logging.h"
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

// A facade built without a host serves the main frame, which is what
// upstream assumes when only one WebUI exists per WebState. Brave's
// per-host WebUIs each name the frame they belong to.
bool MojoFacade::ServesFrame(WebFrame* frame) const {
  if (served_host_.empty()) {
    return frame->IsMainFrame();
  }
  return frame->GetSecurityOrigin().host() == served_host_;
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

// A frame whose document can't run scripts any more never recovers, and
// upstream re-posts the poll on every failure, so the loop has to give up on
// its own. The allowance is for genuinely transient failures; a live frame
// resets it as soon as one poll carries a message.
bool MojoFacade::ShouldRetryPoll() {
  static constexpr int kMaxConsecutivePollFailures = 3;
  return ++consecutive_poll_failures_ <= kMaxConsecutivePollFailures;
}

// Records what this facade knew when a message named a pipe id it doesn't
// hold, so a report says which of these it was: the table was empty
// (something cleared it out from under the live JS context), the table was
// populated but missing the id (the handle was consumed, or never created),
// or the id belongs to a different frame.
void MojoFacade::ReportUnknownPipe(const char* operation,
                                   std::optional<int> pipe_id) {
  SCOPED_CRASH_KEY_STRING32("MojoFacade", "operation", operation);
  SCOPED_CRASH_KEY_NUMBER("MojoFacade", "pipe_id", pipe_id.value_or(-1));
  SCOPED_CRASH_KEY_NUMBER("MojoFacade", "live_pipes",
                          static_cast<int>(pipes_.size()));
  SCOPED_CRASH_KEY_STRING32(
      "MojoFacade", "host",
      served_host_.empty() ? std::string("main") : served_host_);
  base::debug::DumpWithoutCrashing();
}

// TEMPORARY diagnostics. Revert before landing. `facade` distinguishes two
// facades polling one frame (which clobbers the single resolver slot JS keeps
// for fetchNextMessageFromJS) from one facade cycling normally.
namespace {
std::string HostLabel(const std::string& served_host) {
  return served_host.empty() ? std::string("MAIN") : served_host;
}
}  // namespace

void MojoFacade::LogPollStart() {
  LOG(ERROR) << "[mojo] poll-start facade=" << this
             << " host=" << HostLabel(served_host_)
             << " frame=" << GetMainFrameId()
             << " awaiting=" << is_awaiting_message_;
}

void MojoFacade::LogPollDone(const std::string& web_frame_id,
                             const base::Value* value,
                             NSError* error) {
  LOG(ERROR) << "[mojo] poll-done  facade=" << this
             << " host=" << HostLabel(served_host_)
             << " reported=" << web_frame_id << " error="
             << (error ? base::SysNSStringToUTF8(error.description)
                       : std::string("none"))
             << " value="
             << (value ? base::WriteJson(*value).value_or("<unconvertible>")
                       : std::string("<null>"));
}

}  // namespace web
