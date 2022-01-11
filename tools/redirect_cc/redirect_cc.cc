/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <vector>

#include "brave/tools/redirect_cc/logging.h"
#include "brave/tools/redirect_cc/os_utils.h"
#include "brave/tools/redirect_cc/types.h"
#include "brave/tools/redirect_cc/utils.h"

const FilePathStringView kIncludeFlag = FILE_PATH_LITERAL("-I");
const FilePathStringView kBraveChromiumSrc =
    FILE_PATH_LITERAL("brave/chromium_src");
const FilePathStringView kGen = FILE_PATH_LITERAL("gen");
const FilePathStringView kCompileFileFlags[] = {
    FILE_PATH_LITERAL("-c"),
    FILE_PATH_LITERAL("/c"),
};
const FilePathStringView kCompileFilePathDelimeter = FILE_PATH_LITERAL("/");

class RedirectCC {
 public:
  RedirectCC(int argc, const FilePathChar* const* argv)
      : argc_(argc), argv_(argv) {}

  FilePathString GetCompilerExecutable(int* first_compiler_arg_idx) {
    if (argc_ < 2) {
      return FilePathString();
    }

    FilePathString executable;
    if (os_utils::GetEnvVar(FILE_PATH_LITERAL("CC_WRAPPER"), &executable)) {
      *first_compiler_arg_idx = 1;
    } else {
      executable = argv_[1];
      *first_compiler_arg_idx = 2;
    }

    return executable;
  }

  int Run() {
    // Get compiler executable. It can be a first arg to redirect_cc or
    // from CC_WRAPPER env variable.
    int first_compiler_arg_idx = 0;
    const FilePathString& compiler_executable =
        GetCompilerExecutable(&first_compiler_arg_idx);
    if (compiler_executable.empty() || first_compiler_arg_idx == 0) {
      LOG("Compiler executable not found");
      return -1;
    }

    // Path to `src/brave/chromium_src`.
    FilePathString brave_chromium_src_dir;
    // Path to `src/`.
    FilePathString chromium_src_dir_with_slash;

    // Find directories to work with first.
    for (int arg_idx = first_compiler_arg_idx; arg_idx < argc_; ++arg_idx) {
      FilePathStringView arg_piece = argv_[arg_idx];
      if (utils::StartsWith(arg_piece, kIncludeFlag) &&
          utils::EndsWith(arg_piece, kBraveChromiumSrc)) {
        arg_piece.remove_prefix(kIncludeFlag.size());
        brave_chromium_src_dir = FilePathString(arg_piece);
        arg_piece.remove_suffix(kBraveChromiumSrc.size());
        chromium_src_dir_with_slash = FilePathString(arg_piece);
        break;
      }
    }
    if (chromium_src_dir_with_slash.empty()) {
      LOG("Can't find Chromium src/ dir");
      return -1;
    }

    // Prepare argv to launch.
    std::vector<FilePathString> launch_argv;
    launch_argv.reserve(argc_);
    launch_argv.push_back(compiler_executable);
    bool compile_file_found = false;

    for (int arg_idx = first_compiler_arg_idx; arg_idx < argc_; ++arg_idx) {
      const FilePathStringView arg_piece = argv_[arg_idx];
      if (!compile_file_found &&
          std::find(std::begin(kCompileFileFlags), std::end(kCompileFileFlags),
                    arg_piece) != std::end(kCompileFileFlags)) {
        compile_file_found = true;
        if (arg_idx + 1 >= argc_) {
          LOG("No arg after compile flag: " << arg_piece);
          return -1;
        }

        // Trim a file path to look for a similar file in
        // brave/chromium_src.
        FilePathStringView path_cc = argv_[arg_idx + 1];
        // Most common case - a file is located directly in src/...
        if (utils::StartsWith(path_cc, chromium_src_dir_with_slash)) {
          path_cc.remove_prefix(chromium_src_dir_with_slash.size());
        } else {
          // Less common case - a file is generated and located in the `out`
          // directory.
          auto path_cc_parts =
              utils::SplitString(path_cc, kCompileFilePathDelimeter);
          if (path_cc_parts.size() > 0 && path_cc_parts[0] == kGen) {
            // Generated file override, for ex.: gen/base/buildflags.h.
            // Remove gen/ prefix.
            path_cc.remove_prefix(path_cc_parts[0].size() + 1);
          } else if (path_cc_parts.size() > 1 && path_cc_parts[1] == kGen) {
            // Generated file override inside of a custom toolchain, for ex:
            // android_clang_arm64/gen/base/buildflags.h.
            // Remove android_clang_arm64/gen/ prefix.
            path_cc.remove_prefix(path_cc_parts[0].size() + 1);
            path_cc.remove_prefix(path_cc_parts[1].size() + 1);
          }
        }

        FilePathString brave_cc_path = brave_chromium_src_dir;
        brave_cc_path += kCompileFilePathDelimeter;
        brave_cc_path += path_cc;
        VLOG("Looking for override at " << brave_cc_path);
        if (os_utils::PathExists(brave_cc_path)) {
          launch_argv.push_back(FilePathString(arg_piece));
          launch_argv.push_back(std::move(brave_cc_path));
          ++arg_idx;
          continue;
        }
      }
      launch_argv.push_back(FilePathString(arg_piece));
    }

    return os_utils::LaunchProcessAndWaitForExitCode(launch_argv);
  }

 private:
  const int argc_;
  const FilePathChar* const* argv_;
};

#if defined(_WIN32)
#define main wmain
#endif
int main(int argc, FilePathChar* argv[]) {
  RedirectCC redirect_cc(argc, argv);
  return redirect_cc.Run();
}
