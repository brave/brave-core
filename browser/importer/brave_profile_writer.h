/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_PROFILE_WRITER_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_PROFILE_WRITER_H_

#include <vector>

#include "base/macros.h"
#include "chrome/browser/importer/profile_writer.h"
#include "net/cookies/canonical_cookie.h"

class BraveProfileWriter : public ProfileWriter {
 public:
  explicit BraveProfileWriter(Profile* profile);

  virtual void AddCookies(const std::vector<net::CanonicalCookie>& cookies);

 protected:
  friend class base::RefCountedThreadSafe<BraveProfileWriter>;
  ~BraveProfileWriter() override;
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_PROFILE_WRITER_H_
