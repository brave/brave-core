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
#include "base/values.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// GN source covering both shapes the grammar has to get right: a call with a
// block, and the assignments inside it.
constexpr char kGnSource[] = R"(shared_library("gn") {
  output_name = "gn"
  sources = [ "parser.c" ]
})";

// Rules are single-line YAML flow mappings: `--inline-rules` takes a whole
// rule document, and one line keeps newlines out of a command-line argument.
constexpr char kPatternRule[] =
    "{id: pattern, language: gn, rule: {pattern: output_name = $VALUE}}";
constexpr char kNodeKindRule[] =
    "{id: node_kind, language: gn, rule: {kind: assignment_statement}}";

// Verifies ast-grep can load the `gn` grammar that
// `third_party/ast-grep/build_tree_sitter_gn.py` builds and installs, which is
// the only thing teaching ast-grep to parse GN at all.
//
// The provisioned `ast-grep-<os>/` tree is assumed present: the
// `download_ast_grep` gclient hook installs one, and the build script
// refreshes the grammar in place.
class TreeSitterGnTest : public testing::Test {
 protected:
  void SetUp() override {
    base::FilePath source_root;
    ASSERT_TRUE(
        base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &source_root));
    // Both paths come from the GN target, so this test and the build script
    // cannot disagree on where the install tree lives.
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
  std::vector<base::Value::Dict> Scan(std::string_view rule) {
    base::CommandLine command(ast_grep_);
    // Arguments only, never `AppendSwitch*`: `base::CommandLine` groups
    // switches ahead of arguments, which would hoist `--config` in front of
    // the `scan` subcommand it belongs to.
    command.AppendArg("scan");
    command.AppendArg("--inline-rules");
    command.AppendArg(std::string(rule));
    command.AppendArg("--config");
    command.AppendArgPath(sgconfig_);
    command.AppendArg("--json=stream");
    command.AppendArgPath(gn_file_);

    std::string output;
    int exit_code = -1;
    EXPECT_TRUE(base::GetAppOutputWithExitCode(command, &output, &exit_code));
    // A grammar ast-grep cannot dlopen, or whose ABI it rejects, fails here
    // rather than reporting zero matches.
    EXPECT_EQ(exit_code, 0) << command.GetCommandLineString();

    std::vector<base::Value::Dict> matches;
    for (std::string_view line : base::SplitStringPiece(
             output, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
      std::optional<base::Value> match = base::JSONReader::Read(line);
      if (!match || !match->is_dict()) {
        ADD_FAILURE() << "not a JSON object: " << line;
        continue;
      }
      matches.push_back(std::move(*match).TakeDict());
    }
    return matches;
  }

  base::FilePath ast_grep_;
  base::FilePath sgconfig_;
  base::FilePath gn_file_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(TreeSitterGnTest, LoadsGnGrammar) {
  const std::vector<base::Value::Dict> matches = Scan(kPatternRule);
  ASSERT_EQ(matches.size(), 1u);
  const base::Value::Dict& match = matches.front();

  // Reported as `gn`, so ast-grep resolved the custom language rather than
  // falling back to a built-in one.
  const std::string* language = match.FindString("language");
  ASSERT_TRUE(language);
  EXPECT_EQ(*language, "gn");

  const std::string* text = match.FindString("text");
  ASSERT_TRUE(text);
  EXPECT_EQ(*text, "output_name = \"gn\"");

  // The metavariable binding proves the match came from a parse tree, not a
  // textual search.
  const std::string* value =
      match.FindStringByDottedPath("metaVariables.single.VALUE.text");
  ASSERT_TRUE(value);
  EXPECT_EQ(*value, "\"gn\"");
}

TEST_F(TreeSitterGnTest, ExposesGnNodeKinds) {
  // `kind` matches tree-sitter node types, which exist only if the grammar
  // itself loaded: `assignment_statement` comes from tree-sitter-gn.
  const std::vector<base::Value::Dict> matches = Scan(kNodeKindRule);

  std::vector<std::string> texts;
  for (const base::Value::Dict& match : matches) {
    const std::string* text = match.FindString("text");
    ASSERT_TRUE(text);
    texts.push_back(*text);
  }
  EXPECT_THAT(texts, ::testing::ElementsAre("output_name = \"gn\"",
                                            "sources = [ \"parser.c\" ]"));
}

}  // namespace
