/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "brave/utility/importer/brave_external_process_importer_bridge.h"

#include "base/logging.h"
#include "build/build_config.h"

using chrome::mojom::ProfileImportObserver;

namespace {

const int kNumCookiesToSend = 100;

}  // namespace

void BraveExternalProcessImporterBridge::SetCookies(
    const std::vector<net::CanonicalCookie>& cookies) {
  (*observer_)->OnCookiesImportStart(cookies.size());

  // |cookies_left| is required for the checks below as Windows has a
  // Debug bounds-check which prevents pushing an iterator beyond its end()
  // (i.e., |it + 2 < s.end()| crashes in debug mode if |i + 1 == s.end()|).
  int cookies_left = cookies.end() - cookies.begin();
  for (std::vector<net::CanonicalCookie>::const_iterator it =
           cookies.begin(); it < cookies.end();) {
    std::vector<net::CanonicalCookie> cookies_group;
    std::vector<net::CanonicalCookie>::const_iterator end_group =
        it + std::min(cookies_left, kNumCookiesToSend);
    cookies_group.assign(it, end_group);

    (*observer_)->OnCookiesImportGroup(cookies_group);
    cookies_left -= end_group - it;
    it = end_group;
  }
  DCHECK_EQ(0, cookies_left);
}

BraveExternalProcessImporterBridge::BraveExternalProcessImporterBridge(
    const base::DictionaryValue& localized_strings,
    scoped_refptr<chrome::mojom::ThreadSafeProfileImportObserverPtr> observer)
  : ExternalProcessImporterBridge(localized_strings, observer) {}

BraveExternalProcessImporterBridge::~BraveExternalProcessImporterBridge() {}
