/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/firefox_importer.h"

#include <vector>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/utility/importer/brave_external_process_importer_bridge.h"
#include "build/build_config.h"
#include "chrome/grit/generated_resources.h"
#include "components/autofill/core/common/password_form.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_constants.h"
#include "sql/database.h"
#include "sql/statement.h"
#include "url/gurl.h"

using base::Time;

namespace brave {

FirefoxImporter::FirefoxImporter() {
}

FirefoxImporter::~FirefoxImporter() {
}

void FirefoxImporter::StartImport(const importer::SourceProfile& source_profile,
                                  uint16_t items,
                                  ImporterBridge* bridge) {
  ::FirefoxImporter::StartImport(source_profile, items, bridge);

  bridge_ = bridge;
  source_path_ = source_profile.source_path;
  if ((items & importer::COOKIES) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::COOKIES);
    ImportCookies();
    bridge_->NotifyItemEnded(importer::COOKIES);
  }

  bridge_->NotifyEnded();
}

void FirefoxImporter::ImportCookies() {
  base::FilePath file = source_path_.AppendASCII("cookies.sqlite");
  if (!base::PathExists(file)) {
    return;
  }

  sql::Database db;
  if (!db.Open(file)) {
    return;
  }

  const char query[] =
      "select baseDomain, name, value, host, path, expiry, lastAccessed, "
      "creationTime, isSecure, isHttpOnly, sameSite FROM moz_cookies";

  sql::Statement s(db.GetUniqueStatement(query));

  std::vector<net::CanonicalCookie> cookies;
  while (s.Step() && !cancelled()) {
    std::string domain(".");
    domain.append(s.ColumnString(0));
    std::string host;
    if (s.ColumnString(3)[0] == '.') {
      host.append("*");
      host.append(s.ColumnString(3));
    } else {
      host = s.ColumnString(3);
    }

    // Firefox represents expiry in *seconds* since the Unix epoch,
    // while lastAccessed and creationTime are measured in microseconds.
    // Source: netwerk/cookie/nsICookie2.idl.
    const Time expiry = Time::FromDoubleT(s.ColumnInt64(5));
    const Time last_accessed = Time::FromDoubleT(s.ColumnInt64(6) / 1000000);
    const Time creation = Time::FromDoubleT(s.ColumnInt64(7) / 1000000);
    
    auto cookie = net::CanonicalCookie(
        s.ColumnString(1),  // name
        s.ColumnString(2),  // value
        domain,  // domain
        s.ColumnString(4),  // path
        creation,  // creation
        expiry,  // expiration
        last_accessed,  // last_access
        s.ColumnBool(8),  // secure
        s.ColumnBool(9),  // http_only
        static_cast<net::CookieSameSite>(s.ColumnInt(10)),  // samesite
        net::COOKIE_PRIORITY_DEFAULT  // priority
        );
    if (cookie.IsCanonical()) {
      cookies.push_back(cookie);
    }
  }

  if (!cookies.empty() && !cancelled()) {
    bridge_->SetCookies(cookies);
  }
}

}  // namespace brave
