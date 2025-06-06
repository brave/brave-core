/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/utility/importer/safari_importer.h"

#include <Cocoa/Cocoa.h>

#include <vector>

#include "base/logging.h"
#include "chrome/grit/generated_resources.h"
#include "components/user_data_importer/common/imported_bookmark_entry.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

int GetBookmarkGroupFromSafariID() {
  return IDS_BOOKMARK_GROUP_FROM_SAFARI;
}

constexpr char16_t kSafariReadingListPath[] = u"com.apple.ReadingList";

void CorrectSafariReadingListPath(
    std::vector<user_data_importer::ImportedBookmarkEntry>& bookmarks) {
  for (auto& item : bookmarks) {
    for (auto& folder : item.path) {
      if (folder == kSafariReadingListPath) {
        folder = l10n_util::GetStringUTF16(IDS_BOOKMARKS_READING_LIST_GROUP);
      }
    }
  }
}
}  // namespace

#undef IDS_BOOKMARK_GROUP_FROM_SAFARI
#define IDS_BOOKMARK_GROUP_FROM_SAFARI GetBookmarkGroupFromSafariID()); \
  CorrectSafariReadingListPath(bookmarks
#include "src/chrome/utility/importer/safari_importer.mm"
#undef IDS_BOOKMARK_GROUP_FROM_SAFARI
#define IDS_BOOKMARK_GROUP_FROM_SAFARI GetBookmarkGroupFromSafariID()
