// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/chrome/services/printing/printing_service.cc"

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "brave/services/printing/pdf_to_bitmap_converter.h"
#endif

namespace printing {
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)

void PrintingService::BindPdfToBitmapConverter(
    mojo::PendingReceiver<mojom::PdfToBitmapConverter> receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<printing::PdfToBitmapConverter>(), std::move(receiver));
}

#endif
}  // namespace printing
