// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/ios/web/webui/mojo_facade.mm"

#include "base/check.h"
#include "ios/components/webui/web_ui_url_constants.h"

namespace web {
bool MojoFacade::IsWebUIMessageAllowedForFrame(const GURL& origin,
                                               NSString* prompt) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  CHECK(prompt);

  auto name_and_args =
      GetMessageNameAndArguments(base::SysNSStringToUTF8(prompt));

  // If the scheme is untrusted
  if (name_and_args.name == "Mojo.bindInterface" &&
      origin.scheme() == kChromeUIUntrustedScheme) {
    const base::Value::Dict& args = name_and_args.args;
    const std::string* interface_name = args.FindString("interfaceName");
    CHECK(interface_name);

    // Check if the requested interface is registered for this origin
    return web_state_->GetInterfaceBinderForMainFrame()->IsAllowedForOrigin(
        origin, *interface_name);
  }

  // The interface is not requested from an "untrusted" origin,
  // so let the normal code-flow handle it
  return true;
}

std::string MojoFacade::Dummy() {
  return "";
}
}  // namespace web
