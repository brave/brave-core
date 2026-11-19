// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/replace_body_handler.h"

#include <string>
#include <string_view>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

// What a unit test can reach is the parsing and the rewriting. The rest of
// the feature is delivery, and needs a browser: that a non-text MIME type is
// never buffered, that a body over the ceiling is passed through unmodified
// rather than failing, that one grown past it by its own rewrite does fail,
// that Content-Length is cleared and the code cache passed through, and that
// the head reaches the renderer when delivery is taken over rather than after
// the body is ready.

namespace brave_shields {

namespace {

// Parses `directive` and applies it, so a case reads as the filter an author
// would write and the body a server would send.
std::string Rewrite(std::string_view directive, std::string body) {
  auto parsed = ReplaceDirective::Parse(directive);
  EXPECT_TRUE(parsed.has_value()) << "did not parse: " << directive;
  if (!parsed.has_value()) {
    return body;
  }
  return parsed->Apply(std::move(body));
}

// Applies several in the order given, which is how the response path composes
// them.
std::string RewriteAll(const std::vector<std::string_view>& directives,
                       std::string body) {
  for (const auto& directive : directives) {
    body = Rewrite(directive, std::move(body));
  }
  return body;
}

}  // namespace

// ---------------------------------------------------------------- parsing

TEST(ReplaceDirectiveTest, DeclinesWhatIsNotADirective) {
  EXPECT_FALSE(ReplaceDirective::Parse("").has_value());
  EXPECT_FALSE(ReplaceDirective::Parse("a/b/c/").has_value());
  EXPECT_FALSE(ReplaceDirective::Parse("/a/b").has_value());
  EXPECT_FALSE(ReplaceDirective::Parse("/only/").has_value());
}

// Without the leading delimiter the first character of the pattern is simply
// dropped - "xa/b/" would rewrite on "a" - so the shape is checked rather
// than assumed.
TEST(ReplaceDirectiveTest, DeclinesADirectiveThatDoesNotStartAtADelimiter) {
  EXPECT_FALSE(ReplaceDirective::Parse("xa/b/").has_value());
}

// An empty pattern matches everywhere, so a global directive would paste its
// replacement between every character. The engine rejects one too.
TEST(ReplaceDirectiveTest, DeclinesAnEmptyPattern) {
  EXPECT_FALSE(ReplaceDirective::Parse("//b/").has_value());
  EXPECT_FALSE(ReplaceDirective::Parse("///g").has_value());
}

TEST(ReplaceDirectiveTest, DeclinesAnUnknownFlag) {
  EXPECT_FALSE(ReplaceDirective::Parse("/a/b/x/").has_value());
  // Three delimiters leave "c/g" as the flags, which is not flags.
  EXPECT_FALSE(ReplaceDirective::Parse("/a/b/c/g").has_value());
}

TEST(ReplaceDirectiveTest, TakesTheFlagsUBOUses) {
  for (const char* directive : {"/a/b/", "/a/b/g", "/a/b/i", "/a/b/s",
                                "/a/b/m", "/a/b/u", "/a/b/gms"}) {
    EXPECT_TRUE(ReplaceDirective::Parse(directive).has_value()) << directive;
  }
}

// RE2 declines lookahead and lookbehind, so those patterns go to ICU. What
// this pins is which engine takes them: a pattern that landed on RE2 would
// lose the lookaround semantics silently, and one that landed on ICU would
// buy backtracking for nothing.
TEST(ReplaceDirectiveTest, SendsLookaroundToIcuAndEverythingElseToRe2) {
  for (const char* directive : {
           "/(?!x)y/z/",
           "/(?<=a)b/x/",
           // The tokopedia rule, as it ships.
           R"(/(?:(?!"isTopads":false).)+?/x/)",
           // The Twitch HLS rule, as it ships.
           R"(/#EXT-X-DATERANGE:\S+?CLASS="twitch-ad-quartile".+?(?=(#EXT-X-DATERANGE|$))//gs)",
       }) {
    auto parsed = ReplaceDirective::Parse(directive);
    EXPECT_TRUE(parsed.has_value()) << directive;
    if (!parsed.has_value()) {
      continue;
    }
    EXPECT_EQ(ReplaceDirective::Engine::kIcu, parsed->engine()) << directive;
  }
  for (const char* directive : {
           "/a/b/",
           R"(/"adPlacements"/"no_ads"/)",
           R"(/"adPlacements.*?("adSlots"|"adBreakHeartbeatParams")/$1/gms)",
       }) {
    auto parsed = ReplaceDirective::Parse(directive);
    EXPECT_TRUE(parsed.has_value()) << directive;
    if (!parsed.has_value()) {
      continue;
    }
    EXPECT_EQ(ReplaceDirective::Engine::kRe2, parsed->engine()) << directive;
  }
}

