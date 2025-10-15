/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_DELEGATE_H_

#include <optional>

#include "content/public/browser/storage_partition_config.h"

namespace blink {
class WebMouseEvent;
}  // namespace blink

#define AddNewContents                                                        \
  AddNewContents_ChromiumImpl(                                                \
      WebContents* source, std::unique_ptr<WebContents> new_contents,         \
      const GURL& target_url, WindowOpenDisposition disposition,              \
      const blink::mojom::WindowFeatures& window_features, bool user_gesture, \
      bool* was_blocked);                                                     \
  virtual std::optional<content::StoragePartitionConfig>                      \
  MaybeInheritStoragePartition(                                               \
      WebContents* source,                                                    \
      const content::StoragePartitionConfig& partition_config);               \
  virtual WebContents* AddNewContents

// Allows delegates to handle mouse events before sending to the renderer.
// Returns true if the event was handled, false otherwise. A true value means
// no more processing should happen on the event. The default return value is
// false.
#define PreHandleKeyboardEvent(...)                     \
  PreHandleKeyboardEvent(__VA_ARGS__);                  \
  virtual bool PreHandleMouseEvent(WebContents* source, \
                                   const blink::WebMouseEvent& event)

#include <content/public/browser/web_contents_delegate.h>  // IWYU pragma: export

#undef PreHandleKeyboardEvent
#undef AddNewContents

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_DELEGATE_H_
