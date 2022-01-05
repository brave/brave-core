/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_THROTTLE_H_
#define BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_THROTTLE_H_

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "content/public/browser/web_contents.h"

class HostContentSettingsMap;

namespace de_amp {

// Throttle for AMP HTML sniffing.
// If AMP page, redirect request to non-AMP page.
class DeAmpThrottle : public blink::URLLoaderThrottle {
 public:
  explicit DeAmpThrottle(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      content::WebContents* contents);
  ~DeAmpThrottle() override;

  static std::unique_ptr<DeAmpThrottle> MaybeCreateThrottleFor(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      content::WebContents* contents);

  // Implements blink::URLLoaderThrottle.
  void WillProcessResponse(const GURL& response_url,
                           network::mojom::URLResponseHead* response_head,
                           bool* defer) override;
    // Called from DeAmpURLLoader.
  void Resume();

 private:
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  content::WebContents* contents_;  // not owned
  base::WeakPtrFactory<DeAmpThrottle> weak_factory_{this};
};

}  // namespace de_amp

#endif  // BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_THROTTLE_H_
