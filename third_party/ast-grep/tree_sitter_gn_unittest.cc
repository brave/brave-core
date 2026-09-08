/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/strings/string_split.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kGnSource[] = R"(shared_library("gn") {
  output_name = "gn"
  sources = [ "parser.c" ]
})";

constexpr char kPatternRule[] =
    "{id: pattern, language: gn, rule: {pattern: output_name = $VALUE}}";
constexpr char kNodeKindRule[] =
    "{id: node_kind, language: gn, rule: {kind: assignment_statement}}";

class TreeSitterGnTest : public testing::Test {
 protected:
  void SetUp() override {
    base::FilePath source_root;
    ASSERT_TRUE(
        base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &source_root));

    ast_grep_ = source_root.Append(base::FilePath::FromASCII(AST_GREP_BIN));
    sgconfig_ =
        source_root.Append(base::FilePath::FromASCII(AST_GREP_SGCONFIG));
    ASSERT_TRUE(base::PathExists(ast_grep_)) << ast_grep_;
    ASSERT_TRUE(base::PathExists(sgconfig_)) << sgconfig_;

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    // A `.gn` extension, so the scan also exercises the extension mapping
    // `sgconfig.yml` registers for the custom language.
    gn_file_ = temp_dir_.GetPath().AppendASCII("test.gn");
    ASSERT_TRUE(base::WriteFile(gn_file_, kGnSource));
  }

  // Scans `gn_file_` with `rule`, returning one dict per reported match.
  std::vector<base::DictValue> Scan(std::string_view rule) {
    base::CommandLine command(ast_grep_);
    command.AppendArg("scan");
    command.AppendArg("--inline-rules");
    command.AppendArg(std::string(rule));
    command.AppendArg("--config");
    command.AppendArgPath(sgconfig_);
    command.AppendArg("--json=stream");
    command.AppendArgPath(gn_file_);

    std::string output;
    if (!base::GetAppOutputAndError(command, &output)) {
      ADD_FAILURE() << command.GetCommandLineString() << "\n" << output;
      return {};
    }

    // A successful run should only produce a JSON value of the match.
    std::vector<base::DictValue> matches;
    for (std::string_view line : base::SplitStringPiece(
             output, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
      std::optional<base::DictValue> match =
          base::JSONReader::ReadDict(line, base::JSON_PARSE_RFC);
      if (!match) {
        ADD_FAILURE() << "not a JSON object: " << line;
        continue;
      }
      matches.push_back(std::move(*match));
    }
    return matches;
  }

  // the path to the ast-grep binary
  base::FilePath ast_grep_;

  // the path to the sg config file used to load the tree-sitter-gn grammar
  base::FilePath sgconfig_;

  // a test gn file
  base::FilePath gn_file_;

  // the temp dir used during the test.
  base::ScopedTempDir temp_dir_;
};

TEST_F(TreeSitterGnTest, LoadsGnGrammar) {
  const std::vector<base::DictValue> matches = Scan(kPatternRule);
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_THAT(matches.front(), base::test::IsSupersetOfValue(R"json({
        "language": "gn",
        "text": "output_name = \"gn\"",
        "metaVariables": {"single": {"VALUE": {"text": "\"gn\""}}}
      })json"));
}

TEST_F(TreeSitterGnTest, ExposesGnNodeKinds) {
  const std::vector<base::DictValue> matches = Scan(kNodeKindRule);
  EXPECT_THAT(matches,
              ::testing::ElementsAre(
                  base::test::IsSupersetOfValue(
                      R"json({"text": "output_name = \"gn\""})json"),
                  base::test::IsSupersetOfValue(
                      R"json({"text": "sources = [ \"parser.c\" ]"})json")));
}

}  // namespace
