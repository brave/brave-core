/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_REDUCE_LANGUAGE_THROTTLE_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_REDUCE_LANGUAGE_THROTTLE_H_

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "url/gurl.h"

class HostContentSettingsMap;

namespace content {
class WebContents;
}  // namespace content

namespace brave_shields {

class ReduceLanguageThrottle : public blink::URLLoaderThrottle {
 public:
  explicit ReduceLanguageThrottle(const content::WebContents::Getter& wc_getter,
                                  HostContentSettingsMap* content_settings);
  ~ReduceLanguageThrottle() override;

  ReduceLanguageThrottle(const ReduceLanguageThrottle&) =
      delete;
  ReduceLanguageThrottle& operator=(
      const ReduceLanguageThrottle&) = delete;

  static std::unique_ptr<ReduceLanguageThrottle> MaybeCreateThrottleFor(
      const content::WebContents::Getter& wc_getter,
      HostContentSettingsMap* content_settings);

  // blink::URLLoaderThrottle implementation:
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;

 private:
  content::WebContents::Getter wc_getter_;
  HostContentSettingsMap* content_settings_ = nullptr;

  base::WeakPtrFactory<ReduceLanguageThrottle> weak_ptr_factory_{
      this};
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_REDUCE_LANGUAGE_THROTTLE_H_
