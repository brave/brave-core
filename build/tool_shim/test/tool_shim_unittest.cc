/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/scoped_environment_variable_override.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "brave/build/tool_shim/tool_shim_config.h"
#include "brave/build/tool_shim/utils.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

#if BUILDFLAG(IS_WIN)
constexpr char kExeSuffix[] = ".exe";
#else
constexpr char kExeSuffix[] = "";
#endif

// Matches tool_shim.cc.
constexpr int kExitInvalidConfig = 78;
constexpr int kExitCannotExecute = 126;

base::FilePath OutDir() {
  return base::PathService::CheckedGet(base::DIR_EXE);
}

base::FilePath ExeInOutDir(std::string_view name) {
  return OutDir().AppendASCII(base::StrCat({name, kExeSuffix}));
}

base::FilePath HelperExe() {
  return ExeInOutDir("tool_shim_test_helper");
}

base::FilePath GenDir() {
  return OutDir().AppendASCII("gen");
}

// Resolves symlinks (e.g. macOS /var -> /private/var) before comparing.
bool SamePath(const base::FilePath& a, const base::FilePath& b) {
  return base::MakeAbsoluteFilePath(a) == base::MakeAbsoluteFilePath(b);
}

struct ShimRunResult {
  int exit_code = -1;
  base::DictValue report;
};

}  // namespace

class ToolShimTest : public testing::Test {
 protected:
  void SetUp() override {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

  base::FilePath WriteConfig(std::string_view name, std::string_view contents) {
    const base::FilePath path =
        temp_dir_.GetPath().AppendASCII(base::StrCat({name, ".json"}));
    EXPECT_TRUE(base::WriteFile(path, contents));
    return path;
  }

  base::FilePath WriteConfig(std::string_view name,
                             const base::DictValue& contents) {
    std::optional<std::string> json = base::WriteJson(contents);
    EXPECT_TRUE(json.has_value());
    return WriteConfig(name, *json);
  }

  base::FilePath UniqueReportPath() {
    return temp_dir_.GetPath().AppendASCII(base::StrCat(
        {"report_", base::NumberToString(report_counter_++), ".json"}));
  }

  std::string UniqueShimName(std::string_view prefix) {
    return base::StrCat({prefix, "_", base::NumberToString(report_counter_++)});
  }

  // Copies test_tool_shim into temp_dir_ as |name| (plus platform exe suffix).
  // Does not copy any sidecar config; tests write the config they need.
  base::FilePath InstallShim(std::string_view name) {
    const base::FilePath source = ExeInOutDir("test_tool_shim");
    const base::FilePath dest =
        temp_dir_.GetPath().AppendASCII(base::StrCat({name, kExeSuffix}));
    EXPECT_TRUE(base::CopyFile(source, dest));
#if BUILDFLAG(IS_POSIX)
    int mode = 0;
    EXPECT_TRUE(base::GetPosixFilePermissions(source, &mode));
    EXPECT_TRUE(base::SetPosixFilePermissions(dest, mode));
#endif
    return dest;
  }

  ShimRunResult RunShim(const base::FilePath& shim,
                        const std::vector<std::string>& args = {}) {
    const base::FilePath report_path = UniqueReportPath();
    base::ScopedEnvironmentVariableOverride report_env(
        "TOOL_SHIM_TEST_REPORT", report_path.AsUTF8Unsafe());

    base::CommandLine cmd(shim);
    for (const auto& arg : args) {
      cmd.AppendArg(arg);
    }

    base::LaunchOptions options;
#if BUILDFLAG(IS_WIN)
    options.start_hidden = true;
#endif
    base::Process process = base::LaunchProcess(cmd, options);
    EXPECT_TRUE(process.IsValid());

    ShimRunResult result;
    EXPECT_TRUE(process.WaitForExitWithTimeout(TestTimeouts::action_timeout(),
                                               &result.exit_code));

    std::string report_text;
    if (base::PathExists(report_path)) {
      EXPECT_TRUE(base::ReadFileToString(report_path, &report_text));
      auto parsed = base::JSONReader::ReadAndReturnValueWithError(
          report_text, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
      EXPECT_TRUE(parsed.has_value()) << parsed.error().ToString() << "\n"
                                      << report_text;
      if (parsed.has_value() && parsed->is_dict()) {
        result.report = std::move(parsed->GetDict());
      }
    }
    return result;
  }

  base::ScopedTempDir temp_dir_;
  int report_counter_ = 0;
};

TEST_F(ToolShimTest, ResolveAgainstRelativePath) {
#if BUILDFLAG(IS_WIN)
  const base::FilePath base_dir(FILE_PATH_LITERAL("C:\\out\\Default"));
#else
  const base::FilePath base_dir(FILE_PATH_LITERAL("/out/Default"));
#endif
  EXPECT_EQ(ResolveAgainst(base_dir, FILE_PATH_LITERAL("helper")),
            base_dir.Append(FILE_PATH_LITERAL("helper")));
  EXPECT_EQ(ResolveAgainst(base_dir, FILE_PATH_LITERAL("sub/helper")),
            base_dir.Append(FILE_PATH_LITERAL("sub/helper")));
}

TEST_F(ToolShimTest, ResolveAgainstAbsolutePath) {
#if BUILDFLAG(IS_WIN)
  const base::FilePath::StringType absolute = FILE_PATH_LITERAL("C:\\tools\\x");
#else
  const base::FilePath::StringType absolute = FILE_PATH_LITERAL("/tools/x");
#endif
  const base::FilePath base_dir(FILE_PATH_LITERAL("/out/Default"));
  EXPECT_EQ(ResolveAgainst(base_dir, absolute), base::FilePath(absolute));
}

TEST_F(ToolShimTest, ParseValidFullConfig) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  const base::FilePath path = WriteConfig("full", R"({
    "executable": "helper",
    "args": ["a", "b"],
    "env": {"FOO": "bar"},
    "cwd": "workdir"
  })");

