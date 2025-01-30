/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <string>
#include <string_view>

#include "base/containers/contains.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"

#if defined(REDIRECT_CC_AS_REWRAPPER)
#include "base/base_paths.h"
#include "base/path_service.h"
#include "brave/tools/redirect_cc/rewrapper_buildflags.h"  // nogncheck
#else  // defined(REDIRECT_CC_AS_REWRAPPER)
#include "base/environment.h"
#endif  // defined(REDIRECT_CC_AS_REWRAPPER)

const base::FilePath::StringPieceType kIncludeFlag = FILE_PATH_LITERAL("-I");
const base::FilePath::StringPieceType kBraveChromiumSrc =
    FILE_PATH_LITERAL("brave/chromium_src");
const base::FilePath::StringPieceType kGen = FILE_PATH_LITERAL("gen");
const base::FilePath::StringPieceType kCompileFileFlags[] = {
    FILE_PATH_LITERAL("-c"),
    FILE_PATH_LITERAL("/c"),
};
const base::FilePath::StringPieceType kShowIncludesFlag =
    FILE_PATH_LITERAL("/showIncludes");

class RedirectCC {
 public:
  explicit RedirectCC(base::span<const base::FilePath::CharType* const> args)
      : args_(args) {}

  base::FilePath::StringType GetCompilerExecutable(
      size_t* first_compiler_arg_idx) const {
    if (args_.size() < 2) {
      return base::FilePath::StringType();
    }

#if defined(REDIRECT_CC_AS_REWRAPPER)
    *first_compiler_arg_idx = 1;
    // Assume DIR_EXE is always src/out/redirect_cc.
    return base::PathService::CheckedGet(base::DIR_EXE)
        .Append(UTF8ToFilePathString(BUILDFLAG(REAL_REWRAPPER)))
        .value();
#else   // defined(REDIRECT_CC_AS_REWRAPPER)
    std::string cc_wrapper;
    std::unique_ptr<base::Environment> env(base::Environment::Create());
    if (env->HasVar("CC_WRAPPER")) {
      CHECK(env->GetVar("CC_WRAPPER", &cc_wrapper));
    }

    if (!cc_wrapper.empty()) {
      *first_compiler_arg_idx = 1;
      return UTF8ToFilePathString(cc_wrapper);
    }

    *first_compiler_arg_idx = 2;
    return args_[1];
#endif  // defined(REDIRECT_CC_AS_REWRAPPER)
  }

  int Run() {
    // Get compiler executable. It can be a first arg to redirect_cc, a
    // REAL_REWRAPPER buildflag or a CC_WRAPPER env variable.
    size_t first_compiler_arg_idx = 0;
    const base::FilePath::StringType& compiler_executable =
        GetCompilerExecutable(&first_compiler_arg_idx);
    if (compiler_executable.empty() || first_compiler_arg_idx == 0) {
      LOG(ERROR) << "Compiler executable not found";
      return -1;
    }

    // Prepare argv to launch.
    std::vector<base::FilePath::StringType> launch_argv;
    launch_argv.reserve(args_.size());
    launch_argv.push_back(compiler_executable);

    // Path to `src/brave/chromium_src`.
    base::FilePath::StringType brave_chromium_src_dir;
    // Path to `src/`.
    base::FilePath::StringType chromium_src_dir_with_slash;

    // Find directories to work with first.
    for (const auto* arg : args_.subspan(first_compiler_arg_idx)) {
      base::FilePath::StringPieceType arg_piece(arg);
      if (arg_piece.starts_with(kIncludeFlag) &&
          arg_piece.ends_with(kBraveChromiumSrc)) {
        arg_piece.remove_prefix(kIncludeFlag.size());
        brave_chromium_src_dir = base::FilePath::StringType(arg_piece);
        arg_piece.remove_suffix(kBraveChromiumSrc.size());
        chromium_src_dir_with_slash = base::FilePath::StringType(arg_piece);
        break;
      }
    }

    if (chromium_src_dir_with_slash.empty()) {
#if defined(REDIRECT_CC_AS_REWRAPPER)
      // We're called to execute a non-clang action. Just launch it as is.
      for (const auto* arg : args_.subspan(first_compiler_arg_idx)) {
        launch_argv.emplace_back(arg);
      }
      return Launch(launch_argv);
#else   // defined(REDIRECT_CC_AS_REWRAPPER)
      LOG(ERROR) << "Can't find chromium src dir";
      return -1;
#endif  // defined(REDIRECT_CC_AS_REWRAPPER)
    }

    bool compile_file_found = false;
    base::FilePath::StringType brave_path;
    bool has_show_includes_flag = false;

    for (size_t arg_idx = first_compiler_arg_idx; arg_idx < args_.size();
         ++arg_idx) {
      const base::FilePath::StringPieceType arg_piece = args_[arg_idx];
      if (!compile_file_found && base::Contains(kCompileFileFlags, arg_piece)) {
        compile_file_found = true;
        if (arg_idx + 1 >= args_.size()) {
          LOG(ERROR) << "No arg after compile flag " << arg_piece;
          return -1;
        }

        // Trim a file path to look for a similar file in
        // brave/chromium_src.
        base::FilePath::StringPieceType path_cc = args_[arg_idx + 1];
        if (!path_cc.empty() && path_cc[0] == arg_piece[0]) {
          // That's not a file path, but another compiler parameter. This syntax
          // is used by asm compiler. We can safely ignore this, becaused we
          // don't redirect asm files.
          path_cc = base::FilePath::StringPieceType();
        } else if (path_cc.starts_with(chromium_src_dir_with_slash)) {
          // Most common case - a file is located directly in src/...
          path_cc.remove_prefix(chromium_src_dir_with_slash.size());
        } else {
          // Less common case - a file is generated and located in the `out`
          // directory.
          auto path_cc_parts = base::SplitStringPiece(
              path_cc, base::FilePath::kSeparators, base::KEEP_WHITESPACE,
              base::SPLIT_WANT_ALL);
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
          } else {
            LOG(WARNING) << "unhandled path: " << path_cc;
            path_cc = base::FilePath::StringPieceType();
          }
        }

        if (!path_cc.empty()) {
          brave_path = base::StrCat(
              {brave_chromium_src_dir, FILE_PATH_LITERAL("/"), path_cc});
          if (base::PathExists(base::FilePath(brave_path))) {
            launch_argv.emplace_back(arg_piece);
            launch_argv.push_back(brave_path);
            ++arg_idx;
            continue;
          } else {
            brave_path.clear();
          }
        }
      } else {
        has_show_includes_flag |= arg_piece.starts_with(kShowIncludesFlag);
      }
      launch_argv.emplace_back(arg_piece);
    }

