// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_PADDED_IMAGE_SOURCE_H_
#define BRAVE_BROWSER_UI_WEBUI_PADDED_IMAGE_SOURCE_H_

#include <memory>
#include <string>

#include "chrome/browser/ui/webui/sanitized_image_source.h"

class Profile;

class PaddedImageSource : public SanitizedImageSource {
 public:
  explicit PaddedImageSource(Profile* profile);
  ~PaddedImageSource() override;

 protected:
    void OnImageLoaded(std::unique_ptr<network::SimpleURLLoader> loader,
                       RequestAttributes request_attributes,
                       content::URLDataSource::GotDataCallback callback,
                       std::unique_ptr<std::string> body) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_PADDED_IMAGE_SOURCE_H_
