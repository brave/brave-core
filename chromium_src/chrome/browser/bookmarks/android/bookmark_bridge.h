/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BOOKMARKS_ANDROID_BOOKMARK_BRIDGE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BOOKMARKS_ANDROID_BOOKMARK_BRIDGE_H_

#include <utility>

#include "chrome/browser/reading_list/android/reading_list_manager.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/importer_data_types.h"

#define SetReadStatus                                                    \
  ImportBookmarks(                                                       \
      JNIEnv* env, const base::android::JavaParamRef<jobject>& obj,      \
      const base::android::JavaParamRef<jobject>& java_window,           \
      const base::android::JavaParamRef<jstring>& import_file_path);     \
  void ImportBookmarksImpl(                                              \
      std::pair<std::vector<ImportedBookmarkEntry>,                      \
                std::vector<importer::SearchEngineInfo>> importedItems); \
  std::pair<std::vector<ImportedBookmarkEntry>,                          \
            std::vector<importer::SearchEngineInfo>>                     \
  ImportBookmarksReader(                                                 \
      std::u16string import_file_path,                                   \
      std::vector<ImportedBookmarkEntry> bookmarks,                      \
      std::vector<importer::SearchEngineInfo> search_engines);           \
  void ExportBookmarks(                                                  \
      JNIEnv* env, const base::android::JavaParamRef<jobject>& obj,      \
      const base::android::JavaParamRef<jobject>& java_window,           \
      const base::android::JavaParamRef<jstring>& export_file_path);     \
  void SetReadStatus

#include "src/chrome/browser/bookmarks/android/bookmark_bridge.h"  // IWYU pragma: export
#undef SetReadStatus

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BOOKMARKS_ANDROID_BOOKMARK_BRIDGE_H_