// A pattern neither engine will take is still declined.
TEST(ReplaceDirectiveTest, DeclinesAPatternNeitherEngineCanCompile) {
  EXPECT_FALSE(ReplaceDirective::Parse("/[/x/").has_value());
  EXPECT_FALSE(ReplaceDirective::Parse("/a{2,1}/x/").has_value());
  EXPECT_FALSE(ReplaceDirective::Parse("/(?<!/x/").has_value());
}

// ------------------------------------------------------- the ICU fallback

// Every expectation in this section is what node gives for the same regex and
// the same body, because uBO runs these through JS and that is the bar.

TEST(ReplaceDirectiveTest, LookaheadRewritesOnTheIcuPath) {
  // The tokopedia rule as it ships: delete the item whose flag is true and
  // leave the ones whose flag is false. That is what needs the negative
  // lookahead - a plain `.+?` would run from the first item to the last true
  // flag and take the innocent ones with it.
  EXPECT_EQ(
      R"([{"id":123456789,"name":"keep me","isTopads":false,"__typename":"recommendationItem"},{"id":111222333,"name":"keep me too","isTopads":false,"__typename":"recommendationItem"}])",
      Rewrite(
          R"(/\{"id":\d{9,11}(?:(?!"isTopads":false).)+?"isTopads":true.+?"__typename":"recommendationItem"\},//g)",
          R"([{"id":123456789,"name":"keep me","isTopads":false,"__typename":"recommendationItem"},{"id":987654321,"name":"an ad","isTopads":true,"__typename":"recommendationItem"},{"id":111222333,"name":"keep me too","isTopads":false,"__typename":"recommendationItem"}])"));
}

TEST(ReplaceDirectiveTest, LookbehindRewritesOnTheIcuPath) {
  EXPECT_EQ("a[x]c", Rewrite(R"(/(?<=a\[)b/x/)", "a[b]c"));
}

TEST(ReplaceDirectiveTest, TheIcuPathTakesTheSameFlags) {
  // Empty matches included, which is where an engine that did not advance
  // would spin: JS gives "axxbaxxb" here, and so must this.
  EXPECT_EQ("axxbaxxb", Rewrite("/(?<=a)a*/x/g", "aabaab"));
  // And without g it is one replacement on this path too.
  EXPECT_EQ(",x,a", Rewrite("/(?<=,)a/x/", ",a,a"));
  EXPECT_EQ(",x,x", Rewrite("/(?<=,)a/x/g", ",a,a"));
  EXPECT_EQ("Ax", Rewrite("/(?<=A)b/x/i", "Ab"));
  EXPECT_EQ("a\nx", Rewrite("/(?<=^)b/x/m", "a\nb"));
}

TEST(ReplaceDirectiveTest, GroupReferencesWorkOnTheIcuPath) {
  EXPECT_EQ("ba", Rewrite("/(?<=^)(a)(b)/$2$1/", "ab"));
  // Out of range stays literal, as in JS. Letting it reach ICU instead would
  // error there, and an erroring ICU replacement returns an EMPTY string -
  // so this is the difference between a literal $9 and a deleted body.
  EXPECT_EQ("$9", Rewrite("/(?<=^)(a)/$9/", "a"));
  EXPECT_EQ("$", Rewrite("/(?<=^)a/$$/", "a"));
  // $0 is not a group reference in JS, so it must not become the whole match.
  EXPECT_EQ("$0", Rewrite("/(?<=^)(a)/$0/", "a"));
}

