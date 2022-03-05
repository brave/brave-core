/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_THROTTLE_H_
#define BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_THROTTLE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/body_sniffer/body_sniffer_throttle.h"
#include "brave/components/de_amp/browser/de_amp_service.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"

class HostContentSettingsMap;

namespace de_amp {

// Throttle for AMP HTML detection.
// If AMP page, redirect request to non-AMP canonical link.
class DeAmpThrottle : public body_sniffer::BodySnifferThrottle {
 public:
  explicit DeAmpThrottle(scoped_refptr<base::SequencedTaskRunner> task_runner,
                         DeAmpService* service,
                         network::ResourceRequest request,
                         const content::WebContents::Getter& wc_getter);
  ~DeAmpThrottle() override;
  DeAmpThrottle& operator=(const DeAmpThrottle&);

  static std::unique_ptr<DeAmpThrottle> MaybeCreateThrottleFor(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      DeAmpService* service,
      network::ResourceRequest request,
      const content::WebContents::Getter& wc_getter);

  // Implements blink::URLLoaderThrottle.
  void WillProcessResponse(const GURL& response_url,
                           network::mojom::URLResponseHead* response_head,
                           bool* defer) override;

  void Redirect(const GURL& new_url);

 private:
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  raw_ptr<DeAmpService> service_;
  network::ResourceRequest request_;
  content::WebContents::Getter wc_getter_;
};

}  // namespace de_amp

#endif  // BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_THROTTLE_H_
