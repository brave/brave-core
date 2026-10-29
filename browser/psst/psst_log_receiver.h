// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_LOG_RECEIVER_H_
#define BRAVE_BROWSER_PSST_PSST_LOG_RECEIVER_H_

#include <cstddef>
#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/psst/core/browser/psst_log_service.h"
#include "brave/components/psst/core/common/psst_ui_common.mojom.h"
#include "content/public/browser/render_frame_host_receiver_set.h"
#include "content/public/browser/web_contents_user_data.h"

namespace psst {

// Browser end of `mojom::PsstLogSink`: receives log messages from the injected
// user/policy scripts and forwards them to the profile's `PsstLogService`.
//
// Created on demand, when a renderer in the PSST isolated world asks for the
// interface. The renderer only supplies the level, message and payload; the
// tab id, frame token and site are derived here from the validated sending
// frame, so a compromised renderer cannot attribute an entry to another tab or
// site.
class PsstLogReceiver : public content::WebContentsUserData<PsstLogReceiver>,
                        public mojom::PsstLogSink {
 public:
  // Scripts can log arbitrary strings; longer messages are truncated so one
  // entry can't dominate the buffer or the internals page.
  static constexpr size_t kMaxScriptMessageLength = 4096;

  PsstLogReceiver(const PsstLogReceiver&) = delete;
  PsstLogReceiver& operator=(const PsstLogReceiver&) = delete;
  ~PsstLogReceiver() override;

  static void BindPsstLogSink(
      content::RenderFrameHost* rfh,
      mojo::PendingAssociatedReceiver<mojom::PsstLogSink> receiver);

 private:
  friend class content::WebContentsUserData<PsstLogReceiver>;

  explicit PsstLogReceiver(content::WebContents* web_contents);

  // mojom::PsstLogSink:
  void Log(mojom::PsstLogLevel level,
           const std::string& message,
           std::optional<base::DictValue> data) override;

  base::WeakPtr<PsstLogService> log_service_;
  content::RenderFrameHostReceiverSet<mojom::PsstLogSink> receivers_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_LOG_RECEIVER_H_
