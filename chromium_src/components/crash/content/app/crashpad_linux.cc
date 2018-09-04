/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>

#include "base/files/file_path.h"

namespace crashpad {

class BraveCrashpadClient {
 public:
  bool StartHandlerAtCrash(const base::FilePath& handler,
                           const base::FilePath& database,
                           const base::FilePath& metrics_dir,
                           const std::string& url,
                           const std::map<std::string, std::string>& annotations,
                           const std::vector<std::string>& arguments);
  static bool StartHandlerForClient(const base::FilePath& handler,
                                    const base::FilePath& database,
                                    const base::FilePath& metrics_dir,
                                    const std::string& url,
                                    const std::map<std::string, std::string>& annotations,
                                    const std::vector<std::string>& arguments,
                                    int socket);
};

} // namespace crashpad

namespace crash_reporter {

crashpad::BraveCrashpadClient& GetBraveCrashpadClient() {
  static crashpad::BraveCrashpadClient* const client =
      new crashpad::BraveCrashpadClient();
  return *client;
}

} // namespace crash_reporter

#define COMPONENTS_CRASH_CONTENT_APP_CRASHPAD_H_
#define GetCrashpadClient GetBraveCrashpadClient
#include "../../../../../../components/crash/content/app/crashpad_linux.cc"
#undef GetCrashpadClient

namespace crash_reporter {
crashpad::CrashpadClient& GetCrashpadClient();
}

namespace crashpad {

bool BraveCrashpadClient::StartHandlerAtCrash(
    const base::FilePath& handler,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments) {
  return crash_reporter::GetCrashpadClient().StartHandlerAtCrash(
      handler, database, metrics_dir,
      "https://laptop-updates.brave.com/1/bc-crashes", annotations, arguments);
}

// static
bool BraveCrashpadClient::StartHandlerForClient(
    const base::FilePath& handler,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments,
    int socket) {
  return crash_reporter::GetCrashpadClient().StartHandlerForClient(
    handler, database, metrics_dir, "https://laptop-updates.brave.com/1/bc-crashes",
    annotations, arguments, socket);
}

} // namespace crashpad
