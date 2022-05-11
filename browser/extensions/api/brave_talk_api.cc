/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_talk_api.h"

#include <memory>
#include <string>

#include "base/environment.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "brave/browser/brave_talk/brave_talk_service.h"
#include "brave/browser/brave_talk/brave_talk_service_factory.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction BraveTalkIsSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());

  if (profile->IsTor()) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  return RespondNow(OneArgument(base::Value(true)));
}

ExtensionFunction::ResponseAction
BraveTalkBeginAdvertiseShareDisplayMediaFunction::Run() {
  auto* contents = GetSenderWebContents();
  if (!contents) return RespondNow(NoArguments());

  auto* service = brave_talk::BraveTalkServiceFactory::GetForContext(browser_context());
  service->GetDeviceID(contents, base::BindOnce(&BraveTalkBeginAdvertiseShareDisplayMediaFunction::OnDeviceIDReceived, this));

  return RespondLater();
}

void BraveTalkBeginAdvertiseShareDisplayMediaFunction::OnDeviceIDReceived(std::string device_id) {
  Respond(OneArgument(base::Value(device_id)));
}

}  // namespace api
}  // namespace extensions
