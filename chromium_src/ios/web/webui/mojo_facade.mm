// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/ios/web/webui/mojo_facade.mm"

namespace web {
bool MojoFacade::IsWebUIMessageAllowedForFrame(WKFrameInfo* frame,
                                               const GURL& origin,
                                               NSString** prompt) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  CHECK(prompt && *prompt);

  auto name_and_args =
      GetMessageNameAndArguments(base::SysNSStringToUTF8(*prompt));

  // If the scheme is untrusted
  if (name_and_args.name == "Mojo.bindInterface" &&
      origin.scheme() == "chrome-untrusted") {
    const base::Value::Dict& args = name_and_args.args;
    const std::string* interface_name = args.FindString("interfaceName");
    CHECK(interface_name);

    // Check if the requested interface is registered for this origin
    bool is_allowed =
        web_state_->GetInterfaceBinderForMainFrame()->IsAllowedForOrigin(
            origin, *interface_name);

    // If the interface is not allowed, set the prompt to invalid.
    // HandleMojoMessage will not bind it.
    if (!is_allowed) {
      *prompt = @"{\"name\":\"Mojo.invalidInterface\",\"args\":{}}";
    }
  }

  return true;
}

std::string MojoFacade::Dummy() {
  return "";
}
}  // namespace web