  auto config = ParseToolShimConfig(path);
  ASSERT_TRUE(config.has_value()) << config.error();
  EXPECT_EQ(config->executable, ToNativeString("helper"));
  ASSERT_EQ(config->args.size(), 2u);
  EXPECT_EQ(config->args[0], ToNativeString("a"));
  EXPECT_EQ(config->args[1], ToNativeString("b"));
  ASSERT_EQ(config->env.size(), 1u);
  EXPECT_EQ(config->env.at(ToNativeString("FOO")), ToNativeString("bar"));
  ASSERT_TRUE(config->cwd.has_value());
  EXPECT_EQ(*config->cwd, ToNativeString("workdir"));
}

TEST_F(ToolShimTest, ParseValidMinimalConfig) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  const base::FilePath path = WriteConfig("min", R"({"executable": "helper"})");

  auto config = ParseToolShimConfig(path);
  ASSERT_TRUE(config.has_value()) << config.error();
  EXPECT_EQ(config->executable, ToNativeString("helper"));
  EXPECT_TRUE(config->args.empty());
  EXPECT_TRUE(config->env.empty());
  EXPECT_FALSE(config->cwd.has_value());
}

TEST_F(ToolShimTest, ParseMissingConfig) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  auto config = ParseToolShimConfig(
      temp_dir_.GetPath().AppendASCII("does_not_exist.json"));
  EXPECT_FALSE(config.has_value());
  EXPECT_NE(config.error().find("Failed to read config"), std::string::npos);
}

class ToolShimParseInvalidConfigTest
    : public ToolShimTest,
      public testing::WithParamInterface<std::string_view> {};

TEST_P(ToolShimParseInvalidConfigTest, RejectsInvalidConfig) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  const base::FilePath path = WriteConfig("bad", GetParam());
  auto config = ParseToolShimConfig(path);
  EXPECT_FALSE(config.has_value());
}

