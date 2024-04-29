/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_geolocation_permission/common/brave_geolocation_permission.mojom-blink.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"

namespace blink {
namespace {

void SetEnableHighAccuracy(LocalFrame* frame, bool enable_high_accuracy);

}  // namespace
}  // namespace blink

// Pass enabledHighAccuracy bit to browser to make geolocation permission
// bubble gives more detailed infos.
// Renderer uses |Geolocation| mojo interface and it's used |WebContentsImpl|.
// It means it's in internal content layer impls so hard to get about it from
// client layer. Instead of touching |WebContents|, |Geolocation|,
// |GeolocationContext| interfaces, it would be more simple to pass via
// separated mojo interface.
#define BRAVE_START_REQUEST \
  SetEnableHighAccuracy(GetFrame(), notifier->Options()->enableHighAccuracy());

#include "src/third_party/blink/renderer/modules/geolocation/geolocation.cc"

#undef BRAVE_START_REQUEST

namespace blink {
namespace {

void SetEnableHighAccuracy(LocalFrame* frame, bool enable_high_accuracy) {
  if (frame->Client()->GetRemoteNavigationAssociatedInterfaces()) {
    mojo::AssociatedRemote<
        geolocation::mojom::blink::BraveGeolocationPermission>
        brave_geolocation_permission_binding;
    frame->Client()->GetRemoteNavigationAssociatedInterfaces()->GetInterface(
        &brave_geolocation_permission_binding);
    CHECK(brave_geolocation_permission_binding.is_bound());
    brave_geolocation_permission_binding->SetEnableHighAccuracy(
        enable_high_accuracy);
  }
}

}  // namespace
}  // namespace blink
