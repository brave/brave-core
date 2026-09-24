// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_SERVICES_PRINTING_PDF_TO_BITMAP_CONVERTER_H_
#define BRAVE_SERVICES_PRINTING_PDF_TO_BITMAP_CONVERTER_H_

#include "base/memory/read_only_shared_memory_region.h"
#include "brave/services/printing/public/mojom/pdf_to_bitmap_converter.mojom.h"

namespace printing {

// Implements mojom interface for generating images for each PDF pages.
// The PDF file needs to be read and stored in a shared memory region.
class PdfToBitmapConverter : public printing::mojom::PdfToBitmapConverter {
 public:
  PdfToBitmapConverter();
  ~PdfToBitmapConverter() override;

  PdfToBitmapConverter(const PdfToBitmapConverter&) = delete;
  PdfToBitmapConverter& operator=(const PdfToBitmapConverter&) = delete;

  // printing::mojom::PdfToBitmapConverter:
  void GetPdfPageCount(base::ReadOnlySharedMemoryRegion pdf_region,
                       GetPdfPageCountCallback callback) override;
  void GetBitmap(base::ReadOnlySharedMemoryRegion pdf_region,
                 uint32_t page_index,
                 GetBitmapCallback callback) override;

  void SetUseSkiaRendererPolicy(bool use_skia) override;
};

}  // namespace printing

#endif  // BRAVE_SERVICES_PRINTING_PDF_TO_BITMAP_CONVERTER_H_
