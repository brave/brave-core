/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "content/browser/renderer_host/frame_tree.h"
#include "content/browser/renderer_host/frame_tree_node.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_ANDROID)
#include "content/browser/renderer_host/navigator.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#endif

namespace {

GURL GetTopDocumentGURL(content::FrameTreeNode* frame_tree_node) {
  GURL gurl;
#if BUILDFLAG(IS_ANDROID)
  // On Android, a base URL can be set for the frame. If this the case, it is
  // the URL to use for cookies.
  content::NavigationEntry* last_committed_entry =
      frame_tree_node->navigator().controller().GetLastCommittedEntry();
  if (last_committed_entry)
    gurl = last_committed_entry->GetBaseURLForDataURL();
#endif
  if (gurl.is_empty())
    gurl = frame_tree_node->frame_tree().root()->current_url();
  return gurl;
}

}  // namespace

#define BRAVE_ONREQUESTREDIRECTED_MAYBEHIDEREFERRER                   \
  BrowserContext* browser_context =                                   \
      frame_tree_node_->navigator().controller().GetBrowserContext(); \
  GetContentClient()->browser()->MaybeHideReferrer(                   \
      browser_context, common_params_->url,                           \
      GetTopDocumentGURL(frame_tree_node_), &common_params_->referrer);

#define BRAVE_ONSTARTCHECKSCOMPLETE_MAYBEHIDEREFERRER                 \
  GetContentClient()->browser()->MaybeHideReferrer(                   \
      frame_tree_node_->navigator().controller().GetBrowserContext(), \
      common_params_->url, GetTopDocumentGURL(frame_tree_node_),      \
      &common_params_->referrer);

#include "src/content/browser/renderer_host/navigation_request.cc"

#undef BRAVE_ONSTARTCHECKSCOMPLETE_MAYBEHIDEREFERRER
