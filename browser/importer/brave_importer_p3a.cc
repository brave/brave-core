/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_importer_p3a.h"

#include "base/metrics/histogram_macros.h"

namespace {
// Note: append-only enumeration! Never remove any existing values, as this enum
// is used to bucket a UMA histogram, and removing values breaks that.
enum class ImporterSource {
  kNone,
  kBrave,
  kChrome,
  kFirefox,
  kBookmarksHTMLFile,
  kSafari,
  kIE,
  kEdge,
  kSize
};
}  // namespace

void RecordImporterP3A(importer::ImporterType type) {
  ImporterSource metric;
  switch (type) {
  case importer::TYPE_UNKNOWN:
    metric = ImporterSource::kNone;
    break;
#if defined(OS_WIN)
  case importer::TYPE_IE:
    metric = ImporterSource::kIE;
    break;
  case importer::TYPE_EDGE:
    metric = ImporterSource::kEdge;
    break;
#endif
  case importer::TYPE_FIREFOX:
    metric = ImporterSource::kFirefox;
    break;
#if defined(OS_MAC)
  case importer::TYPE_SAFARI:
    metric = ImporterSource::kSafari;
    break;
#endif
  case importer::TYPE_BOOKMARKS_FILE:
    metric = ImporterSource::kBookmarksHTMLFile;
    break;
  case importer::TYPE_CHROME:
    metric = ImporterSource::kChrome;
    break;
  }

  UMA_HISTOGRAM_ENUMERATION("Brave.Importer.ImporterSource", metric,
                            ImporterSource::kSize);
}
