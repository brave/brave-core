// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CONTENT_YOUTUBE_INJECTOR_HOST_H_
#define BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CONTENT_YOUTUBE_INJECTOR_HOST_H_

#include "brave/components/youtube_script_injector/common/youtube_injector.mojom.h"
#include "url/gurl.h"

namespace youtube_script_injector {

class COMPONENT_EXPORT(YOUTUBE_SCRIPT_INJECTOR_BROWSER_CONTENT)
    YouTubeInjectorHost final
    : public youtube_script_injector::mojom::YouTubeInjector {
 public:
  YouTubeInjectorHost(const YouTubeInjectorHost&) = delete;
  YouTubeInjectorHost& operator=(const YouTubeInjectorHost&) = delete;

  YouTubeInjectorHost(const GURL& url);
  ~YouTubeInjectorHost() override;

  // youtube_script_injector::mojom::YouTubeInjector:
  void NativePipMode() override;

 private:
  const GURL url_;
};

}  // namespace youtube_script_injector

#endif  // BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CONTENT_YOUTUBE_INJECTOR_HOST_H_
