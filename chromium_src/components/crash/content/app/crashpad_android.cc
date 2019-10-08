/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>

#include "base/files/file_path.h"

namespace crashpad {

namespace {
const char brave_crash_url[] = "https://laptop-updates.brave.com/1/bc-crashes";
}

class BraveCrashpadClient {
 public:
#if defined(OS_ANDROID)

  static bool StartJavaHandlerAtCrash(
      const std::string& class_name,
      const std::vector<std::string>* env,
      const base::FilePath& database,
      const base::FilePath& metrics_dir,
      const std::string& url,
      const std::map<std::string, std::string>& annotations,
      const std::vector<std::string>& arguments);

  static bool StartJavaHandlerForClient(
      const std::string& class_name,
      const std::vector<std::string>* env,
      const base::FilePath& database,
      const base::FilePath& metrics_dir,
      const std::string& url,
      const std::map<std::string, std::string>& annotations,
      const std::vector<std::string>& arguments,
      int socket);

  static bool StartHandlerWithLinkerAtCrash(
      const std::string& handler_trampoline,
      const std::string& handler_library,
      bool is_64_bit,
      const std::vector<std::string>* env,
      const base::FilePath& database,
      const base::FilePath& metrics_dir,
      const std::string& url,
      const std::map<std::string, std::string>& annotations,
      const std::vector<std::string>& arguments);

  static bool StartHandlerWithLinkerForClient(
      const std::string& handler_trampoline,
      const std::string& handler_library,
      bool is_64_bit,
      const std::vector<std::string>* env,
      const base::FilePath& database,
      const base::FilePath& metrics_dir,
      const std::string& url,
      const std::map<std::string, std::string>& annotations,
      const std::vector<std::string>& arguments,
      int socket);

#endif  // defined(OS_ANDROID)

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
#include "../../../../../../components/crash/content/app/crashpad_android.cc"
#undef GetCrashpadClient

namespace crash_reporter {
crashpad::CrashpadClient& GetCrashpadClient();
}

namespace crashpad {

#if defined(OS_ANDROID)

// static
bool BraveCrashpadClient::StartJavaHandlerAtCrash(
    const std::string& class_name,
    const std::vector<std::string>* env,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments) {
  return CrashpadClient::StartJavaHandlerAtCrash(
      class_name, env, database, metrics_dir,
      brave_crash_url, annotations, arguments);
}

// static
bool BraveCrashpadClient::StartJavaHandlerForClient(
    const std::string& class_name,
    const std::vector<std::string>* env,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments,
    int socket) {
  return CrashpadClient::StartJavaHandlerForClient(
      class_name, env, database, metrics_dir, brave_crash_url,
      annotations, arguments, socket);
}

// static
bool BraveCrashpadClient::StartHandlerWithLinkerAtCrash(
    const std::string& handler_trampoline,
    const std::string& handler_library,
    bool is_64_bit,
    const std::vector<std::string>* env,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments) {
  return CrashpadClient::StartHandlerWithLinkerAtCrash(
      handler_trampoline, handler_library, is_64_bit, env, database,
      metrics_dir, brave_crash_url, annotations, arguments);
}

// static
bool BraveCrashpadClient::StartHandlerWithLinkerForClient(
    const std::string& handler_trampoline,
    const std::string& handler_library,
    bool is_64_bit,
    const std::vector<std::string>* env,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments,
    int socket) {
  return CrashpadClient::StartHandlerWithLinkerForClient(
      handler_trampoline, handler_library, is_64_bit, env, database,
      metrics_dir, brave_crash_url, annotations, arguments, socket);
}

#endif  // defined(OS_ANDROID)

bool BraveCrashpadClient::StartHandlerAtCrash(
    const base::FilePath& handler,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments) {
  return crash_reporter::GetCrashpadClient().StartHandlerAtCrash(
      handler, database, metrics_dir,
      brave_crash_url, annotations, arguments);
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
    handler, database, metrics_dir, brave_crash_url,
    annotations, arguments, socket);
}

} // namespace crashpad
