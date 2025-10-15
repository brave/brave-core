/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/web_contents/web_contents_impl.h"

namespace content {

bool WebContentsImpl::ShouldDoLearning() {
  if (!ShouldDoLearning_ChromiumImpl()) {
    return false;
  }
  return !GetContentClient()->browser()->IsWindowsRecallDisabled();
}

bool WebContentsImpl::GetShouldDoLearningForTesting() {
  return ShouldDoLearning();
}

}  // namespace content

#define ShouldDoLearning(...) ShouldDoLearning_ChromiumImpl(__VA_ARGS__)

#define BRAVE_WEB_CONTENTS_IMPL_CREATE_NEW_WINDOW_INHERIT_STORAGE_PARTITION \
  if (delegate_) {                                                          \
    auto inherited_storage_partition_config =                               \
        delegate_->MaybeInheritStoragePartition(this, partition_config);    \
    if (inherited_storage_partition_config) {                               \
      site_instance = SiteInstance::CreateForFixedStoragePartition(         \
          GetBrowserContext(), params.target_url,                           \
          *inherited_storage_partition_config);                             \
    }                                                                       \
  }

#include <content/browser/web_contents/web_contents_impl.cc>

#undef ShouldDoLearning
#undef BRAVE_WEB_CONTENTS_IMPL_CREATE_NEW_WINDOW_INHERIT_STORAGE_PARTITION

namespace content {

bool WebContentsImpl::PreHandleMouseEvent(const blink::WebMouseEvent& event) {
  OPTIONAL_TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("content.verbose"),
                        "WebContentsImpl::PreHandleMouseEvent");
  return delegate_ ? delegate_->PreHandleMouseEvent(this, event) : false;
}

}  // namespace content
