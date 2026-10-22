/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "gpu/ipc/service/image_transport_surface_overlay_mac.h"

#include "build/build_config.h"

#if BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_IOS_TVOS)
#include "gpu/ipc/common/ios/be_layer_hierarchy_transport.h"
#import "ui/accelerated_widget_mac/ca_layer_frame_sink_provider.h"

namespace gpu {
namespace {

// The in-process GPU transport drops the layer hierarchy, so host it directly.
// The lookup only succeeds when the view lives in this process.
void HostLayerHierarchyInProcess(SurfaceHandle surface_handle,
                                 BELayerHierarchy* hierarchy) {
  dispatch_async(dispatch_get_main_queue(), ^{
    CALayerFrameSinkProvider* sink =
        [CALayerFrameSinkProvider lookupByHandle:surface_handle];
    if (sink) {
      sink.handle = hierarchy.handle;
    }
  });
}

}  // namespace
}  // namespace gpu

#define ForwardBELayerHierarchyToBrowser(surface_handle, ipc_representation) \
  ForwardBELayerHierarchyToBrowser(surface_handle, ipc_representation);      \
  HostLayerHierarchyInProcess(surface_handle, layer_hierarchy_)
#endif

#include <gpu/ipc/service/image_transport_surface_overlay_mac.mm>

#if BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_IOS_TVOS)
#undef ForwardBELayerHierarchyToBrowser
#endif
