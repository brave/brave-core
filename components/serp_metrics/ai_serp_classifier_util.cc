/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/ai_serp_classifier_util.h"

#include "url/gurl.h"

namespace serp_metrics {

namespace {

bool IsChatGPTSerp(const GURL& url) {
  return url.host() == "chatgpt.com" && url.path().ends_with("/conversation");
}

bool IsPerplexitySerp(const GURL& url) {
  return url.host() == "www.perplexity.ai" &&
         url.path().ends_with("/perplexity_ask");
}

}  // namespace

std::optional<SerpMetricType> MaybeClassifyAISerp(const GURL& url) {
  if (IsChatGPTSerp(url)) {
    return SerpMetricType::kChatGPT;
  }

  if (IsPerplexitySerp(url)) {
    return SerpMetricType::kPerplexity;
  }

  return std::nullopt;
}

}  // namespace serp_metrics
