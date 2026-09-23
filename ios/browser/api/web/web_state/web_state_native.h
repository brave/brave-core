/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/memory/weak_ptr.h"
#include "components/sessions/core/session_id.h"
#include "ios/web/public/web_state_observer.h"
#include "url/gurl.h"

#ifndef BRAVE_IOS_BROWSER_API_WEB_WEB_STATE_WEB_STATE_NATIVE_H_
#define BRAVE_IOS_BROWSER_API_WEB_WEB_STATE_WEB_STATE_NATIVE_H_

class Browser;
class GURL;

namespace brave {

class NativeWebState final {
 public:
  NativeWebState(Browser* browser, bool off_the_record);
  ~NativeWebState();

  void SetTitle(const std::u16string& title);
  void SetURL(const GURL& url);
  base::WeakPtr<web::WebState> GetWeakWebState();

 private:
  class Observer : public web::WebStateObserver {
   public:
    explicit Observer(NativeWebState* tab);
    Observer(const Observer&) = delete;
    Observer& operator=(const Observer&) = delete;
    ~Observer() override;

   private:
    // WebStateObserver:
    void WebStateDestroyed(web::WebState* web_state) override;
    raw_ptr<NativeWebState> native_state_;
  };

  raw_ptr<Browser> browser_;
  SessionID session_id_;
  raw_ptr<web::WebState> web_state_;
  std::unique_ptr<Observer> web_state_observer_;
};

}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_API_WEB_WEB_STATE_WEB_STATE_NATIVE_H_
