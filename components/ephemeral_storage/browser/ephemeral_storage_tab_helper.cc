/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/browser/ephemeral_storage_tab_helper.h"

#include <string>

#include "base/feature_list.h"
#include "components/content_settings/core/common/features.h"
#include "content/browser/storage_partition_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/renderer/modules/storage/brave_dom_window_storage.h"

using content::BrowserContext;
using content::NavigationHandle;
using content::SessionStorageNamespaceImpl;
using content::WebContents;

namespace ephemeral_storage {

EphemeralStorageTabHelper::~EphemeralStorageTabHelper() {}

EphemeralStorageTabHelper::EphemeralStorageTabHelper(WebContents* web_contents)
    : WebContentsObserver(web_contents) {
  CreateEphemeralStorageNamespcesIfNeeded();
}

void EphemeralStorageTabHelper::CreateEphemeralStorageNamespcesIfNeeded() {
  if (!base::FeatureList::IsEnabled(blink::kBraveEphemeralStorage))
    return;

  if (ephemeral_session_storage_namespace_ &&
      ephemeral_local_storage_namespace_)
    return;

  WebContents* contents = web_contents();
  content::StoragePartition* storage_partition =
      contents->GetMainFrame()->GetStoragePartition();
  if (!storage_partition)
    return;

  content::DOMStorageContextWrapper* dom_storage_context =
      static_cast<content::DOMStorageContextWrapper*>(
          storage_partition->GetDOMStorageContext());
  if (!dom_storage_context)
    return;

  std::string ephemeral_session_namespace_id;
  std::string ephemeral_local_namespace_id;
  if (!GenerateEphemeralStorageNamespcesIds(ephemeral_session_namespace_id,
                                            ephemeral_local_namespace_id))
    return;

  ephemeral_session_storage_namespace_ = SessionStorageNamespaceImpl::Create(
      dom_storage_context, ephemeral_session_namespace_id);
  ephemeral_local_storage_namespace_ = SessionStorageNamespaceImpl::Create(
      dom_storage_context, ephemeral_local_namespace_id);
}

bool EphemeralStorageTabHelper::GenerateEphemeralStorageNamespcesIds(
    std::string& ephemeral_session_namespace_id,
    std::string& ephemeral_local_namespace_id) {
  WebContents* contents = web_contents();
  content::SessionStorageNamespace* session_namespace =
      contents->GetController().GetDefaultSessionStorageNamespace();
  if (!session_namespace)
    return false;

  ephemeral_session_namespace_id =
      session_namespace->id() + "ephemeral-session-storage";
  ephemeral_local_namespace_id =
      session_namespace->id() + "ephemeral-local-storage";
  return true;
}

void EphemeralStorageTabHelper::ReadyToCommitNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame())
    return;
  if (navigation_handle->IsSameDocument())
    return;

  ClearEphemeralStorage();
}

void EphemeralStorageTabHelper::WebContentsDestroyed() {
  ClearEphemeralStorage();
}

void EphemeralStorageTabHelper::ClearEphemeralStorage() {
  if (!base::FeatureList::IsEnabled(blink::kBraveEphemeralStorage)) {
    return;
  }

  WebContents* contents = web_contents();
  contents->GetBrowserContext()->ClearEphemeralStorageForHost(
      contents->GetRenderViewHost(), contents->GetSiteInstance());
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(EphemeralStorageTabHelper)

}  // namespace ephemeral_storage
