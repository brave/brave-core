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

}  // namespace web
