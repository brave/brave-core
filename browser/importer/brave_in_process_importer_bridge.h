/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_IN_PROCESS_IMPORTER_BRIDGE_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_IN_PROCESS_IMPORTER_BRIDGE_H_

#include <string>
#include <vector>

#include "brave/browser/importer/brave_profile_writer.h"
#include "chrome/browser/importer/in_process_importer_bridge.h"
#include "net/cookies/canonical_cookie.h"

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"

class BraveInProcessImporterBridge : public InProcessImporterBridge {
 public:
  BraveInProcessImporterBridge(
      ProfileWriter* writer,
      base::WeakPtr<ExternalProcessImporterHost> host);

  void SetCookies(
      const std::vector<net::CanonicalCookie>& cookies) override;

 private:
  ~BraveInProcessImporterBridge() override;

  BraveProfileWriter* const writer_;  // weak

  DISALLOW_COPY_AND_ASSIGN(BraveInProcessImporterBridge);
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_IN_PROCESS_IMPORTER_BRIDGE_H_
