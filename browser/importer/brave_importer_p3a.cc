/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_importer_p3a.h"

#include "base/metrics/histogram_macros.h"
#include "build/build_config.h"

namespace {
// Note: append-only enumeration! Never remove any existing values, as this enum
// is used to bucket a UMA histogram, and removing values breaks that.
//
// If changes are absolutely required, we can version the histogram (ex .2, .3).
// We need to let folks from stats know so we can update the server side also.
enum class ImporterSource {
  kNone,
  kBookmarksHTMLFile,
  kChrome,
  kFirefox,
  kMicrosoft,  // includes IE, Legacy Edge, Chromium Edge
  kOpera,
  kSafari,
  kOther,  // includes Vivaldi, Yandex and can include others
  kSize
};
}  // namespace

void RecordImporterP3A(user_data_importer::ImporterType type) {
  ImporterSource metric;
  switch (type) {
    case user_data_importer::TYPE_UNKNOWN:
      metric = ImporterSource::kNone;
      break;
#if BUILDFLAG(IS_WIN)
    case user_data_importer::TYPE_IE:
      metric = ImporterSource::kMicrosoft;
      break;
    case user_data_importer::TYPE_EDGE:
      metric = ImporterSource::kMicrosoft;
      break;
#endif
    case user_data_importer::TYPE_FIREFOX:
      metric = ImporterSource::kFirefox;
      break;
#if BUILDFLAG(IS_MAC)
    case user_data_importer::TYPE_SAFARI:
      metric = ImporterSource::kSafari;
      break;
#endif
    case user_data_importer::TYPE_BOOKMARKS_FILE:
      metric = ImporterSource::kBookmarksHTMLFile;
      break;
    case user_data_importer::TYPE_CHROME:
      metric = ImporterSource::kChrome;
      break;
    case user_data_importer::TYPE_EDGE_CHROMIUM:
      metric = ImporterSource::kMicrosoft;
      break;
    case user_data_importer::TYPE_VIVALDI:
    case user_data_importer::TYPE_YANDEX:
    case user_data_importer::TYPE_WHALE:
      metric = ImporterSource::kOther;
      break;
    case user_data_importer::TYPE_OPERA:
      metric = ImporterSource::kOpera;
      break;
  }

  UMA_HISTOGRAM_ENUMERATION("Brave.Importer.ImporterSource.2", metric,
                            ImporterSource::kSize);
}
