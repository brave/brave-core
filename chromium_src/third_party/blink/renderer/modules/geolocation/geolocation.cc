/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "services/device/public/mojom/geolocation.mojom-blink.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
#include "brave/components/brave_geolocation_permission/common/brave_geolocation_permission.mojom-blink.h"
#endif

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
namespace blink {
namespace {

bool SetEnableHighAccuracy(LocalFrame* frame, bool enable_high_accuracy) {
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

  return enable_high_accuracy;
}

}  // namespace
}  // namespace blink

// Pass enabledHighAccuracy bit to browser to make geolocation permission
// bubble gives more detailed infos.
// Renderer uses |Geolocation| mojo interface and it's used |WebContentsImpl|.
// It means it's in internal content layer impls so hard to get about it from
// client layer. Instead of touching |WebContents|, |Geolocation|,
// |GeolocationContext| interfaces, it would be more simple to pass via
// separated mojo interface.
#define SetHighAccuracyHint(is_high_accuracy) \
  SetHighAccuracyHint(SetEnableHighAccuracy(GetFrame(), is_high_accuracy));
#else
#define SetHighAccuracyHint(is_high_accuracy) \
  SetHighAccuracyHint(is_high_accuracy)
#endif

#include "src/third_party/blink/renderer/modules/geolocation/geolocation.cc"

#undef SetHighAccuracyHint
