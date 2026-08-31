// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/workspace_content_url_loader_factory.h"

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_POSIX)
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_util.h"

namespace ai_chat {

namespace {

constexpr char kUuid[] = "6b1b3f1e-0b7a-4f27-9a2f-2f2b9d4e5a10";

GURL WorkspaceURL(std::string_view path) {
  return GURL(base::StrCat({kLeoWorkspaceContentScheme,
                            url::kStandardSchemeSeparator, kUuid, path}));
}

}  // namespace

// The scheme is registered standard in production (BraveContentClient::
// AddAdditionalSchemes), which decides how GURL canonicalises these URLs -
// notably that backslashes become separators and dot segments are collapsed.
// The components test suite installs a plain ContentClient, so without this
// fixture the tests would exercise non-special URL parsing instead.
class WorkspaceContentURLTest : public testing::Test {
 public:
  WorkspaceContentURLTest() {
    url::AddStandardScheme(kLeoWorkspaceContentScheme, url::SCHEME_WITH_HOST);
  }

 protected:
  base::FilePath ParsedPath(std::string_view path) {
    std::optional<WorkspaceContentPath> parsed =
        ParseWorkspaceContentURL(WorkspaceURL(path));
    EXPECT_TRUE(parsed.has_value()) << path;
    if (!parsed) {
      return base::FilePath();
    }
    EXPECT_EQ(kUuid, parsed->uuid);
    return parsed->relative_path;
  }

  void ExpectRejected(std::string_view path) {
    EXPECT_FALSE(ParseWorkspaceContentURL(WorkspaceURL(path)).has_value())
        << path;
  }

 private:
  url::ScopedSchemeRegistryForTests scoped_scheme_registry_;
};

TEST_F(WorkspaceContentURLTest, ParsesUuidAndRelativePath) {
  EXPECT_EQ(base::FilePath::FromASCII("index.html"), ParsedPath("/index.html"));
  EXPECT_EQ(base::FilePath::FromASCII("assets").AppendASCII("app.js"),
            ParsedPath("/assets/app.js"));
}

TEST_F(WorkspaceContentURLTest, DirectoryRootServesIndex) {
  EXPECT_EQ(base::FilePath::FromASCII("index.html"), ParsedPath(""));
  EXPECT_EQ(base::FilePath::FromASCII("index.html"), ParsedPath("/"));
  EXPECT_EQ(base::FilePath::FromASCII("docs").AppendASCII("index.html"),
            ParsedPath("/docs/"));
}

TEST_F(WorkspaceContentURLTest, StripsQueryAndFragment) {
  EXPECT_EQ(base::FilePath::FromASCII("app.js"), ParsedPath("/app.js?v=2"));
  EXPECT_EQ(base::FilePath::FromASCII("app.js"), ParsedPath("/app.js#top"));
}

TEST_F(WorkspaceContentURLTest, DecodesEscapedNames) {
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("my file.html"),
            ParsedPath("/my%20file.html"));
  // Non-ASCII names are legitimate; they must survive rather than be rejected.
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("caf\u00e9.html"),
            ParsedPath("/caf%C3%A9.html"));
}

TEST_F(WorkspaceContentURLTest, RejectsEncodedSeparatorsWithinASegment) {
  // Encoded separators must not decode into real ones after the split.
  ExpectRejected("/%2e%2e%2fsecret");
  ExpectRejected("/foo%2f..%2fbar");
  ExpectRejected("/foo%5c..%5cbar");
  // Control bytes.
  ExpectRejected("/%00");
  ExpectRejected("/%0a");
}

TEST_F(WorkspaceContentURLTest, CanonicalisationCollapsesDotSegments) {
  // GURL collapses dot segments before we see the path, so traversal arrives
  // already resolved and never reaches DecodePathSegment as "..". Asserted
  // explicitly so a GURL behaviour change surfaces here rather than as a
  // traversal bug.
  for (const char* path : {"/../secret", "/%2e%2e/secret", "/./secret",
                           "/%2e/secret", "/a/../secret"}) {
    EXPECT_EQ(base::FilePath::FromASCII("secret"), ParsedPath(path)) << path;
  }
}

TEST_F(WorkspaceContentURLTest, CanonicalisationConvertsBackslashes) {
  // Being a standard scheme, GURL treats "\" as a path separator and collapses
  // the dot segments that follow, exactly as it would for http(s). Pinned here
  // because the reject-on-decode check in DecodePathSegment is only defence in
  // depth as long as this holds.
  EXPECT_EQ(base::FilePath::FromASCII("foo").AppendASCII("bar"),
            ParsedPath("/foo\\bar"));
  EXPECT_EQ(base::FilePath::FromASCII("bar"), ParsedPath("/foo\\..\\bar"));
}

TEST_F(WorkspaceContentURLTest, RejectsForeignSchemes) {
  EXPECT_FALSE(ParseWorkspaceContentURL(GURL("https://example.com/index.html"))
                   .has_value());
  EXPECT_FALSE(ParseWorkspaceContentURL(
                   GURL("chrome-untrusted://leo-workspace/x/index.html"))
                   .has_value());
  EXPECT_FALSE(ParseWorkspaceContentURL(GURL()).has_value());
}

TEST_F(WorkspaceContentURLTest, RejectsMissingHost) {
  // A standard scheme must have an authority, so this does not parse at all.
  EXPECT_FALSE(ParseWorkspaceContentURL(
                   GURL(base::StrCat({kLeoWorkspaceContentScheme,
                                      url::kStandardSchemeSeparator})))
                   .has_value());

  // An empty authority is not an empty host: GURL skips the extra slash and
  // promotes the first path segment, so this asks for workspace "a.html" and
  // fails closed at the registry rather than here.
  std::optional<WorkspaceContentPath> promoted = ParseWorkspaceContentURL(
      GURL(base::StrCat({kLeoWorkspaceContentScheme,
                         url::kStandardSchemeSeparator, "/a.html"})));
  ASSERT_TRUE(promoted.has_value());
  EXPECT_EQ("a.html", promoted->uuid);
}

