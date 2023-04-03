/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_util.h"

#include <memory>
#include <tuple>
#include <utility>

#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/rust/ffi/speedreader.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"

namespace speedreader {

bool PageSupportsDistillation(DistillState state) {
  return state == DistillState::kSpeedreaderOnDisabledPage ||
         state == DistillState::kPageProbablyReadable;
}

bool PageStateIsDistilled(DistillState state) {
  return state == DistillState::kReaderMode ||
         state == DistillState::kSpeedreaderMode;
}

bool PageWantsDistill(DistillState state) {
  return state == DistillState::kReaderMode ||
         state == DistillState::kSpeedreaderMode ||
         state == DistillState::kReaderModePending ||
         state == DistillState::kSpeedreaderModePending;
}

void SetEnabledForSite(HostContentSettingsMap* map,
                       const GURL& url,
                       bool enable) {
  DCHECK(!url.is_empty());  // Not supported. Disable Speedreader in settings.

  // Rule covers all protocols and pages.
  auto pattern = ContentSettingsPattern::FromString("*://" + url.host() + "/*");
  if (!pattern.IsValid()) {
    return;
  }

  ContentSetting setting =
      enable ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK;
  map->SetContentSettingCustomScope(pattern, ContentSettingsPattern::Wildcard(),
                                    ContentSettingsType::BRAVE_SPEEDREADER,
                                    setting);
}

bool IsEnabledForSite(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::BRAVE_SPEEDREADER);
  const bool enabled =
      setting == CONTENT_SETTING_ALLOW || setting == CONTENT_SETTING_DEFAULT;
  return enabled;
}

void DistillPage(const GURL& url,
                 std::string body,
                 SpeedreaderService* speedreader_service,
                 SpeedreaderRewriterService* rewriter_service,
                 DistillationResultCallback callback) {
  struct Result {
    DistillationResult result;
    std::string body;
    std::string transformed;
  };

  auto distill = [](const GURL& url, std::string data,
                    std::unique_ptr<Rewriter> rewriter) -> Result {
    SCOPED_UMA_HISTOGRAM_TIMER("Brave.Speedreader.Distill");
    int written = rewriter->Write(data.c_str(), data.length());
    // Error occurred
    if (written != 0) {
      return {DistillationResult::kFail, std::move(data), std::string()};
    }

    rewriter->End();
    const std::string& transformed = rewriter->GetOutput();

    // If the distillation failed, the rewriter returns an empty string. Also,
    // if the output is too small, we assume that the content of the distilled
    // page does not contain enough text to read.
    if (transformed.length() < 1024) {
      return {DistillationResult::kFail, std::move(data), std::string()};
    }
    return {DistillationResult::kSuccess, std::move(data), transformed};
  };

  auto return_result = [](DistillationResultCallback callback, Result r) {
    std::move(callback).Run(r.result, r.body, r.transformed);
  };

  auto rewriter = rewriter_service->MakeRewriter(
      url, speedreader_service->GetThemeName(),
      speedreader_service->GetFontFamilyName(),
      speedreader_service->GetFontSizeName(), "");

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_BLOCKING, base::MayBlock()},
      base::BindOnce(distill, url, std::move(body), std::move(rewriter)),
      base::BindOnce(return_result, std::move(callback)));
}

}  // namespace speedreader