INSTANTIATE_TEST_SUITE_P(
    ,
    ToolShimParseInvalidConfigTest,
    testing::Values(R"([])",
                    R"({})",
                    R"({"executable": 1})",
                    R"({"executable": "x", "args": "nope"})",
                    R"({"executable": "x", "args": [1]})",
                    R"({"executable": "x", "env": []})",
                    R"({"executable": "x", "env": {"K": 1}})",
                    R"({"executable": "x", "cwd": 1})",
                    R"({not json)"));

TEST_F(ToolShimTest, ConfiguredShimArgsEnvCwdAndExitCode) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  // Run the GN-built test_tool_shim with its baked sidecar config.
  ShimRunResult result =
      RunShim(ExeInOutDir("test_tool_shim"), {"runtime", "arg with space"});
  ASSERT_EQ(result.exit_code, 7);

  const base::ListValue* argv = result.report.FindList("argv");
  ASSERT_TRUE(argv);
  ASSERT_EQ(argv->size(), 6u);
  EXPECT_EQ((*argv)[0].GetString(), "baked");
  EXPECT_EQ((*argv)[1].GetString(), "with space");
  EXPECT_EQ((*argv)[2].GetString(), "");
  EXPECT_EQ((*argv)[3].GetString(), "quote\"d");
  EXPECT_EQ((*argv)[4].GetString(), "runtime");
  EXPECT_EQ((*argv)[5].GetString(), "arg with space");

  const std::string* cwd = result.report.FindString("cwd");
  ASSERT_TRUE(cwd);
  EXPECT_TRUE(SamePath(base::FilePath::FromUTF8Unsafe(*cwd), GenDir()));

  const base::DictValue* env = result.report.FindDict("env");
  ASSERT_TRUE(env);
  const std::string* test_var = env->FindString("TOOL_SHIM_TEST_VAR");
  ASSERT_TRUE(test_var);
  EXPECT_EQ(*test_var, "from-config");
}

TEST_F(ToolShimTest, MinimalShimForwardsArgsAndDefaultsCwd) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  const std::string name = UniqueShimName("minimal");
  const base::FilePath shim = InstallShim(name);
  WriteConfig(name,
              base::DictValue().Set("executable", HelperExe().AsUTF8Unsafe()));

  ShimRunResult result = RunShim(shim, {"alpha", "beta gamma"});
  ASSERT_EQ(result.exit_code, 0);

  const base::ListValue* argv = result.report.FindList("argv");
  ASSERT_TRUE(argv);
  ASSERT_EQ(argv->size(), 2u);
  EXPECT_EQ((*argv)[0].GetString(), "alpha");
  EXPECT_EQ((*argv)[1].GetString(), "beta gamma");

  // With no cwd in the config, the shim defaults to its own directory.
  const std::string* cwd = result.report.FindString("cwd");
  ASSERT_TRUE(cwd);
  EXPECT_TRUE(SamePath(base::FilePath::FromUTF8Unsafe(*cwd), shim.DirName()));
}

TEST_F(ToolShimTest, MissingConfigReturns78) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  const base::FilePath shim = InstallShim(UniqueShimName("missing_config"));
  ShimRunResult result = RunShim(shim);
  EXPECT_EQ(result.exit_code, kExitInvalidConfig);
}

TEST_F(ToolShimTest, MalformedConfigReturns78) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  const std::string name = UniqueShimName("bad_config");
  const base::FilePath shim = InstallShim(name);
  WriteConfig(name, "{not-json");

  ShimRunResult result = RunShim(shim);
  EXPECT_EQ(result.exit_code, kExitInvalidConfig);
}

TEST_F(ToolShimTest, MissingExecutableReturns126) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  const std::string name = UniqueShimName("missing_exe");
  const base::FilePath shim = InstallShim(name);
  WriteConfig(name, R"({"executable": "does_not_exist_tool_shim_target"})");

  ShimRunResult result = RunShim(shim);
  EXPECT_EQ(result.exit_code, kExitCannotExecute);
}