TEST_F(WorkspaceContentURLTest, NormalisesUuidCase) {
  // GURL lowercases the host, so a registration made with the canonical uuid
  // still matches.
  std::optional<WorkspaceContentPath> parsed = ParseWorkspaceContentURL(GURL(
      base::StrCat({kLeoWorkspaceContentScheme, url::kStandardSchemeSeparator,
                    "6B1B3F1E-0B7A", "/index.html"})));
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ("6b1b3f1e-0b7a", parsed->uuid);
}

#if BUILDFLAG(IS_WIN)
TEST_F(WorkspaceContentURLTest, RejectsWindowsDriveAndAlternateDataStreams) {
  ExpectRejected("/C:");
  ExpectRejected("/C%3a/secret");
  ExpectRejected("/notes.txt:ads");
}
#endif  // BUILDFLAG(IS_WIN)

class WorkspaceContentFileTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    // ResolveWorkspaceFileBlocking() returns realpath()-resolved paths, and on
    // macOS the temp dir sits under /var, a symlink to /private/var. Normalise
    // up front so expectations can be written against |folder_|.
    root_ = base::MakeAbsoluteFilePath(temp_dir_.GetPath());
    ASSERT_FALSE(root_.empty());
    folder_ = root_.AppendASCII("workspace");
    ASSERT_TRUE(base::CreateDirectory(folder_));
    ASSERT_TRUE(
        base::WriteFile(folder_.AppendASCII("index.html"), "<h1>hi</h1>"));
  }

 protected:
  base::ScopedTempDir temp_dir_;
  base::FilePath root_;
  base::FilePath folder_;
};

TEST_F(WorkspaceContentFileTest, ResolvesFileInsideFolder) {
  EXPECT_EQ(folder_.AppendASCII("index.html"),
            ResolveWorkspaceFileBlocking(
                folder_, base::FilePath::FromASCII("index.html")));
}

TEST_F(WorkspaceContentFileTest, RejectsMissingFile) {
  EXPECT_EQ(std::nullopt, ResolveWorkspaceFileBlocking(
                              folder_, base::FilePath::FromASCII("nope.html")));
}

TEST_F(WorkspaceContentFileTest, RejectsDirectory) {
  ASSERT_TRUE(base::CreateDirectory(folder_.AppendASCII("assets")));
  EXPECT_EQ(std::nullopt, ResolveWorkspaceFileBlocking(
                              folder_, base::FilePath::FromASCII("assets")));
}

TEST_F(WorkspaceContentFileTest, RejectsParentReferences) {
  ASSERT_TRUE(base::WriteFile(root_.AppendASCII("outside.txt"), "secret"));
  EXPECT_EQ(
      std::nullopt,
      ResolveWorkspaceFileBlocking(
          folder_, base::FilePath::FromASCII("..").AppendASCII("outside.txt")));
}

TEST_F(WorkspaceContentFileTest, RejectsAbsolutePaths) {
  // The file exists, so this can only pass because of the IsAbsolute() guard.
  base::FilePath outside = root_.AppendASCII("outside.txt");
  ASSERT_TRUE(base::WriteFile(outside, "secret"));
  EXPECT_EQ(std::nullopt, ResolveWorkspaceFileBlocking(folder_, outside));
}

#if BUILDFLAG(IS_POSIX)
TEST_F(WorkspaceContentFileTest, RejectsSymlinkEscapingFolder) {
  // The case the resolve step exists for.
  base::FilePath outside = root_.AppendASCII("outside.txt");
  ASSERT_TRUE(base::WriteFile(outside, "secret"));
  ASSERT_TRUE(
      base::CreateSymbolicLink(outside, folder_.AppendASCII("link.txt")));

  EXPECT_EQ(std::nullopt, ResolveWorkspaceFileBlocking(
                              folder_, base::FilePath::FromASCII("link.txt")));
}

TEST_F(WorkspaceContentFileTest, AllowsSymlinkWithinFolder) {
  base::FilePath inside = folder_.AppendASCII("real.txt");
  ASSERT_TRUE(base::WriteFile(inside, "fine"));
  ASSERT_TRUE(
      base::CreateSymbolicLink(inside, folder_.AppendASCII("alias.txt")));

  EXPECT_EQ(inside, ResolveWorkspaceFileBlocking(
                        folder_, base::FilePath::FromASCII("alias.txt")));
}

TEST_F(WorkspaceContentFileTest, RejectsFifo) {
  // Would otherwise block a thread-pool thread forever waiting for EOF.
  base::FilePath fifo = folder_.AppendASCII("pipe");
  ASSERT_EQ(0, mkfifo(fifo.value().c_str(), 0600));
  EXPECT_EQ(std::nullopt, ResolveWorkspaceFileBlocking(
                              folder_, base::FilePath::FromASCII("pipe")));
}

TEST_F(WorkspaceContentFileTest, RejectsUnreadableFile) {
  base::FilePath unreadable = folder_.AppendASCII("locked.html");
  ASSERT_TRUE(base::WriteFile(unreadable, "secret"));
  ASSERT_TRUE(base::SetPosixFilePermissions(unreadable, 0));

  EXPECT_EQ(std::nullopt,
            ResolveWorkspaceFileBlocking(
                folder_, base::FilePath::FromASCII("locked.html")));
}
#endif  // BUILDFLAG(IS_POSIX)

}  // namespace ai_chat
