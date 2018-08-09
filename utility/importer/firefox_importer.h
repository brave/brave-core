/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_IMPORTER_FIREFOX_IMPORTER_H_
#define BRAVE_UTILITY_IMPORTER_FIREFOX_IMPORTER_H_

#include "chrome/utility/importer/firefox_importer.h"

namespace brave {

class FirefoxImporter : public ::FirefoxImporter {
 public:
  FirefoxImporter();

  // Importer:
  void StartImport(const importer::SourceProfile& source_profile,
                   uint16_t items,
                   ImporterBridge* bridge) override;

 private:
  ~FirefoxImporter() override;

  void ImportCookies();

  base::FilePath source_path_;

  DISALLOW_COPY_AND_ASSIGN(FirefoxImporter);
};

}  // namespace brave

#endif  // BRAVE_UTILITY_IMPORTER_FIREFOX_IMPORTER_H_
