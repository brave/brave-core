/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/pdfium/public/fpdf_catalog.h"

#define FPDFCatalog_IsTagged    \
  false;                        \
  if (!page_object_count) {     \
    images_.push_back(Image()); \
    return;                     \
  }                             \
  is_tagged = FPDFCatalog_IsTagged

#include "src/pdf/pdfium/pdfium_page.cc"
#undef FPDFCatalog_IsTagged