// Same question on the RE2 path, where $0 used to reach RE2 as \0 and be
// substituted with the whole match, which JS does not do.
TEST(ReplaceDirectiveTest, DollarZeroIsLiteralOnBothPaths) {
  EXPECT_EQ("$0", Rewrite("/(a)/$0/", "a"));
  EXPECT_EQ("x$0y", Rewrite("/a/x$0y/", "a"));
}

// ICU decodes the body as UTF-8 and would turn every ill-formed byte into
// U+FFFD, so a body it cannot read is left alone rather than corrupted.
TEST(ReplaceDirectiveTest, LeavesANonUtf8BodyAloneOnTheIcuPath) {
  const std::string latin1 = "caf\xE9 end";  // e9 is latin-1, not UTF-8
  EXPECT_EQ(latin1, Rewrite(R"(/(?<=caf)./X/)", latin1));
  // The same text as valid UTF-8 does get rewritten, so the latin-1 case is
  // about the encoding and not about the pattern.
  EXPECT_EQ("cafX end", Rewrite(R"(/(?<=caf)./X/)", "caf\xC3\xA9 end"));
}

// Puts the step limit back whatever the case does.
class ScopedIcuStepLimit {
 public:
  explicit ScopedIcuStepLimit(int32_t limit) {
    ReplaceDirective::SetIcuStepLimitForTesting(limit);
  }
  ~ScopedIcuStepLimit() { ReplaceDirective::SetIcuStepLimitForTesting(0); }
};

// ICU backtracks, so a pattern from a filter list can be made to run away on
// a body - (a+)+b over 26 a's takes two seconds unbounded. A match that runs
// past the step limit is abandoned and the body comes back whole, rather than
// truncated, empty, or three seconds late.
//
// The body here has a match in it, past a prefix that backtracks: that is
// what makes this case say something. Under a limit the directive does not
// apply; with the default limit the same directive on the same body rewrites,
// exactly as JS does.
TEST(ReplaceDirectiveTest, AbandonsAMatchThatRunsPastTheStepLimit) {
  const std::string body = std::string(16, 'a') + "Xaab";
  {
    ScopedIcuStepLimit limit(1);
    EXPECT_EQ(body, Rewrite("/(?<=a)(a+)+b/x/", body));
  }
  EXPECT_EQ(std::string(16, 'a') + "Xax", Rewrite("/(?<=a)(a+)+b/x/", body));
}

// A global rewrite can time out after it has already replaced something, and
// what is in hand then is neither the body the server sent nor the rewrite the
// rule asks for. ICU reports it the same way it reports running out of
// matches - find() returns false - so the error has to be read, or a part-done
// rewrite goes out as if it were finished.
//
// Here the body has two matches with a backtracking stretch between them. JS
// replaces both: "axaaaaaaaaaaaaaaaaXax". Delivering what was in hand at the
// timeout would give "axaaaaaaaaaaaaaaaaXaab", one of the two. Neither is what
// this does, which is to leave the body alone.
TEST(ReplaceDirectiveTest, AbandonsAPartDoneRewriteRatherThanDeliveringIt) {
  const std::string body = "aab" + std::string(16, 'a') + "Xaab";
  {
    ScopedIcuStepLimit limit(1);
    EXPECT_EQ(body, Rewrite("/(?<=a)(a+)+b/x/g", body));
  }
  EXPECT_EQ("ax" + std::string(16, 'a') + "Xax",
            Rewrite("/(?<=a)(a+)+b/x/g", body));
}

TEST(ReplaceDirectiveTest, CompilesTheLiveRules) {
  // Rules from the lists Brave already ships, plus the YouTube pair in the
  // deletion form that works.
  for (const char* directive : {
           R"(/"adPlacements.*?("adSlots"|"adBreakHeartbeatParams")/$1/gms)",
           R"(/"playerAds".*?"adSlots"/"adSlots"/s)",
           R"(/popunder//)",
           R"(/"isTopads":true/"isTopads":false/g)",
       }) {
    EXPECT_TRUE(ReplaceDirective::Parse(directive).has_value()) << directive;
  }
}