    const auto exit_code = Launch(launch_argv);

    // To check the redirected file timestamp, it should be marked as dependency
    // for ninja. Linux/MacOS gcc deps format includes this file properly.
    // Windows msvc deps format does not include it, so we do it manually here.
    // This is a specially crafted string that ninja will look for to create
    // deps.
    if (exit_code == 0 && !brave_path.empty() && has_show_includes_flag) {
#if BUILDFLAG(IS_WIN)
      std::wcerr << L"Note: including file: " << brave_path << L"\n";
#else   // BUILDFLAG(IS_WIN)
      std::cerr << "Note: including file: " << brave_path << "\n";
#endif  // BUILDFLAG(IS_WIN)
    }

    return exit_code;
  }

  int Launch(const std::vector<base::FilePath::StringType>& launch_argv) {
#if BUILDFLAG(IS_WIN)
    const auto& to_launch = CreateCmdLine(launch_argv);
#else   // BUILDFLAG(IS_WIN)
    const auto& to_launch = launch_argv;
#endif  // BUILDFLAG(IS_WIN)

    auto process = base::LaunchProcess(to_launch, base::LaunchOptions());
    int exit_code = -1;
    if (!process.WaitForExit(&exit_code)) {
      LOG(ERROR) << "Failed to WaitForExit";
      return -1;
    }
    return exit_code;
  }

 private:
  static base::FilePath::StringType UTF8ToFilePathString(
      std::string_view utf8) {
#if BUILDFLAG(IS_WIN)
    return base::UTF8ToWide(utf8);
#else   // BUILDFLAG(IS_WIN)
    return base::FilePath::StringType(utf8);
#endif  // BUILDFLAG(IS_WIN)
  }

#if BUILDFLAG(IS_WIN)
  // Quote a string as necessary for CommandLineToArgvW compatibility *on
  // Windows*.
  static bool QuoteForCommandLineToArgvW(const base::FilePath::StringType& arg,
                                         base::FilePath::StringType* out) {
    DCHECK(out);
    // We follow the quoting rules of CommandLineToArgvW.
    // http://msdn.microsoft.com/en-us/library/17w5ykft.aspx
    base::FilePath::StringType quotable_chars(L" \\\"");
    if (arg.find_first_of(quotable_chars) == base::FilePath::StringType::npos) {
      // No quoting necessary.
      return false;
    }

    out->clear();
    out->push_back('"');
    for (size_t i = 0; i < arg.size(); ++i) {
      if (arg[i] == '\\') {
        // Find the extent of this run of backslashes.
        size_t start = i, end = start + 1;
        for (; end < arg.size() && arg[end] == '\\'; ++end) {
        }
        size_t backslash_count = end - start;

        // Backslashes are escapes only if the run is followed by a double
        // quote. Since we also will end the string with a double quote, we
        // escape for either a double quote or the end of the string.
        if (end == arg.size() || arg[end] == '"') {
          // To quote, we need to output 2x as many backslashes.
          backslash_count *= 2;
        }
        for (size_t j = 0; j < backslash_count; ++j) {
          out->push_back('\\');
        }

        // Advance i to one before the end to balance i++ in loop.
        i = end - 1;
      } else if (arg[i] == '"') {
        out->push_back('\\');
        out->push_back('"');
      } else {
        out->push_back(arg[i]);
      }
    }
    out->push_back('"');
    return true;
  }

  static base::FilePath::StringType CreateCmdLine(
      const std::vector<base::FilePath::StringType>& argv) {
    base::FilePath::StringType cmd_line;
    base::FilePath::StringType quotted_arg;
    quotted_arg.reserve(1024);
    for (const auto& arg : argv) {
      const auto& space_separator =
          cmd_line.empty()
              ? base::FilePath::StringPieceType()
              : base::FilePath::StringPieceType(FILE_PATH_LITERAL(" "));
      const auto& arg_to_append =
          QuoteForCommandLineToArgvW(arg, &quotted_arg) ? quotted_arg : arg;
      base::StrAppend(&cmd_line, {space_separator, arg_to_append});
    }
    return cmd_line;
  }
#endif  // BUILDFLAG(IS_WIN)

  const base::span<const base::FilePath::CharType* const> args_;
};

#if BUILDFLAG(IS_WIN)
#define main wmain
#endif  // BUILDFLAG(IS_WIN)
int main(int argc, base::FilePath::CharType* argv[]) {
  // SAFETY: These are runtime-provided pointers that are guaranteed to be
  // valid.
  RedirectCC redirect_cc(
      UNSAFE_BUFFERS(base::span(argv, static_cast<size_t>(argc))));
  return redirect_cc.Run();
}
