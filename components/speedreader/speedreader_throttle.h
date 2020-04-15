/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_THROTTLE_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_THROTTLE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/speedreader/speedreader_switches.h"
#include "brave/components/speedreader/speedreader_whitelist.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace speedreader {

// Launches the speedreader distillation pass over a reponce body, deferring
// the load until distillation is done.
// TODO(iefremov): Avoid distilling the same page twice (see comments in
// blink::URLLoaderThrottle)?
// TODO(iefremov): Check throttles order?
// Cargoculted from |MimeSniffingThrottle|.
class SpeedReaderThrottle : public blink::URLLoaderThrottle {
 public:
  // |task_runner| is used to bind the right task runner for handling incoming
  // IPC in SpeedReaderLoader. |task_runner| is supposed to be bound to the
  // current sequence.
  SpeedReaderThrottle(SpeedreaderWhitelist* whitelist,
                      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~SpeedReaderThrottle() override;

  SpeedReaderThrottle(const SpeedReaderThrottle&) = delete;
  SpeedReaderThrottle& operator=(const SpeedReaderThrottle&) = delete;

  // Implements blink::URLLoaderThrottle.
  void WillProcessResponse(const GURL& response_url,
                           network::mojom::URLResponseHead* response_head,
                           bool* defer) override;

  // Called from SpeedReaderURLLoader.
  void Resume();

 private:
  SpeedreaderWhitelist* speedreader_whitelist_;  // not owned
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::WeakPtrFactory<SpeedReaderThrottle> weak_factory_{this};
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_THROTTLE_H_