// ICU's `$`, like Perl's and Java's, matches at the end of the input AND
// before a line terminator that ends it; JS's `$` without `m` matches only at
// the very end. Untranslated, a lookahead for `$` stops one character earlier
// on the ICU path than it does in uBO, so a body ending in a newline keeps
// that newline - and where the `$` alternative is the only way to match, uBO
// deletes and ICU does not.
//
// Measured against node, not invented: the uAssets Twitch rule on a real-
// shaped 18 KB playlist came back one byte wider than node's, byte-identical
// only once the body's trailing newline was removed. `$` is translated to
// `\z` for ICU so the two now agree.
TEST(ReplaceDirectiveTest, TheIcuDollarMeansEndOfBodyAsInJs) {
  // node: "a". The newline is inside what JS matches, so it goes too.
  EXPECT_EQ("a", Rewrite("/b.*?(?=(c|$))//gs", "ab\n"));
  // node: "ab\n" - without `s` the `.*?` cannot cross the newline to reach
  // the end of the input, so nothing matches and nothing is removed.
  EXPECT_EQ("ab\n", Rewrite("/b.*?(?=(c|$))//g", "ab\n"));
  // With no trailing newline there was never a disagreement.
  EXPECT_EQ("a", Rewrite("/b.*?(?=(c|$))//gs", "ab"));
  // The anchor on its own, on the ICU path: `b` is not at the end of this
  // body, so uBO leaves it and so does this.
  EXPECT_EQ("ab\n", Rewrite("/b(?=x)|b$//gs", "ab\n"));
  // And RE2's `$` was already JS's: end of text, newline or no newline.
  EXPECT_EQ("a", Rewrite("/b.*$//gs", "ab\n"));
}

// `m` is the case the translation leaves alone, because UREGEX_MULTILINE
// already means what JS's `m` means: `$` at every line end.
TEST(ReplaceDirectiveTest, TheIcuDollarStaysPerLineWithM) {
  // Both lines end at a `$`, and the lookahead keeps this on the ICU path.
  EXPECT_EQ("X\nX\n", Rewrite("/[ab](?=$)/X/gm", "a\nb\n"));
}

// A `$` that is not an anchor is not translated: escaped, or inside a
// character class, it is a literal dollar in JS and has to stay one here.
TEST(ReplaceDirectiveTest, LeavesANonAnchorDollarAlone) {
  // Lookahead forces the ICU path in both cases.
  EXPECT_EQ("X9", Rewrite("/\\$(?=9)/X/g", "$9"));
  EXPECT_EQ("X9", Rewrite("/[$](?=9)/X/g", "$9"));
  // A class holding an escaped `]` alongside the dollar still ends where it
  // should, so the dollar after it is the anchor.
  EXPECT_EQ("a", Rewrite("/[\\]$]b(?=$)//g", "a$b"));
}

// ---------------------------------------------------------------- applying

TEST(ReplaceDirectiveTest, ReplacesTheFirstMatchWithoutG) {
  EXPECT_EQ("Xaa", Rewrite("/a/X/", "aaa"));
}

TEST(ReplaceDirectiveTest, ReplacesEveryMatchWithG) {
  EXPECT_EQ("XXX", Rewrite("/a/X/g", "aaa"));
}

TEST(ReplaceDirectiveTest, IgnoresCaseWithI) {
  EXPECT_EQ("X", Rewrite("/abc/X/i", "ABC"));
}

TEST(ReplaceDirectiveTest, DotCrossesNewlinesOnlyWithS) {
  EXPECT_EQ("X", Rewrite("/a.b/X/s", "a\nb"));
  EXPECT_EQ("a\nb", Rewrite("/a.b/X/", "a\nb"));
}

// RE2's one_line option only means anything under posix_syntax, so m has to
// reach the pattern as (?m) or it does nothing at all.
TEST(ReplaceDirectiveTest, AnchorsPerLineOnlyWithM) {
  EXPECT_EQ("a\nX", Rewrite("/^b/X/m", "a\nb"));
  EXPECT_EQ("a\nb", Rewrite("/^b/X/", "a\nb"));
}

