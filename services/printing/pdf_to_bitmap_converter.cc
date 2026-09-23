// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/printing/pdf_to_bitmap_converter.h"

#include <optional>
#include <utility>

#include "base/logging.h"
#include "base/memory/shared_memory_mapping.h"
#include "pdf/pdf.h"
#include "printing/units.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/size_conversions.h"

namespace printing {

PdfToBitmapConverter::PdfToBitmapConverter() = default;

PdfToBitmapConverter::~PdfToBitmapConverter() = default;

void PdfToBitmapConverter::GetPdfPageCount(
    base::ReadOnlySharedMemoryRegion pdf_region,
    GetPdfPageCountCallback callback) {
  base::ReadOnlySharedMemoryMapping pdf_map = pdf_region.Map();
  if (!pdf_map.IsValid()) {
    DLOG(ERROR) << "Failed to decode memory map for PDF";
    std::move(callback).Run(std::nullopt);
    return;
  }
  auto pdf_buffer = pdf_map.GetMemoryAsSpan<const uint8_t>();
  int page_count;
  if (!chrome_pdf::GetPDFDocInfo(pdf_buffer, &page_count, nullptr)) {
    DLOG(ERROR) << "Failed to get PDF document info";
    std::move(callback).Run(std::nullopt);
    return;
  }
  std::move(callback).Run(page_count);
}

void PdfToBitmapConverter::GetBitmap(
    base::ReadOnlySharedMemoryRegion pdf_region,
    uint32_t page_index,
    GetBitmapCallback callback) {
  base::ReadOnlySharedMemoryMapping pdf_map = pdf_region.Map();
  if (!pdf_map.IsValid()) {
    DLOG(ERROR) << "Failed to decode memory map for PDF";
    std::move(callback).Run(SkBitmap());
    return;
  }
  auto pdf_buffer = pdf_map.GetMemoryAsSpan<const uint8_t>();

  // Get the original size of the PDF page in points. Each point is 1/72 inch
  // per chrome_pdf::CalculatePosition and printing::kPointsPerInch.
  std::optional<gfx::SizeF> page_size =
      chrome_pdf::GetPDFPageSizeByIndex(pdf_buffer, page_index);
  if (!page_size.has_value()) {
    DLOG(ERROR) << "Failed to get PDF page size";
    std::move(callback).Run(SkBitmap());
    return;
  }
  // Scale up the page size from 72 dpi to 300 dpi
  gfx::SizeF size_300dpi = gfx::ScaleSize(
      page_size.value(),
      static_cast<float>(printing::kDefaultPdfDpi) / printing::kPointsPerInch);
  gfx::Size size = gfx::ToCeiledSize(size_300dpi);

  // Allocate a bitmap with the scaled up size so it can fit in later rendered
  // 300 dpi PDF page image.
  SkBitmap bitmap;
  const SkImageInfo info = SkImageInfo::Make(
      size.width(), size.height(), kBGRA_8888_SkColorType, kOpaque_SkAlphaType);
  if (!bitmap.tryAllocPixels(info, info.minRowBytes())) {
    DLOG(ERROR) << "Failed to allocate bitmap pixels";
    std::move(callback).Run(SkBitmap());
    return;
  }

  // Convert PDF bytes into a bitmap
  chrome_pdf::RenderOptions options = {
      .stretch_to_bounds = false,
      .keep_aspect_ratio = true,
      .autorotate = false,
      .use_color = true,
      .render_device_type = chrome_pdf::RenderDeviceType::kDisplay,
  };
  // Render the PDF page to a bitmap with 300 dpi with scaled up size.
  if (!chrome_pdf::RenderPDFPageToBitmap(
          pdf_buffer, page_index, bitmap.getPixels(), size,
          gfx::Size(printing::kDefaultPdfDpi, printing::kDefaultPdfDpi),
          options)) {
    DLOG(ERROR) << "Failed to render PDF buffer as bitmap image";
    std::move(callback).Run(SkBitmap());
    return;
  }

  std::move(callback).Run(std::move(bitmap));
}

void PdfToBitmapConverter::SetUseSkiaRendererPolicy(bool use_skia) {
  chrome_pdf::SetUseSkiaRendererPolicy(use_skia);
}

}  // namespace printing
