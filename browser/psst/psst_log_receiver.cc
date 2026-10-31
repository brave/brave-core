// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_log_receiver.h"

#include <utility>

#include "base/strings/string_util.h"
#include "brave/browser/psst/psst_log_service_factory.h"
#include "brave/components/psst/core/browser/psst_log_service.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "url/origin.h"

namespace psst {

// static
void PsstLogReceiver::BindPsstLogSink(
    content::RenderFrameHost* rfh,
    mojo::PendingAssociatedReceiver<mojom::PsstLogSink> receiver) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents) {
    return;
  }
  // Drop the request instead if service is not available.
  if (!PsstLogServiceFactory::GetForBrowserContext(
          web_contents->GetBrowserContext())) {
    return;
  }

  PsstLogReceiver::GetOrCreateForWebContents(web_contents)
      ->receivers_.Bind(rfh, std::move(receiver));
}

PsstLogReceiver::PsstLogReceiver(content::WebContents* web_contents)
    : content::WebContentsUserData<PsstLogReceiver>(*web_contents),
      receivers_(web_contents, this) {
  if (auto* log_service = PsstLogServiceFactory::GetForBrowserContext(
          web_contents->GetBrowserContext())) {
    log_service_ = log_service->GetWeakPtr();
  }
}

PsstLogReceiver::~PsstLogReceiver() = default;

void PsstLogReceiver::Log(mojom::PsstLogLevel level,
                          const std::string& message,
                          std::optional<base::DictValue> data) {
  if (!log_service_) {
    return;
  }

  const auto& sender_rfh = receivers_.CurrentTargetFrame();
  const auto tab_id =
      sessions::SessionTabHelper::IdForTab(&GetWebContents()).id();

  log_service_->AddEntry(
      mojom::PsstLogSource::kScript, level, tab_id,
      sender_rfh.GetLastCommittedOrigin().GetURL(),
      base::TruncateUTF8ToByteSize(message, kMaxScriptMessageLength),
      std::move(data));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PsstLogReceiver);

}  // namespace psst
