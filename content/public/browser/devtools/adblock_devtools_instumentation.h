/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_PUBLIC_BROWSER_DEVTOOLS_ADBLOCK_DEVTOOLS_INSTUMENTATION_H_
#define BRAVE_CONTENT_PUBLIC_BROWSER_DEVTOOLS_ADBLOCK_DEVTOOLS_INSTUMENTATION_H_

#include <cstdint>
#include <optional>
#include <string>

#include "content/common/content_export.h"
#include "content/public/browser/global_routing_id.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
}

namespace content::devtools_instrumentation {

// Identifies where a matched filter/exception rule came from, on a
// best-effort basis. Only populated when the adblock engine was built in
// debug mode; otherwise absent even when a rule matched.
struct CONTENT_EXPORT AdblockFilterRuleInfo {
  // String representation of the original filter rule. May be a
  // "best effort" representation if multiple filters were fused together.
  std::string raw_line;
  // Zero-based index of the filter list source the rule came from.
  uint32_t source_index = 0;
  // Zero-indexed line number of the filter within its source list.
  uint32_t line_number = 0;
};

struct CONTENT_EXPORT AdblockInfo {
  AdblockInfo();
  ~AdblockInfo();
  AdblockInfo(const AdblockInfo&);
  AdblockInfo(AdblockInfo&&);
  AdblockInfo& operator=(const AdblockInfo&);
  AdblockInfo& operator=(AdblockInfo&&);

  // Check context
  GURL request_url;
  GURL checked_url;
  std::string source_host;
  std::optional<blink::mojom::ResourceType> resource_type;
  bool aggressive = false;

  // Result
  bool blocked = false;
  bool did_match_important_rule = false;
  bool did_match_rule = false;
  bool did_match_exception = false;
  bool has_mock_data = false;
  std::optional<std::string> rewritten_url;
  std::optional<AdblockFilterRuleInfo> filter;
  std::optional<AdblockFilterRuleInfo> exception;
};

CONTENT_EXPORT void SendAdblockInfo(
    content::GlobalRenderFrameHostToken render_frame_token,
    const std::string& request_id,
    const AdblockInfo& info);

CONTENT_EXPORT void SendAdblockInfo(content::NavigationHandle* handle,
                                    const AdblockInfo& info);

}  // namespace content::devtools_instrumentation

#endif  // BRAVE_CONTENT_PUBLIC_BROWSER_DEVTOOLS_ADBLOCK_DEVTOOLS_INSTUMENTATION_H_
