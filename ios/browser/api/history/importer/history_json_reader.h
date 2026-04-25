/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_HISTORY_IMPORTER_HISTORY_JSON_READER_H_
#define BRAVE_IOS_BROWSER_API_HISTORY_IMPORTER_HISTORY_JSON_READER_H_

#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "components/history/core/browser/history_types.h"

class GURL;

namespace base {
class FilePath;
}  // namespace base

namespace history_json_reader {

// Imports the history from the specified file.
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
// |history_items| is a pointer to a vector, which is filled with the imported
// history. It may not be NULL.
void ImportHistoryFile(
    base::RepeatingCallback<bool(void)> cancellation_callback,
    base::RepeatingCallback<bool(const GURL&)> valid_url_callback,
    const base::FilePath& file_path,
    std::vector<history::URLRow>* history_items);

namespace internal {

// The file format that History parses
// [url] - A string that’s the URL of the history item.
// [title] - An optional string that, if present, is the title of the history
// item. [time_usec] - An integer that’s the UNIX timestamp in microseconds of
// the latest visit to the item. [destination_url] - An optional string that, if
// present, is the URL of the next item in the redirect chain.
// [destination_time_usec] - An optional integer that’s present if
// destination_url is also present and is the UNIX timestamp (the number of
// microseconds since midnight UTC, January 1, 1970) of the next navigation in
// the redirect chain. [source_url] - An optional string that, if present, is
// the URL of the previous item in the redirect chain. [source_time_usec] - An
// optional integer that’s present if source_url is also present and is the UNIX
// timestamp in microseconds of the previous navigation in the redirect chain.
// [visits_count] - An integer that’s the number of visits the browser made to
// this item, and is always greater than or equal to 1.
// [latest_visit_was_load_failure] - An optional Boolean that’s true if Safari
// failed to load the site when someone most recently tried to access it;
// otherwise, it’s false. [latest_visit_was_http_get] - An optional Boolean
// that’s true if the last visit to this item used the HTTP GET method;
// otherwise, it’s false. Reference:
// https://developer.apple.com/documentation/safariservices/importing-data-exported-from-safari?language=objc

bool ParseHistoryItems(
    const std::string& json_data,
    std::vector<history::URLRow>* history_items,
    base::RepeatingCallback<bool(void)> cancellation_callback,
    base::RepeatingCallback<bool(const GURL&)> valid_url_callback);

}  // namespace internal

}  // namespace history_json_reader

#endif  // BRAVE_IOS_BROWSER_API_HISTORY_IMPORTER_HISTORY_JSON_READER_H_
