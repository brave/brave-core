// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/print_preview_extraction/print_preview_extractor_factory.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "base/containers/id_map.h"
#include "base/functional/bind.h"
#include "brave/browser/print_preview_extraction/print_preview_extractor.h"
#include "brave/browser/print_preview_extraction/print_preview_extractor_internal.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "content/public/browser/web_contents.h"
#include "printing/mojom/print.mojom.h"

namespace ai_chat {

std::unique_ptr<PrintPreviewExtractor> CreateDefaultPrintPreviewExtractor(
    content::WebContents* web_contents) {
  return std::make_unique<PrintPreviewExtractor>(
      web_contents,
      base::BindRepeating(
          [](content::WebContents* wc, bool is_pdf,
             PrintPreviewExtractor::Extractor::ImageCallback&& callback)
              -> std::unique_ptr<PrintPreviewExtractor::Extractor> {
            return std::make_unique<PrintPreviewExtractorInternal>(
                wc, Profile::FromBrowserContext(wc->GetBrowserContext()),
                is_pdf, std::move(callback),
                base::BindRepeating(
                    []() -> base::IDMap<printing::mojom::PrintPreviewUI*>& {
                      return printing::PrintPreviewUI::
                          GetPrintPreviewUIIdMap();
                    }),
                base::BindRepeating([]() -> base::flat_map<int, int>& {
                  return printing::PrintPreviewUI::
                      GetPrintPreviewUIRequestIdMap();
                }));
          }));
}

}  // namespace ai_chat
