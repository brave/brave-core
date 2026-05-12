// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_JS_MESSAGING_JS_PROMPT_COMMUNICATION_HANDLER_IMPL_H_
#define BRAVE_IOS_WEB_JS_MESSAGING_JS_PROMPT_COMMUNICATION_HANDLER_IMPL_H_

#include <set>

#include "base/memory/raw_ptr.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web/public/web_state_user_data.h"
#include "ios/web/web_state/ui/crw_wk_ui_handler.h"

class GURL;

namespace web {

class WebState;

class JSPromptCommunicationHandlerImpl
    : public JSPromptCommunicationHandler,
      public WebStateUserData<JSPromptCommunicationHandlerImpl>,
      public WebStateObserver,
      public WebFramesManager::Observer {
 public:
  ~JSPromptCommunicationHandlerImpl() override;

  JSPromptCommunicationHandlerImpl(const JSPromptCommunicationHandlerImpl&) =
      delete;
  JSPromptCommunicationHandlerImpl& operator=(
      const JSPromptCommunicationHandlerImpl&) = delete;

  bool HandleJavaScriptPrompt(GURL request_url,
                              bool is_main_frame,
                              NSString* prompt,
                              NSString** result) override;

  // WebStateObserver:
  void DidStartNavigation(WebState* web_state,
                          NavigationContext* navigation_context) override;
  void PageLoaded(WebState* web_state,
                  PageLoadCompletionStatus load_completion_status) override;
  void WebFrameBecameAvailable(WebFramesManager* web_frames_manager,
                               WebFrame* web_frame) override;
  void WebStateDestroyed(WebState* web_state) override;

 private:
  explicit JSPromptCommunicationHandlerImpl(WebState* web_state);
  friend class WebStateUserData<JSPromptCommunicationHandlerImpl>;

  raw_ptr<WebState> web_state_;
  bool is_handling_blocked_on_main_frame_ = false;
};

}  // namespace web

#endif  // BRAVE_IOS_WEB_JS_MESSAGING_JS_PROMPT_COMMUNICATION_HANDLER_IMPL_H_
