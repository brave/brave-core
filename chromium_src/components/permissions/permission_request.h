/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_H_

#include <optional>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

#define PermissionRequest PermissionRequest_ChromiumImpl
#define IsDuplicateOf IsDuplicateOf_ChromiumImpl
#include "src/components/permissions/permission_request.h"  // IWYU pragma: export
#undef IsDuplicateOf
#undef PermissionRequest

namespace permissions {

class PermissionRequest : public PermissionRequest_ChromiumImpl {
 public:
  PermissionRequest(const GURL& requesting_origin,
                    RequestType request_type,
                    bool has_gesture,
                    PermissionDecidedCallback permission_decided_callback,
                    base::OnceClosure delete_callback);

  PermissionRequest(PermissionRequestData request_data,
                    PermissionDecidedCallback permission_decided_callback,
                    base::OnceClosure delete_callback,
                    bool uses_automatic_embargo);

  PermissionRequest(const PermissionRequest&) = delete;
  PermissionRequest& operator=(const PermissionRequest&) = delete;

  ~PermissionRequest() override;

  bool SupportsLifetime() const;
  void SetLifetime(std::optional<base::TimeDelta> lifetime);
  const std::optional<base::TimeDelta>& GetLifetime() const;

  void set_dont_ask_again(bool dont_ask_again) {
    dont_ask_again_ = dont_ask_again;
  }
  bool get_dont_ask_again() const { return dont_ask_again_; }

  // We rename upstream's IsDuplicateOf() via a define above and re-declare it
  // here to workaround the fact that the PermissionRequest_ChromiumImpl rename
  // will affect this method's only parameter too, which will break subclasses.
  virtual bool IsDuplicateOf(PermissionRequest* other_request) const;

  // Returns a weak pointer to this instance.
  base::WeakPtr<PermissionRequest> GetWeakPtr();

 private:
  std::optional<base::TimeDelta> lifetime_;

  bool dont_ask_again_ = false;

  base::WeakPtrFactory<PermissionRequest> weak_factory_{this};
};

}  // namespace permissions

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_H_
