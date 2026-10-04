// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/screenshot/print_preview_extractor_factory.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/unguessable_token.h"
#include "brave/browser/screenshot/print_preview_extractor.h"
#include "brave/browser/screenshot/print_preview_extractor_internal.h"
#include "chrome/browser/profiles/profile.h"

namespace screenshot {

std::unique_ptr<PrintPreviewExtractor> CreatePrintPreviewExtractor(
    base::RepeatingCallback<base::flat_map<base::UnguessableToken, int>&()>
        request_id_map_callback) {
  return std::make_unique<PrintPreviewExtractor>(base::BindRepeating(
      [](base::RepeatingCallback<base::flat_map<base::UnguessableToken, int>&()>
             request_id_map_callback,
         content::WebContents* wc, bool is_pdf,
         PrintPreviewExtractor::CaptureImagesCallback&& callback)
          -> std::unique_ptr<PrintPreviewExtractor::Extractor> {
        return std::make_unique<PrintPreviewExtractorInternal>(
            wc, Profile::FromBrowserContext(wc->GetBrowserContext()), is_pdf,
            std::move(callback), std::move(request_id_map_callback));
      },
      std::move(request_id_map_callback)));
}

}  // namespace screenshot
