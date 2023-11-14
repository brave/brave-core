/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <string>
#include <string_view>

#include "base/containers/contains.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"

#if defined(REDIRECT_CC_AS_GOMACC)
#include "brave/tools/redirect_cc/gomacc_buildflags.h"
#elif defined(REDIRECT_CC_AS_REWRAPPER)
#include "brave/tools/redirect_cc/rewrapper_buildflags.h"
#else  // defined(REDIRECT_CC_AS_GOMACC) || defined(REDIRECT_CC_AS_REWRAPPER)
#include "base/environment.h"
#endif  // defined(REDIRECT_CC_AS_GOMACC) || defined(REDIRECT_CC_AS_REWRAPPER)

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
  RedirectCC(int argc, const base::FilePath::CharType* const* argv)
      : argc_(argc), argv_(argv), environment_(base::Environment::Create()) {}

  base::FilePath::StringType GetCompilerExecutable(
      int* first_compiler_arg_idx) const {
    if (argc_ < 2) {
      return base::FilePath::StringType();
    }

#if defined(REDIRECT_CC_AS_GOMACC)
    *first_compiler_arg_idx = 1;
    return UTF8ToFilePathString(BUILDFLAG(REAL_GOMACC));
#elif defined(REDIRECT_CC_AS_REWRAPPER)
    *first_compiler_arg_idx = 1;
    return UTF8ToFilePathString(BUILDFLAG(REAL_REWRAPPER));
#else   // defined(REDIRECT_CC_AS_GOMACC) || defined(REDIRECT_CC_AS_REWRAPPER)
    std::string cc_wrapper;
    if (environment_->HasVar("CC_WRAPPER")) {
      CHECK(environment_->GetVar("CC_WRAPPER", &cc_wrapper));
    }

    if (!cc_wrapper.empty()) {
      *first_compiler_arg_idx = 1;
      return UTF8ToFilePathString(cc_wrapper);
    }

    *first_compiler_arg_idx = 2;
    return argv_[1];
#endif  // defined(REDIRECT_CC_AS_GOMACC) || defined(REDIRECT_CC_AS_REWRAPPER)
  }

  int Run() {
    // Get compiler executable. It can be a first arg to redirect_cc, a
    // REAL_GOMACC/REAL_REWRAPPER buildflag or a CC_WRAPPER env variable.
    int first_compiler_arg_idx = 0;
    const base::FilePath::StringType& compiler_executable =
        GetCompilerExecutable(&first_compiler_arg_idx);
    if (compiler_executable.empty() || first_compiler_arg_idx == 0) {
      LOG(ERROR) << "Compiler executable not found";
      return -1;
    }

    // Prepare argv to launch.
    std::vector<base::FilePath::StringType> launch_argv;
    launch_argv.reserve(argc_);
    base::EnvironmentMap launch_env;

    // Path to `src/brave/chromium_src`.
    base::FilePath::StringType brave_chromium_src_dir;
    // Path to `src/`.
    base::FilePath::StringType chromium_src_dir_with_slash;

#if defined(REDIRECT_CC_AS_REWRAPPER)
    int first_non_rewrapper_arg = -1;
    bool is_relocatable_compilation = false;
#endif  // defined(REDIRECT_CC_AS_REWRAPPER)

    // Find directories to work with first.
    for (int arg_idx = first_compiler_arg_idx; arg_idx < argc_; ++arg_idx) {
      base::FilePath::StringPieceType arg_piece = argv_[arg_idx];
#if defined(REDIRECT_CC_AS_REWRAPPER)
      if (first_non_rewrapper_arg == -1 && arg_piece[0] != '-') {
        first_non_rewrapper_arg = arg_idx;
        continue;
      }
      if (!is_relocatable_compilation &&
          (base::Contains(arg_piece,
                          FILE_PATH_LITERAL("ffile-compilation-dir")) ||
           base::Contains(arg_piece,
                          FILE_PATH_LITERAL("fdebug-compilation-dir")))) {
        is_relocatable_compilation = true;
        continue;
      }
#endif  // defined(REDIRECT_CC_AS_REWRAPPER)
      if (brave_chromium_src_dir.empty() &&
          chromium_src_dir_with_slash.empty() &&
          base::StartsWith(arg_piece, kIncludeFlag) &&
          base::EndsWith(arg_piece, kBraveChromiumSrc)) {
        arg_piece.remove_prefix(kIncludeFlag.size());
        brave_chromium_src_dir = base::FilePath::StringType(arg_piece);
        arg_piece.remove_suffix(kBraveChromiumSrc.size());
        chromium_src_dir_with_slash = base::FilePath::StringType(arg_piece);
      }
    }

    if (chromium_src_dir_with_slash.empty()) {
#if defined(REDIRECT_CC_AS_REWRAPPER)
      // We're called to execute a non-clang action. Just launch it as is.
      launch_argv.push_back(compiler_executable);
      for (int arg_idx = first_compiler_arg_idx; arg_idx < argc_; ++arg_idx) {
        launch_argv.emplace_back(argv_[arg_idx]);
      }
      return Launch(launch_argv, {});
#else   // defined(REDIRECT_CC_AS_REWRAPPER)
      LOG(ERROR) << "Can't find chromium src dir";
      return -1;
#endif  // defined(REDIRECT_CC_AS_REWRAPPER)
    }

#if defined(REDIRECT_CC_AS_REWRAPPER)
    constexpr std::string_view kCcacheExecutable(BUILDFLAG(CCACHE_EXECUTABLE));
    if (std::string use_remoteexec_with_ccache;
        !kCcacheExecutable.empty() &&
        environment_->GetVar("USE_REMOTEEXEC_WITH_CCACHE",
                             &use_remoteexec_with_ccache) &&
        use_remoteexec_with_ccache == "true") {
      launch_argv.emplace_back(UTF8ToFilePathString(kCcacheExecutable));
      if (is_relocatable_compilation) {
        launch_env[FILE_PATH_LITERAL("CCACHE_NOHASHDIR")] =
            FILE_PATH_LITERAL("1");
      }

      // Set CCACHE_PREFIX to the actual rewrapper.
      launch_env[FILE_PATH_LITERAL("CCACHE_PREFIX")] =
          base::MakeAbsoluteFilePath(base::FilePath(compiler_executable))
              .value();

      // Pass rewrapper parameters via environment variables, because
      // CCACHE_PREFIX can only point to an executable.
      for (int arg_idx = first_compiler_arg_idx;
           arg_idx < first_non_rewrapper_arg; ++arg_idx) {
        const base::FilePath::StringPieceType arg_piece = argv_[arg_idx];
        CHECK_EQ(arg_piece[0], '-') << arg_piece;
        const auto eq_sign_idx = arg_piece.find(FILE_PATH_LITERAL("="));
        CHECK_NE(eq_sign_idx, std::string::npos) << arg_piece;
        base::FilePath::StringType env_var_key(base::StrCat(
            {FILE_PATH_LITERAL("RBE_"), arg_piece.substr(1, eq_sign_idx - 1)}));
        base::FilePath::StringType env_var_value(
            arg_piece.substr(eq_sign_idx + 1, arg_piece.length()));
        launch_env.emplace(std::move(env_var_key), std::move(env_var_value));
      }
      first_compiler_arg_idx = first_non_rewrapper_arg;
    } else {
      launch_argv.push_back(compiler_executable);
    }
#else
    launch_argv.push_back(compiler_executable);
#endif  // defined(REDIRECT_CC_AS_REWRAPPER)

    bool compile_file_found = false;
    base::FilePath::StringType brave_path;
    bool has_show_includes_flag = false;

    for (int arg_idx = first_compiler_arg_idx; arg_idx < argc_; ++arg_idx) {
      const base::FilePath::StringPieceType arg_piece = argv_[arg_idx];
      if (!compile_file_found && base::Contains(kCompileFileFlags, arg_piece)) {
        compile_file_found = true;
        if (arg_idx + 1 >= argc_) {
          LOG(ERROR) << "No arg after compile flag " << arg_piece;
          return -1;
        }

        // Trim a file path to look for a similar file in
        // brave/chromium_src.
        base::FilePath::StringPieceType path_cc = argv_[arg_idx + 1];
        if (!path_cc.empty() && path_cc[0] == arg_piece[0]) {
          // That's not a file path, but another compiler parameter. This syntax
          // is used by asm compiler. We can safely ignore this, becaused we
          // don't redirect asm files.
          path_cc = base::FilePath::StringPieceType();
        } else if (base::StartsWith(path_cc, chromium_src_dir_with_slash)) {
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

    const auto exit_code = Launch(launch_argv, std::move(launch_env));

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

  int Launch(const std::vector<base::FilePath::StringType>& launch_argv,
             base::EnvironmentMap environment) {
#if BUILDFLAG(IS_WIN)
    const auto& to_launch = CreateCmdLine(launch_argv);
#else   // BUILDFLAG(IS_WIN)
    const auto& to_launch = launch_argv;
#endif  // BUILDFLAG(IS_WIN)

    base::LaunchOptions options;
    options.environment = std::move(environment);
    auto process = base::LaunchProcess(to_launch, options);
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

  const int argc_;
  const base::FilePath::CharType* const* argv_;
  std::unique_ptr<base::Environment> environment_;
};

#if BUILDFLAG(IS_WIN)
#define main wmain
#endif  // BUILDFLAG(IS_WIN)
int main(int argc, base::FilePath::CharType* argv[]) {
  RedirectCC redirect_cc(argc, argv);
  return redirect_cc.Run();
}
