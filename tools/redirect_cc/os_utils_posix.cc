
/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "brave/tools/redirect_cc/logging.h"
#include "brave/tools/redirect_cc/os_utils.h"

extern char** environ;

namespace os_utils {

namespace {

#define HANDLE_EINTR(x)                                     \
  ({                                                        \
    decltype(x) eintr_wrapper_result;                       \
    do {                                                    \
      eintr_wrapper_result = (x);                           \
    } while (eintr_wrapper_result == -1 && errno == EINTR); \
    eintr_wrapper_result;                                   \
  })

class PosixSpawnAttr {
 public:
  PosixSpawnAttr() { posix_spawnattr_init(&attr_); }

  ~PosixSpawnAttr() { posix_spawnattr_destroy(&attr_); }

  posix_spawnattr_t* get() { return &attr_; }

 private:
  posix_spawnattr_t attr_;
};

class PosixSpawnFileActions {
 public:
  PosixSpawnFileActions() { posix_spawn_file_actions_init(&file_actions_); }

  PosixSpawnFileActions(const PosixSpawnFileActions&) = delete;
  PosixSpawnFileActions& operator=(const PosixSpawnFileActions&) = delete;

  ~PosixSpawnFileActions() { posix_spawn_file_actions_destroy(&file_actions_); }

  void Open(int filedes, const char* path, int mode) {
    posix_spawn_file_actions_addopen(&file_actions_, filedes, path, mode, 0);
  }

  void Dup2(int filedes, int newfiledes) {
    posix_spawn_file_actions_adddup2(&file_actions_, filedes, newfiledes);
  }

  const posix_spawn_file_actions_t* get() const { return &file_actions_; }

 private:
  posix_spawn_file_actions_t file_actions_;
};

}  // namespace

int LaunchProcessAndWaitForExitCode(const std::vector<FilePathString>& argv) {
  PosixSpawnAttr attr;
  PosixSpawnFileActions file_actions;

  file_actions.Open(STDIN_FILENO, "/dev/null", O_RDONLY);
  file_actions.Dup2(STDOUT_FILENO, STDOUT_FILENO);
  file_actions.Dup2(STDERR_FILENO, STDERR_FILENO);

  std::vector<char*> argv_cstr;
  argv_cstr.reserve(argv.size() + 1);
  for (const auto& arg : argv)
    argv_cstr.push_back(const_cast<char*>(arg.data()));
  argv_cstr.push_back(nullptr);

  const char* executable_path = argv_cstr[0];

  int rv;
  pid_t pid;
  rv = posix_spawnp(&pid, executable_path, file_actions.get(), attr.get(),
                    &argv_cstr[0], environ);

  if (rv != 0) {
    LOG("posix_spawnp error: " << rv);
    return rv;
  }

  HANDLE_EINTR(waitpid(pid, &rv, 0));
  return rv;
}

bool PathExists(FilePathStringView path) {
  return access(path.data(), F_OK) == 0;
}

bool GetEnvVar(FilePathStringView variable_name, FilePathString* result) {
  const char* env_value = getenv(variable_name.data());
  if (!env_value)
    return false;
  if (result)
    *result = env_value;
  return true;
}

}  // namespace os_utils
