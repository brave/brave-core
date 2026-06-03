// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_YOUTUBE_YOUTUBE_NETWORK_CHANGE_OBSERVER_H_
#define BRAVE_IOS_BROWSER_YOUTUBE_YOUTUBE_NETWORK_CHANGE_OBSERVER_H_

#include "base/memory/raw_ptr.h"
#include "ios/web/public/web_state_user_data.h"
#include "net/base/network_change_notifier.h"

namespace web {
class WebState;
}

namespace youtube {

// A network change observer that observes when a users network drops to a
// cellular connection to possibly reset a youtube pages video quality back to
// auto.
class YouTubeNetworkChangeObserver
    : public web::WebStateUserData<YouTubeNetworkChangeObserver>,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  ~YouTubeNetworkChangeObserver() override;

  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

 private:
  friend class web::WebStateUserData<YouTubeNetworkChangeObserver>;
  explicit YouTubeNetworkChangeObserver(web::WebState* web_state);

  raw_ptr<web::WebState> web_state_;
};

}  // namespace youtube

#endif  // BRAVE_IOS_BROWSER_YOUTUBE_YOUTUBE_NETWORK_CHANGE_OBSERVER_H_
