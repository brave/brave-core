/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_BOOKMARK_HTML_READER_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_BOOKMARK_HTML_READER_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/strings/string16.h"
#include "components/favicon_base/favicon_usage_data.h"

class GURL;
struct ImportedBookmarkEntry;

namespace base {
class FilePath;
class Time;
}

namespace bookmark_html_reader {

// Imports the bookmarks from the specified file.
//
// |cancellation_callback| is polled to query if the import should be cancelled;
// if it returns |true| at any time the import will be cancelled. If
// |cancellation_callback| is a null callback the import will run to completion.
//
// |valid_url_callback| is called to determine if a specified URL is valid for
// import; it returns |true| if it is. If |valid_url_callback| is a null
// callback, all URLs are considered to be valid.
//
// |file_path| is the path of the file on disk to import.
//
// |bookmarks| is a pointer to a vector, which is filled with the imported
// bookmarks. It may not be NULL.
//
// |search_engines| is a pointer to a vector, which is filled with the imported
// search engines.
//
// |favicons| is a pointer to a vector, which is filled with the favicons of
// imported bookmarks. It may be NULL, in which case favicons are not imported.
void ImportBookmarksFile(
    base::RepeatingCallback<bool(void)> cancellation_callback,
    base::RepeatingCallback<bool(const GURL&)> valid_url_callback,
    const base::FilePath& file_path,
    std::vector<ImportedBookmarkEntry>* bookmarks,
    favicon_base::FaviconUsageDataList* favicons);

// Returns true if |url| should be imported as a search engine, i.e. because it
// has replacement terms. Chrome treats such bookmarks as search engines rather
// than true bookmarks.
bool CanImportURLAsSearchEngine(const GURL& url,
                                std::string* search_engine_url);

namespace internal {

// The file format that BookmarkHTMLReader parses starts with a heading
// tag, which contains its title. All bookmarks and sub-folders follow,
// bracketed by a <DL> tag:
//   <DT><H3 PERSONAL_TOOLBAR_FOLDER="true" ...>title</H3>
//   <DL><p>
//      ... container ...
//   </DL><p>
// And a bookmark is presented by a <A> tag:
//   <DT><A HREF="url" SHORTCUTURL="shortcut" ADD_DATE="11213014"...>name</A>
// Reference: http://kb.mozillazine.org/Bookmarks.html

bool ParseCharsetFromLine(const std::string& line,
                          std::string* charset);
bool ParseFolderNameFromLine(const std::string& line,
                             const std::string& charset,
                             base::string16* folder_name,
                             bool* is_toolbar_folder,
                             base::Time* add_date);
// See above, this will also put the data: URL of the favicon into |*favicon|
// if there is a favicon given. |post_data| is set for POST base keywords to
// the contents of the actual POST (with %s for the search term).
bool ParseBookmarkFromLine(const std::string& line,
                           const std::string& charset,
                           base::string16* title,
                           GURL* url,
                           GURL* favicon,
                           base::string16* shortcut,
                           base::Time* add_date,
                           base::string16* post_data);
// Save bookmarks imported from browsers with Firefox 2 compatible bookmark
// systems such as Epiphany. This bookmark format is the same as that of the
// basic Firefox 2 bookmark, but it misses additional properties and uses
// lower-case tag:
//   ...<h1>Bookmarks</h1><dl>
//   <dt><a href="url">name</a></dt>
//   <dt><a href="url">name</a></dt>
//   </dl>
bool ParseMinimumBookmarkFromLine(const std::string& line,
                                  const std::string& charset,
                                  base::string16* title,
                                  GURL* url);

}  // namespace internal

}  // namespace bookmark_html_reader

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_BOOKMARK_HTML_READER_H_
