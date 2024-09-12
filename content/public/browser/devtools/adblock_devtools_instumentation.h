/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_PUBLIC_BROWSER_DEVTOOLS_ADBLOCK_DEVTOOLS_INSTUMENTATION_H_
#define BRAVE_CONTENT_PUBLIC_BROWSER_DEVTOOLS_ADBLOCK_DEVTOOLS_INSTUMENTATION_H_

#include <optional>
#include <string>

#include "content/common/content_export.h"
#include "content/public/browser/frame_tree_node_id.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
}

namespace content::devtools_instrumentation {

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
};

CONTENT_EXPORT void SendAdblockInfo(content::FrameTreeNodeId frame_tree_node_id,
                                    const std::string& request_id,
                                    const AdblockInfo& info);

CONTENT_EXPORT void SendAdblockInfo(content::NavigationHandle* handle,
                                    const AdblockInfo& info);

}  // namespace content::devtools_instrumentation

#endif  // BRAVE_CONTENT_PUBLIC_BROWSER_DEVTOOLS_ADBLOCK_DEVTOOLS_INSTUMENTATION_H_
