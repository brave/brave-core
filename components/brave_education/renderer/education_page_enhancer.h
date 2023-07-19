// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_EDUCATION_RENDERER_EDUCATION_PAGE_ENHANCER_H_
#define BRAVE_COMPONENTS_BRAVE_EDUCATION_RENDERER_EDUCATION_PAGE_ENHANCER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_education/common/mojom/brave_education.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "v8/include/v8.h"

namespace brave_education {

// A RenderFrameObserver that enhances the capabilities of "user education"
// pages on brave.com (e.g. the "what's new" page) by looking for metadata
// attached to HTML elements and "activating" those elements. For example, a
// button on the "what's new" page might be enhanced to open the settings page
// with a specific toggle highlighted.
class EducationPageEnhancer : public content::RenderFrameObserver {
 public:
  EducationPageEnhancer(content::RenderFrame* render_frame, int32_t world_id);

  // RenderFrameObserver:
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;
  void OnDestruct() override;

 private:
  // Instances are implicitly owned by the `RenderFrame` and are destroyed when
  // the `OnDestruct` event fires.
  ~EducationPageEnhancer() override;

  mojo::Remote<mojom::EducationRequestHandler>& GetRequestHandler();

  void InjectEducationRequestAPI(v8::Local<v8::Context> context);

  void ShowSettingsPageCallback(const std::string& relative_url,
                                const std::string& target);

  int32_t world_id_ = 0;
  mojo::Remote<mojom::EducationRequestHandler> request_handler_;
  base::WeakPtrFactory<EducationPageEnhancer> weak_factory_{this};
};

}  // namespace brave_education

#endif  // BRAVE_COMPONENTS_BRAVE_EDUCATION_RENDERER_EDUCATION_PAGE_ENHANCER_H_
