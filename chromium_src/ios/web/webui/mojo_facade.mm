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
// the #include above, so they can use file-scope helpers upstream's own .mm
// defines (e.g. FindIntOrDoubleAsInt) and rely on its own transitively
// included headers (e.g. for base::SequencedTaskRunner, WebThread,
// base::SysNSStringToUTF16).
namespace web {

// Resolves which frame a watch's callback should be delivered to, falling
// back to the main frame exactly as upstream's own GetMainWebFrame() would.
web::WebFrame* MojoFacade::GetFrameForWatch(int watch_id) {
  auto it = watch_id_to_frame_id_.find(watch_id);
  if (it != watch_id_to_frame_id_.end()) {
    if (WebFrame* frame =
            web_state_->GetPageWorldWebFramesManager()->GetFrameWithId(
                it->second)) {
      return frame;
    }
  }
  return web_state_->GetPageWorldWebFramesManager()->GetMainWebFrame();
}

// Erases every watch (and its watchers_ entry) that `frame_id` created via
// MojoHandle.watch, so a sub-frame's watches don't outlive it. Without this,
// a stale watch_id_to_frame_id_ entry would make a later notification for it
// fall through GetFrameForWatch to the main frame instead of being dropped.
void MojoFacade::EraseWatchersForFrame(const std::string& frame_id) {
  for (auto it = watch_id_to_frame_id_.begin();
       it != watch_id_to_frame_id_.end();) {
    if (it->second == frame_id) {
      watchers_.erase(it->first);
      it = watch_id_to_frame_id_.erase(it);
    } else {
      ++it;
    }
  }
}

// Mirrors upstream's own AwaitNextMessage(), parameterized by frame instead
// of hardcoded to main_frame_, so sub-frames that bind mojo interfaces (e.g.
// AI Chat's untrusted conversation-entries iframe) get polled too.
void MojoFacade::AwaitNextMessageForFrame(const std::string& frame_id) {
  DCHECK_CURRENTLY_ON(WebThread::UI);

  if (extra_polling_frame_ids_[frame_id]) {
    return;
  }

  WebFrame* frame =
      web_state_->GetPageWorldWebFramesManager()->GetFrameWithId(frame_id);
  if (!frame) {
    return;
  }

  extra_polling_frame_ids_[frame_id] = true;

  auto callback =
      base::BindOnce(&MojoFacade::OnAwaitNextMessageForFrameCompleted,
                     weak_ptr_factory_.GetWeakPtr(), frame_id);

  std::u16string fetch_next_message =
      u"return await Mojo.internal.fetchNextMessageFromJS();";

  frame->ExecuteAsyncJavaScript(fetch_next_message, base::DictValue(),
                                std::move(callback));
}

// Mirrors upstream's own OnAwaitNextMessageCompleted(): dispatches into the
// existing HandleMojoMessage(), routes the response back via
// Mojo.internal.messageReceived on this same frame, then re-arms.
void MojoFacade::OnAwaitNextMessageForFrameCompleted(
    const std::string& frame_id,
    const base::Value* value,
    NSError* error) {
  DCHECK_CURRENTLY_ON(WebThread::UI);

  extra_polling_frame_ids_[frame_id] = false;

  WebFrame* frame =
      web_state_->GetPageWorldWebFramesManager()->GetFrameWithId(frame_id);
  bool frame_still_alive = !web_state_->IsBeingDestroyed() && frame != nullptr;

  if (error || !frame_still_alive || !value) {
    if (error && frame_still_alive) {
      base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE, base::BindOnce(&MojoFacade::AwaitNextMessageForFrame,
                                    weak_ptr_factory_.GetWeakPtr(), frame_id));
    }
    return;
  }

  const base::DictValue* dict = value->GetIfDict();
  if (dict) {
    std::optional<int> message_id = FindIntOrDoubleAsInt(*dict, "message_id");
    const base::DictValue* message = dict->FindDict("message");

    if (message_id && message) {
      base::WeakPtr<WebFrame> weak_frame = frame->AsWeakPtr();
      HandleMojoMessage(
          *message_id, message,
          base::BindOnce(^(int msg_id, std::string response) {
            WebFrame* target_frame = weak_frame.get();
            if (!target_frame) {
              return;
            }
            NSString* response_str = @"null";
            if (!response.empty()) {
              response_str = base::SysUTF8ToNSString(response);
            }
            NSString* script = [NSString
                stringWithFormat:@"Mojo.internal.messageReceived(%d, %@)",
                                 msg_id, response_str];
            target_frame->ExecuteAsyncJavaScript(
                base::SysNSStringToUTF16(script), base::DictValue(),
                base::DoNothing());
          }));
    }
  }
  AwaitNextMessageForFrame(frame_id);
}

// Gates Mojo.bindInterface calls from chrome-untrusted:// origins against
// InterfaceBinder::IsAllowedForOrigin (brave/chromium_src/ios/web/public/
// web_state.h), which upstream never checks.
bool MojoFacade::IsBindInterfaceAllowedForFrame(const base::DictValue& args) {
  const std::string* frame_id = args.FindString("frameId");
  if (!frame_id) {
    return true;
  }
  WebFrame* frame =
      web_state_->GetPageWorldWebFramesManager()->GetFrameWithId(*frame_id);
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
