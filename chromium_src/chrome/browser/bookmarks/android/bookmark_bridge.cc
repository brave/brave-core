/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/bookmarks/android/bookmark_bridge.h"

#include <utility>

#include "base/task/thread_pool.h"
#include "base/threading/thread.h"
#include "chrome/browser/bookmarks/bookmark_html_writer.h"
#include "chrome/browser/importer/profile_writer.h"
#include "chrome/common/url_constants.h"
#include "chrome/utility/importer/bookmark_html_reader.h"
#include "components/url_formatter/url_fixer.h"

#define BraveBookmarkBridge BookmarkBridge
#include "brave/build/android/jni_headers/BraveBookmarkBridge_jni.h"
#undef BraveBookmarkBridge

#include "src/chrome/browser/bookmarks/android/bookmark_bridge.cc"
#include "ui/android/window_android.h"

using base::android::JavaParamRef;

namespace internal {

// Returns true if |url| has a valid scheme that we allow to import. We
// filter out the URL with a unsupported scheme.
// Taken from src/chrome/utility/importer/bookmarks_file_importer.cc because the
// file is not compiled on Android,
bool CanImportURL(const GURL& url) {
  // The URL is not valid.
  if (!url.is_valid()) {
    return false;
  }

  // Filter out the URLs with unsupported schemes.
  for (const char* invalid_scheme : {"wyciwyg", "place"}) {
    if (url.SchemeIs(invalid_scheme)) {
      return false;
    }
  }

  // Check if |url| is about:blank.
  if (url == url::kAboutBlankURL) {
    return true;
  }

  // If |url| starts with chrome:// or about:, check if it's one of the URLs
  // that we support.
  if (url.SchemeIs(content::kChromeUIScheme) ||
      url.SchemeIs(url::kAboutScheme)) {
    if (url.host_piece() == chrome::kChromeUIAboutHost) {
      return true;
    }

    GURL fixed_url(url_formatter::FixupURL(url.spec(), std::string()));
    const base::span<const base::cstring_view> hosts = chrome::ChromeURLHosts();
    for (const base::cstring_view host : hosts) {
      if (fixed_url.DomainIs(host)) {
        return true;
      }
    }

    if (base::Contains(chrome::ChromeDebugURLs(), fixed_url)) {
      return true;
    }

    // If url has either chrome:// or about: schemes but wasn't found in the
    // above lists, it means we don't support it, so we don't allow the user
    // to import it.
    return false;
  }

  // Otherwise, we assume the url has a valid (importable) scheme.
  return true;
}

}  // namespace internal

namespace {

class FileBookmarksExportObserver : public BookmarksExportObserver {
 public:
  explicit FileBookmarksExportObserver(const JavaParamRef<jobject>& obj)
      : obj_(ScopedJavaGlobalRef<jobject>(obj)) {}

  void OnExportFinished(Result result) override {
    JNIEnv* env = AttachCurrentThread();
    Java_BraveBookmarkBridge_bookmarksExported(env, obj_,
                                               result == Result::kSuccess);
    delete this;
  }

 private:
  const ScopedJavaGlobalRef<jobject> obj_;
};
}  // namespace

// Attempts to create a TemplateURL from the provided data. |title| is optional.
// If TemplateURL creation fails, returns null.
std::unique_ptr<TemplateURL> CreateTemplateURL(const std::u16string& url,
                                               const std::u16string& keyword,
                                               const std::u16string& title) {
  if (url.empty() || keyword.empty()) {
    return nullptr;
  }
  TemplateURLData data;
  data.SetKeyword(keyword);
  // We set short name by using the title if it exists.
  // Otherwise, we use the shortcut.
  data.SetShortName(title.empty() ? keyword : title);
  data.SetURL(TemplateURLRef::DisplayURLToURLRef(url));
  return std::make_unique<TemplateURL>(data);
}

void BookmarkBridge::ImportBookmarks(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jobject>& java_window,
    const base::android::JavaParamRef<jstring>& j_import_path) {
  ui::WindowAndroid* window =
      ui::WindowAndroid::FromJavaWindowAndroid(java_window);
  CHECK(window);

  std::u16string import_path =
      base::android::ConvertJavaStringToUTF16(env, j_import_path);

  std::vector<ImportedBookmarkEntry> bookmarks;
  std::vector<importer::SearchEngineInfo> search_engines;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&BookmarkBridge::ImportBookmarksReader,
                     base::Unretained(this), import_path, bookmarks,
                     search_engines),
      base::BindOnce(&BookmarkBridge::ImportBookmarksImpl,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BookmarkBridge::ImportBookmarksImpl(
    std::pair<std::vector<ImportedBookmarkEntry>,
              std::vector<importer::SearchEngineInfo>> importedItems) {
  std::vector<ImportedBookmarkEntry> bookmarks = get<0>(importedItems);
  std::vector<importer::SearchEngineInfo> search_engines =
      get<1>(importedItems);
  auto* writer = new ProfileWriter(profile_);

  if (!bookmarks.empty()) {
    writer->AddBookmarks(bookmarks, u"Imported");
  }

  if (!search_engines.empty()) {
    TemplateURLService::OwnedTemplateURLVector owned_template_urls;
    for (const auto& search_engine : search_engines) {
      std::unique_ptr<TemplateURL> owned_template_url = CreateTemplateURL(
          search_engine.url, search_engine.keyword, search_engine.display_name);
      if (owned_template_url) {
        owned_template_urls.push_back(std::move(owned_template_url));
      }
    }
    writer->AddKeywords(std::move(owned_template_urls), false);
  }

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj =
      ScopedJavaLocalRef<jobject>(java_bookmark_model_);
  if (obj.is_null()) {
    return;
  }

  Java_BraveBookmarkBridge_bookmarksImported(env, obj, !bookmarks.empty());
}

std::pair<std::vector<ImportedBookmarkEntry>,
          std::vector<importer::SearchEngineInfo>>
BookmarkBridge::ImportBookmarksReader(
    std::u16string import_path,
    std::vector<ImportedBookmarkEntry> bookmarks,
    std::vector<importer::SearchEngineInfo> search_engines) {
  base::FilePath import_path_ = base::FilePath::FromUTF16Unsafe(import_path);
  bookmark_html_reader::ImportBookmarksFile(
      base::RepeatingCallback<bool(void)>(),
      base::BindRepeating(internal::CanImportURL), import_path_, &bookmarks,
      &search_engines, nullptr);

  return std::make_pair(bookmarks, search_engines);
}

void BookmarkBridge::ExportBookmarks(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& java_window,
    const JavaParamRef<jstring>& j_export_path) {
  DCHECK(IsLoaded());

  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  ui::WindowAndroid* window =
      ui::WindowAndroid::FromJavaWindowAndroid(java_window);
  CHECK(window);

  std::u16string export_path =
      base::android::ConvertJavaStringToUTF16(env, j_export_path);
  base::FilePath file_export_path =
      base::FilePath::FromUTF16Unsafe(export_path);

  BookmarksExportObserver* observer = new FileBookmarksExportObserver(obj);

  bookmark_html_writer::WriteBookmarks(profile_, file_export_path, observer);
}
