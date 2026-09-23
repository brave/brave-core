// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_SCREENSHOT_PRINT_PREVIEW_EXTRACTOR_FACTORY_H_
#define BRAVE_BROWSER_SCREENSHOT_PRINT_PREVIEW_EXTRACTOR_FACTORY_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/unguessable_token.h"
#include "brave/browser/screenshot/print_preview_extractor.h"
#include "printing/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_PRINT_PREVIEW));

namespace screenshot {

// Constructs a PrintPreviewExtractor wired up with the default Extractor
// factory used in production (PrintPreviewExtractorInternal).
// Deps to callbacks are injected from the caller to avoid circular dependencies
// between this module and the `chrome/browser/ui/` target.
std::unique_ptr<PrintPreviewExtractor> CreatePrintPreviewExtractor(
    base::RepeatingCallback<base::flat_map<base::UnguessableToken, int>&()>
        request_id_map_callback);

}  // namespace screenshot

#endif  // BRAVE_BROWSER_SCREENSHOT_PRINT_PREVIEW_EXTRACTOR_FACTORY_H_
