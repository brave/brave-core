/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BOOKMARKS_ANDROID_BOOKMARK_BRIDGE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BOOKMARKS_ANDROID_BOOKMARK_BRIDGE_H_

#include "base/threading/sequence_bound.h"
#include "chrome/browser/reading_list/android/reading_list_manager.h"
#include "components/user_data_importer/utility/bookmark_parser.h"

namespace user_data_importer {
class ContentBookmarkParser;
}  // namespace user_data_importer

#define SetReadStatus                                                       \
  ImportBookmarks(JNIEnv* env, const base::android::JavaRef<jobject>& obj,  \
                  const base::android::JavaRef<jobject>& java_window,       \
                  const base::android::JavaRef<jstring>& import_file_path); \
  void OnParseFinished(                                                     \
      user_data_importer::BookmarkParser::BookmarkParsingResult result);    \
  base::SequenceBound<user_data_importer::ContentBookmarkParser>            \
      bookmark_parser_;                                                     \
  void ExportBookmarks(                                                     \
      JNIEnv* env, const base::android::JavaRef<jobject>& obj,              \
      const base::android::JavaRef<jobject>& java_window,                   \
      const base::android::JavaRef<jstring>& export_file_path);             \
  void SetReadStatus

#include <chrome/browser/bookmarks/android/bookmark_bridge.h>  // IWYU pragma: export
#undef SetReadStatus

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BOOKMARKS_ANDROID_BOOKMARK_BRIDGE_H_
