// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/containers_web_contents_user_data.h"

#include <utility>

#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "content/public/browser/security_principal.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"

namespace containers {

WEB_CONTENTS_USER_DATA_KEY_IMPL(ContainersWebContentsUserData);

ContainersWebContentsUserData::ContainersWebContentsUserData(
    content::WebContents* web_contents,
    std::string container_id)
    : content::WebContentsUserData<ContainersWebContentsUserData>(
          *web_contents),
      container_id_(std::move(container_id)) {}

ContainersWebContentsUserData::~ContainersWebContentsUserData() = default;

// static
void ContainersWebContentsUserData::CopyContainerStateForDiscardedContents(
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  if (!old_contents || !new_contents) {
    return;
  }

  std::string container_id;
  const auto& config = old_contents->GetSiteInstance()
                           ->GetSecurityPrincipal()
                           .GetStoragePartitionConfig();
  if (IsContainersStoragePartition(config)) {
    container_id = config.partition_name();
  } else if (const ContainersWebContentsUserData* const old_user_data =
                 ContainersWebContentsUserData::FromWebContents(old_contents)) {
    container_id = old_user_data->container_id();
  }

  if (container_id.empty()) {
    return;
  }

  CreateForWebContents(new_contents, std::move(container_id));
}

}  // namespace containers
