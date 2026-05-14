// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_CONTAINERS_WEB_CONTENTS_USER_DATA_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_CONTAINERS_WEB_CONTENTS_USER_DATA_H_

#include <string>

#include "base/component_export.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}  // namespace content

namespace containers {

// Persists the container id on a WebContents when SiteInstance does not yet
// reflect the Containers storage partition (e.g. stub contents after discard).
class COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER) ContainersWebContentsUserData
    : public content::WebContentsUserData<ContainersWebContentsUserData> {
 public:
  ~ContainersWebContentsUserData() override;

  const std::string& container_id() const { return container_id_; }

  // Copies resolved container state from |old_contents| onto |new_contents|
  // when the tab's WebContents was replaced (e.g. memory-saver discard).
  static void CopyContainerStateForDiscardedContents(
      content::WebContents* old_contents,
      content::WebContents* new_contents);

 private:
  friend content::WebContentsUserData<ContainersWebContentsUserData>;

  explicit ContainersWebContentsUserData(content::WebContents* web_contents,
                                         std::string container_id);

  std::string container_id_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_CONTAINERS_WEB_CONTENTS_USER_DATA_H_
