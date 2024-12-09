// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_RENDERER_BRAVE_RENDER_FRAME_OBSERVER_H_
#define BRAVE_RENDERER_BRAVE_RENDER_FRAME_OBSERVER_H_

#include <string>

#include "content/common/content_export.h"
#include "content/public/renderer/render_frame_observer.h"
#include "services/service_manager/public/cpp/binder_registry.h"

class CONTENT_EXPORT BraveRenderFrameObserver
    : public content::RenderFrameObserver {
 public:
  explicit BraveRenderFrameObserver(content::RenderFrame* render_frame);
  BraveRenderFrameObserver(const BraveRenderFrameObserver&) = delete;
  BraveRenderFrameObserver& operator=(const BraveRenderFrameObserver&) = delete;

  service_manager::BinderRegistry* registry() { return &registry_; }

  // content::RenderFrameObserver:
  void OnDestruct() override;
  void OnInterfaceRequestForFrame(
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;

 protected:
  ~BraveRenderFrameObserver() override;

 private:
  service_manager::BinderRegistry registry_;
};

#endif  // BRAVE_RENDERER_BRAVE_RENDER_FRAME_OBSERVER_H_
