// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/webui/mojo_facade.h"

#include "base/check.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "mojo/public/cpp/system/simple_watcher.h"

// Parse the FrameId from the mojom message
#define last_watch_id_                                \
  last_watch_id_;                                     \
  std::string* frame_id = args.FindString("frameId"); \
  CHECK(frame_id)

// Brave Override that responds to the specific frame that requested the data
// via mojom. Chrome-iOS responds only to the main-frame, but sub-frames can
// also bind mojom. So we need to respond to whatever frame requested the
// data, instead of always the main-frame.
#define GetMainWebFrame()                                              \
  GetFrameWithId(frame_id);                                            \
  if (!main_frame) {                                                   \
    main_frame =                                                       \
        web_state_->GetPageWorldWebFramesManager()->GetMainWebFrame(); \
  }
// Add the frame id argument to the callback
#define OnWatcherCallback(callback_id, watch_id, result) \
  OnWatcherCallback(callback_id, watch_id, std::string frame_id, result)
// Bind the frame id to the OnWatcherCallback
#define Watch(PIPE, SIGNAL, CALLBACK)                                       \
  Watch(PIPE, SIGNAL,                                                       \
        base::BindRepeating(&MojoFacade::OnWatcherCallback,                 \
                            base::Unretained(this), *callback_id, watch_id, \
                            *frame_id));                                    \
  (void)callback

#include <ios/web/webui/mojo_facade.mm>

#undef Watch
#undef OnWatcherCallback
#undef GetMainWebFrame
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
      origin.scheme() == "chrome-untrusted") {
    const base::DictValue& args = name_and_args.args;
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
