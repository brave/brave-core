/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_talk_api.h"

#include <memory>
#include <string>

#include "base/environment.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "chrome/browser/ui/browser_finder.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction BraveTalkIsSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());

  if (profile->IsTor()) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  return RespondNow(OneArgument(base::Value(true)));
}

ExtensionFunction::ResponseAction BraveTalkBeginAdvertiseShareDisplayMediaFunction::Run() {
  LOG(ERROR) << "Huh. Made it here";
  
  auto* contents = content::WebContents::FromRenderFrameHost(render_frame_host());
  auto* browser = chrome::FindBrowserWithWebContents(contents);
  LOG(ERROR) << "Found Browser: " << browser;
  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