TEST(ReplaceDirectiveTest, AnEmptyReplacementDeletes) {
  EXPECT_EQ("xy", Rewrite("/ads//", "xadsy"));
}

TEST(ReplaceDirectiveTest, LeavesABodyWithNoMatchAlone) {
  // The no-op control: delivery has to be byte-identical through the same
  // buffering and rewriting as any other response.
  const std::string body = "nothing in here matches";
  EXPECT_EQ(body, Rewrite("/zzz/X/", body));
}

// ------------------------------------------------- the replacement grammar

TEST(ReplaceDirectiveTest, DollarOneIsACaptureGroup) {
  EXPECT_EQ("[b]", Rewrite("/a(b)c/[$1]/", "abc"));
}

// RE2 fails the whole replacement over a reference to a group that is not
// there, which would make the directive silently do nothing. JS leaves it
// alone, so this does too.
TEST(ReplaceDirectiveTest, AGroupThatIsNotThereStaysLiteral) {
  EXPECT_EQ("$3", Rewrite("/a/$3/", "a"));
}

TEST(ReplaceDirectiveTest, DoubleDollarIsOneLiteralDollar) {
  EXPECT_EQ("$", Rewrite("/X/$$/", "X"));
}

TEST(ReplaceDirectiveTest, AnEscapedDelimiterWorksInThePattern) {
  EXPECT_EQ("X", Rewrite(R"(/a\/b/X/)", "a/b"));
}

// The escape protected the delimiter from the filter's own parsing. Passing
// it through to RE2 instead makes an invalid rewrite, and RE2 answers that by
// replacing nothing at all.
TEST(ReplaceDirectiveTest, AnEscapedDelimiterWorksInTheReplacement) {
  EXPECT_EQ("a/b", Rewrite(R"(/X/a\/b/)", "X"));
}

TEST(ReplaceDirectiveTest, ABackslashSurvivesAsABackslash) {
  EXPECT_EQ("a\\b", Rewrite(R"(/X/a\\b/)", "X"));
}

// ------------------------------------------------------------- composition

TEST(ReplaceDirectiveTest, DirectivesApplyInOrder) {
  // The second sees the first's output, which is what makes them compose
  // rather than one of them winning.
  EXPECT_EQ("c", RewriteAll({"/a/b/", "/b/c/"}, "a"));
}

TEST(ReplaceDirectiveTest, OneDirectiveCanUndoAnother) {
  EXPECT_EQ("a", RewriteAll({"/a/b/g", "/b/a/g"}, "a"));
}

// A replacement longer than what it matched grows the body, which is why the
// ceiling is applied to the result and not only to what was buffered.
TEST(ReplaceDirectiveTest, AReplacementCanGrowTheBody) {
  EXPECT_EQ("XXXXXXXXXXXX", Rewrite("/a/XXXX/g", "aaa"));
}

// A pattern that can match the empty string is accepted by the engine - only
// an empty pattern is rejected there - and multiplies the body.
TEST(ReplaceDirectiveTest, AnEmptyMatchingPatternMultipliesTheBody) {
  EXPECT_GT(Rewrite("/x*/Y/g", "abc").size(), std::string("abc").size());
}

// --------------------------------------------------- the charset gate

// Most responses have no charset at all, and text/* without one is ASCII as
// far as everything downstream is concerned.
TEST(CharsetTest, NoCharsetIsRewritable) {
  EXPECT_TRUE(IsRewritableCharset(""));
}

TEST(CharsetTest, Utf8AndAsciiAreRewritable) {
  EXPECT_TRUE(IsRewritableCharset("utf-8"));
  EXPECT_TRUE(IsRewritableCharset("UTF-8"));
  EXPECT_TRUE(IsRewritableCharset("utf8"));
  EXPECT_TRUE(IsRewritableCharset("us-ascii"));
}

