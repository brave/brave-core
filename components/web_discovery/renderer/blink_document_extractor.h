/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_RENDERER_BLINK_DOCUMENT_EXTRACTOR_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_RENDERER_BLINK_DOCUMENT_EXTRACTOR_H_

#include <vector>

#include "brave/components/web_discovery/common/web_discovery.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace web_discovery {

class BlinkDocumentExtractor : public content::RenderFrameObserver,
                               public mojom::DocumentExtractor {
 public:
  BlinkDocumentExtractor(content::RenderFrame* render_frame,
                         service_manager::BinderRegistry* registry);
  ~BlinkDocumentExtractor() override;

  BlinkDocumentExtractor(const BlinkDocumentExtractor&) = delete;
  BlinkDocumentExtractor& operator=(const BlinkDocumentExtractor&) = delete;

  // mojom::DocumentExtractor:
  void QueryElementAttributes(std::vector<mojom::SelectRequestPtr> requests,
                              QueryElementAttributesCallback callback) override;

  // RenderFrameObserver:
  void OnDestruct() override;

 private:
  void BindReceiver(mojo::PendingReceiver<mojom::DocumentExtractor> receiver);

  raw_ptr<content::RenderFrame> render_frame_;
  mojo::Receiver<mojom::DocumentExtractor> receiver_{this};
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_RENDERER_BLINK_DOCUMENT_EXTRACTOR_H_
