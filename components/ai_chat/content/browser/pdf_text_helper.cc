// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/pdf_text_helper.h"

#include <utility>

#include "base/barrier_callback.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/types/fixed_array.h"
#include "components/pdf/browser/pdf_document_helper.h"
#include "content/public/browser/web_contents.h"
#include "pdf/mojom/pdf.mojom.h"

namespace ai_chat {

namespace {

void OnAllPagesTextReceived(
    PdfTextCallback callback,
    std::vector<std::pair<size_t, std::string>> page_texts) {
  base::FixedArray<std::string_view> ordered_texts(page_texts.size());
  for (const auto& [index, text] : page_texts) {
    ordered_texts[index] = text;
  }
  std::move(callback).Run(base::JoinString(ordered_texts, "\n"));
}

void OnGetPdfPageCount(content::WebContents* web_contents,
                       PdfTextCallback callback,
                       pdf::mojom::PdfListener::GetPdfBytesStatus status,
                       const std::vector<uint8_t>& bytes,
                       uint32_t page_count) {
  auto* pdf_helper =
      pdf::PDFDocumentHelper::MaybeGetForWebContents(web_contents);
  if (status == pdf::mojom::PdfListener::GetPdfBytesStatus::kFailed ||
      !pdf_helper) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  auto barrier_callback = base::BarrierCallback<std::pair<size_t, std::string>>(
      page_count, base::BindOnce(&OnAllPagesTextReceived, std::move(callback)));

  for (uint32_t i = 0; i < page_count; ++i) {
    pdf_helper->GetPageText(
        i, base::BindOnce(
               [](base::OnceCallback<void(std::pair<size_t, std::string>)>
                      barrier_cb,
                  size_t page_index, const std::u16string& page_text) {
                 std::move(barrier_cb)
                     .Run(std::make_pair(page_index,
                                         base::UTF16ToUTF8(page_text)));
               },
               barrier_callback, static_cast<size_t>(i)));
  }
}

}  // namespace

void ExtractTextFromLoadedPdf(content::WebContents* web_contents,
                              PdfTextCallback callback) {
  auto* pdf_helper =
      pdf::PDFDocumentHelper::MaybeGetForWebContents(web_contents);
  if (!pdf_helper) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  pdf_helper->GetPdfBytes(
      /*size_limit=*/0,
      base::BindOnce(&OnGetPdfPageCount, web_contents, std::move(callback)));
}

}  // namespace ai_chat
