/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_PERMISSION_CONTROLLER_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_PERMISSION_CONTROLLER_DELEGATE_H_

#include "content/public/browser/permission_controller.h"

#define UnsubscribeFromPermissionStatusChange                               \
  PermissionControllerDelegateNotUsed() {}                                  \
  virtual void RequestPermissionsForOrigin(                                 \
      const std::vector<blink::PermissionType>& permissions,                \
      content::RenderFrameHost* render_frame_host,                          \
      const GURL& requesting_origin, bool user_gesture,                     \
      base::OnceCallback<void(                                              \
          const std::vector<blink::mojom::PermissionStatus>&)> callback) {} \
                                                                            \
  virtual blink::mojom::PermissionStatus GetPermissionStatusForOrigin(      \
      blink::PermissionType permission,                                     \
      content::RenderFrameHost* render_frame_host,                          \
      const GURL& requesting_origin);                                       \
                                                                            \
  virtual void UnsubscribeFromPermissionStatusChange

#include "src/content/public/browser/permission_controller_delegate.h"  // IWYU pragma: export

#undef UnsubscribeFromPermissionStatusChange

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_PERMISSION_CONTROLLER_DELEGATE_H_
