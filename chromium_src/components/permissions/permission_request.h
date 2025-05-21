/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_H_

#include <optional>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

// TOOD(https://github.com/brave/brave-browser/issues/45582): Plaster issue.
// This header has to be pre-emptively included as it eventually causes the
// inclusion of components/permissions/permission_prompt.h, where
// `PermissionRequest` is used as a forward declaration, but was getting
// replaced with `PermissionRequest_ChromiumImpl`. This is another issue that
// has to be fixed with plaster.
#include "components/permissions/permission_hats_trigger_helper.h"

#define PermissionRequest PermissionRequest_ChromiumImpl
#define IsDuplicateOf IsDuplicateOf_ChromiumImpl
#include "src/components/permissions/permission_request.h"  // IWYU pragma: export
#undef IsDuplicateOf
#undef PermissionRequest

namespace permissions {

class PermissionRequest : public PermissionRequest_ChromiumImpl {
 public:
  PermissionRequest(
      std::unique_ptr<PermissionRequestData> request_data,
      PermissionDecidedCallback permission_decided_callback,
      base::OnceClosure request_finished_callback = base::DoNothing(),
      bool uses_automatic_embargo = true);

  PermissionRequest(const PermissionRequest&) = delete;
  PermissionRequest& operator=(const PermissionRequest&) = delete;

  ~PermissionRequest() override;

#if BUILDFLAG(IS_ANDROID)
  AnnotatedMessageText GetDialogAnnotatedMessageText(
      const GURL& embedding_origin) const override;

  static AnnotatedMessageText GetDialogAnnotatedMessageText(
      std::u16string requesting_origin_formatted_for_display,
      int message_id,
      bool format_origin_bold);
#endif

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