// The single-byte families, where every byte below 0x80 is that ASCII
// character - which is what the RE2 path needs to match bytes safely.
TEST(CharsetTest, SingleByteWesternCharsetsAreRewritable) {
  EXPECT_TRUE(IsRewritableCharset("iso-8859-1"));
  EXPECT_TRUE(IsRewritableCharset("ISO_8859-15"));
  EXPECT_TRUE(IsRewritableCharset("iso8859-2"));
  EXPECT_TRUE(IsRewritableCharset("windows-1252"));
  EXPECT_TRUE(IsRewritableCharset("WINDOWS-1251"));
  EXPECT_TRUE(IsRewritableCharset("cp1250"));
  EXPECT_TRUE(IsRewritableCharset("x-cp1256"));
}

// The ones that would corrupt: a trail byte can be an ASCII letter in
// Shift_JIS, Big5, GBK and GB18030, ISO-2022-JP changes what a byte means
// with escape sequences, and UTF-16 is not byte-addressable.
TEST(CharsetTest, CharsetsWhereAsciiIsNotLiteralAreNotRewritable) {
  EXPECT_FALSE(IsRewritableCharset("shift_jis"));
  EXPECT_FALSE(IsRewritableCharset("Shift_JIS"));
  EXPECT_FALSE(IsRewritableCharset("sjis"));
  EXPECT_FALSE(IsRewritableCharset("big5"));
  EXPECT_FALSE(IsRewritableCharset("gbk"));
  EXPECT_FALSE(IsRewritableCharset("gb18030"));
  EXPECT_FALSE(IsRewritableCharset("iso-2022-jp"));
  EXPECT_FALSE(IsRewritableCharset("utf-16"));
  EXPECT_FALSE(IsRewritableCharset("utf-16le"));
}

// windows-31j is Shift_JIS under a Windows name, which is why the prefix is
// windows-125 and not windows-.
TEST(CharsetTest, AWindowsNameForShiftJisIsNotRewritable) {
  EXPECT_FALSE(IsRewritableCharset("windows-31j"));
  EXPECT_FALSE(IsRewritableCharset("windows-932"));
}

// EUC keeps ASCII literal, so this one is parity with uBO rather than
// safety: it is not in uBO's charset map, so uBO declines it too.
TEST(CharsetTest, EucIsDeclinedForParityWithUbo) {
  EXPECT_FALSE(IsRewritableCharset("euc-jp"));
  EXPECT_FALSE(IsRewritableCharset("euc-kr"));
}

// ----------------------------------------------- the disposition gate

// The response path reads the header and asks this; what it passes when the
// header is absent is the empty string.
TEST(ContentDispositionTest, NoDispositionIsRewritable) {
  EXPECT_TRUE(IsInlineContentDisposition(""));
}

TEST(ContentDispositionTest, InlineIsRewritable) {
  EXPECT_TRUE(IsInlineContentDisposition("inline"));
}

// The parameters are about saving the file, not about the type.
TEST(ContentDispositionTest, InlineWithParametersIsRewritable) {
  EXPECT_TRUE(IsInlineContentDisposition("inline; filename=\"page.html\""));
}

// The HTTP token is case-insensitive, and whitespace around it is legal.
TEST(ContentDispositionTest, TheTypeIsCaseInsensitiveAndTrimmed) {
  EXPECT_TRUE(IsInlineContentDisposition("  INLINE  "));
  EXPECT_TRUE(IsInlineContentDisposition("Inline ; filename=a.txt"));
}

// The case the gate exists for: a download, which keeps its own body.
TEST(ContentDispositionTest, AnAttachmentIsNotRewritable) {
  EXPECT_FALSE(IsInlineContentDisposition("attachment"));
  EXPECT_FALSE(
      IsInlineContentDisposition("attachment; filename=\"accounts.csv\""));
}

// Anything that is not inline declines, which is uBO's answer too - and a
// type that merely starts with "inline" is not inline, which is where this
// is narrower than uBO's startsWith.
TEST(ContentDispositionTest, AnythingElseIsNotRewritable) {
  EXPECT_FALSE(IsInlineContentDisposition("form-data; name=\"field\""));
  EXPECT_FALSE(IsInlineContentDisposition("inline-but-not-really"));
  EXPECT_FALSE(IsInlineContentDisposition("inlinefoo"));
}

}  // namespace brave_shields
