// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/webui/mojo_facade.h"

#include "base/notreached.h"
#include "base/time/time.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace web {
// Spacing between retries of a poll that came back empty, used by the
// mojo_facade.mm plaster. Defined ahead of the include below so upstream's
// body can see it.
inline constexpr base::TimeDelta kPollRetryDelay = base::Milliseconds(25);
}  // namespace web

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
// its own. The allowance covers a page whose mojo_api.js hasn't run yet,
// which fails every poll until it has; paired with kPollRetryDelay it spans
// about a second. A live frame resets it as soon as one poll carries a
// message.
bool MojoFacade::ShouldRetryPoll() {
  static constexpr int kMaxConsecutivePollFailures = 40;
  return ++consecutive_poll_failures_ <= kMaxConsecutivePollFailures;
}

// Reports what this facade knew when a message named a pipe id it doesn't
// hold, so a dump says which of these it was: the table was empty (something
// cleared it out from under the live JS context), the table was populated but
// missing the id (the handle was consumed, or never created), or the id
// belongs to a different frame.
void MojoFacade::ReportUnknownPipe(const char* operation,
                                   std::optional<int> pipe_id) {
  DUMP_WILL_BE_NOTREACHED()
      << operation << " named pipe id " << pipe_id.value_or(-1)
      << ", which is not held by the facade for host "
      << (served_host_.empty() ? std::string("<main frame>") : served_host_)
      << ", holding " << pipes_.size() << " pipe(s)";
}

}  // namespace web
