/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>

#include "base/files/file_path.h"

namespace crashpad {

class BraveCrashpadClient {
 public:
  bool StartHandler(const base::FilePath& handler,
                    const base::FilePath& database,
                    const base::FilePath& metrics_dir,
                    const std::string& url,
                    const std::map<std::string, std::string>& annotations,
                    const std::vector<std::string>& arguments,
                    bool restartable,
                    bool asynchronous_start);
  std::wstring GetHandlerIPCPipe() const;
  bool SetHandlerIPCPipe(const std::wstring& ipc_pipe);
};

} // namespace crashpad

namespace crash_reporter {

void DumpWithoutCrashing();
crashpad::BraveCrashpadClient& GetBraveCrashpadClient() {
  static crashpad::BraveCrashpadClient* const client =
      new crashpad::BraveCrashpadClient();
  return *client;
}

} // namespace crash_reporter

#define COMPONENTS_CRASH_CONTENT_APP_CRASHPAD_H_
#define GetCrashpadClient GetBraveCrashpadClient
#include "../../../../../../components/crash/content/app/crashpad_win.cc"
#undef GetCrashpadClient

namespace crash_reporter {
crashpad::CrashpadClient& GetCrashpadClient();
}

namespace crashpad {

bool BraveCrashpadClient::StartHandler(
    const base::FilePath& handler,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments,
    bool restartable,
    bool asynchronous_start) {

  std::string url;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  env->GetVar("BRAVE_CRASH_PARSER_URL", &url);
  if (url.empty())
    url = "https://laptop-updates.brave.com/1/bc-crashes";

  return crash_reporter::GetCrashpadClient().StartHandler(
      handler, database, metrics_dir,
      url, annotations, arguments,
      restartable, asynchronous_start);
}

std::wstring BraveCrashpadClient::GetHandlerIPCPipe() const {
  return crash_reporter::GetCrashpadClient().GetHandlerIPCPipe();
}

bool BraveCrashpadClient::SetHandlerIPCPipe(const std::wstring& ipc_pipe) {
  return crash_reporter::GetCrashpadClient().SetHandlerIPCPipe(ipc_pipe);
}

} // namespace crashpad
