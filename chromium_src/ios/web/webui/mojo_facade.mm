// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/webui/mojo_facade.h"

#include "base/check.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "mojo/public/cpp/system/simple_watcher.h"

// Parse the FrameId from the mojom message
#define last_watch_id_                                \
  last_watch_id_;                                     \
  std::string* frame_id = args.FindString("frameId"); \
  CHECK(frame_id)

// Bind the FrameId to the OnWatcherCallback
#define Watch(PIPE, SIGNAL, CALLBACK)                                       \
  Watch(PIPE, SIGNAL,                                                       \
        base::BindRepeating(&MojoFacade::OnWatcherCallback_BraveImpl,       \
                            base::Unretained(this), *callback_id, watch_id, \
                            *frame_id));                                    \
  (void)callback

#include <ios/web/webui/mojo_facade.mm>

#undef Watch
#undef last_watch_id_

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

// Brave Override that responds to the specific frame that requested the data
// via mojom. Chrome-iOS responds only to the main-frame, but sub-frames can
// also bind mojom. So we need to respond to whatever frame requested the data,
// instead of always the main-frame.
void MojoFacade::OnWatcherCallback_BraveImpl(int callback_id,
                                             int watch_id,
                                             std::string frame_id,
                                             MojoResult result) {
  // Respond to the frame that requested the data, identified by its frame_id
  web::WebFrame* frame =
      web_state_->GetPageWorldWebFramesManager()->GetFrameWithId(frame_id);
  if (!frame) {
    return;
  }

  NSString* script =
      [NSString stringWithFormat:
                    @"Mojo.internal.watchCallbacksHolder.callCallback(%d, %d)",
                    callback_id, result];
  auto callback = base::BindOnce(
      [](base::WeakPtr<MojoFacade> facade, int watch_id, const base::Value*,
         NSError*) {
        if (facade) {
          facade->ArmOnNotifyWatcher(watch_id);
        }
      },
      weak_ptr_factory_.GetWeakPtr(), watch_id);
  frame->ExecuteJavaScript(base::SysNSStringToUTF16(script),
                           std::move(callback));
}

std::string MojoFacade::Dummy() {
  return "";
}

}  // namespace web
