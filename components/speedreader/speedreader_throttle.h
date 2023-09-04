/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_THROTTLE_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_THROTTLE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/components/body_sniffer/body_sniffer_throttle.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "url/gurl.h"

namespace speedreader {

class SpeedreaderThrottleDelegate;
class SpeedreaderRewriterService;
class SpeedreaderService;

// Launches the speedreader distillation pass over a response body, deferring
// the load until distillation is done.
// TODO(iefremov): Avoid distilling the same page twice (see comments in
// blink::URLLoaderThrottle)?
// TODO(iefremov): Check throttles order?
// Cargoculted from |MimeSniffingThrottle| -- refactored common functionality
// between SpeedReader and de-amp urlloader / throttle into
// components/body_sniffer
class SpeedReaderThrottle : public body_sniffer::BodySnifferThrottle {
 public:
  ~SpeedReaderThrottle() override;

  // |task_runner| is used to bind the right task runner for handling incoming
  // IPC in SpeedReaderLoader. |task_runner| is supposed to be bound to the
  // current sequence.
  SpeedReaderThrottle(SpeedreaderRewriterService* rewriter_service,
                      SpeedreaderService* speedreader_service,
                      base::WeakPtr<SpeedreaderThrottleDelegate> delegate,
                      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  static std::unique_ptr<SpeedReaderThrottle> MaybeCreateThrottleFor(
      SpeedreaderRewriterService* rewriter_service,
      SpeedreaderService* speedreader_service,
      base::WeakPtr<SpeedreaderThrottleDelegate> delegate,
      const GURL& url,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  // Implements blink::URLLoaderThrottle.
  void WillProcessResponse(const GURL& response_url,
                           network::mojom::URLResponseHead* response_head,
                           bool* defer) override;

 private:
  void StartSpeedReaderLocalUrlLoader(const GURL& response_url);
  void StartSpeedReaderUrlLoader(const GURL& response_url);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  raw_ptr<SpeedreaderRewriterService> rewriter_service_ = nullptr;  // not owned
  raw_ptr<SpeedreaderService> speedreader_service_ = nullptr;
  base::WeakPtr<SpeedreaderThrottleDelegate> speedreader_delegate_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_THROTTLE_H_
