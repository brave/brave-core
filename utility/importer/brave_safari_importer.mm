/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <Cocoa/Cocoa.h>

#include "brave/utility/importer/brave_safari_importer.h"

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/common/importer/importer_bridge.h"
#include "chrome/common/importer/importer_url_row.h"
#include "sql/statement.h"
#include "url/gurl.h"

BraveSafariImporter::~BraveSafariImporter() = default;

void BraveSafariImporter::ImportHistory() {
  // For importing history from History.plist.
  SafariImporter::ImportHistory();

  // From now, try to import history from History.db.
  NSString* library_dir = [NSString
      stringWithUTF8String:library_dir_.value().c_str()];
  NSString* safari_dir = [library_dir
      stringByAppendingPathComponent:@"Safari"];
  NSString* history_db = [safari_dir
      stringByAppendingPathComponent:@"History.db"];

  sql::Database db;
  const char* db_path = [history_db fileSystemRepresentation];
  if (!db.Open(base::FilePath(db_path)))
    return;

  std::vector<ImporterURLRow> rows;
  const char query[] = "SELECT hi.url, hi.visit_count, hv.visit_time, hv.title "
                       "FROM history_items as hi "
                       "JOIN history_visits as hv ON hi.id == hv.history_item";
  sql::Statement s(db.GetUniqueStatement(query));
  while (s.Step() && !cancelled()) {
    const GURL url = GURL(s.ColumnString(0));
    if (!url.is_valid())
      continue;

    ImporterURLRow row(url);
    row.visit_count = s.ColumnInt(1);
    double visit_time = s.ColumnDouble(2);
    if (!visit_time)
      continue;
    row.last_visit =
        base::Time::FromDoubleT(visit_time + kCFAbsoluteTimeIntervalSince1970);
    std::string title = s.ColumnString(3);
    if (title.empty())
      title = url.spec();
    row.title = base::UTF8ToUTF16(title);
    row.hidden = 0;
    row.typed_count = 0;
    rows.push_back(row);
  }

  if (!rows.empty() && !cancelled()) {
    bridge_->SetHistoryItems(rows, importer::VISIT_SOURCE_SAFARI_IMPORTED);
  }
}
