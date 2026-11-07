/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/content/browser/replace_body_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "third_party/icu/source/common/unicode/unistr.h"
#include "third_party/icu/source/i18n/unicode/regex.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave_shields {

// Holds the compiled ICU pattern. A RegexPattern is immutable and hands out a
// matcher per use, which is what makes one of these shareable the way the RE2
// object is.
struct IcuReplacePattern {
  std::unique_ptr<const icu::RegexPattern> pattern;
  // Converted once here rather than per response: ICU wants UTF-16, and a
  // replacement is short, but it is one conversion per matching response.
  icu::UnicodeString replacement;
};

namespace {

// ICU backtracks where RE2 cannot, so a pattern from a filter list can be made
// to run away on a body: (a+)+b over 26 a's runs for two seconds unbounded.
// ICU's step limit is what bounds that, and it counts the whole rewrite rather
// than one match attempt - a matcher that finds 122k times spends the budget
// across all of them - so no separate aggregate limit is needed.
//
// Sizing it is a trade between declining a legitimate rule and stalling on a
// hostile one. Measured, on x86: the densest body the response ceiling allows,
// 10 MB matching every 90 bytes, completes in under 300 steps, and a
// catastrophic pattern spends about 75us per step before giving up. 2000 is
// therefore ~7x the worst legitimate case measured and ~150ms at the worst.
//
// Note that RegexMatcher::replaceAll() SWALLOWS this timeout - it returns the
// input and loses the error, which reads as "no match" - which is why the loop
// in ApplyWithIcu is written out by hand.
constexpr int32_t kIcuStepLimit = 2000;
int32_t g_icu_step_limit = kIcuStepLimit;

// And the backtracking stack ICU keeps on the heap is bounded as well, well
// under its own 8 MB default.
constexpr int32_t kIcuStackLimitBytes = 1024 * 1024;

// uBO writes $1 for a capture group; RE2 writes \1. RE2's rewrite grammar
// takes only \1-\9 and \\, and a backslash in front of anything else makes
// the whole replacement invalid - at which point RE2 replaces nothing, returns
// false and says nothing, so the directive silently does not happen. An escape
// in a filter is there to protect a character from the filter's own parsing,
// the delimiter above all, so it is spent here rather than passed on.
//
// `max_group` is what the compiled pattern actually captures. A reference past
// it is not a group, and RE2 would fail the whole replacement over it, so it
// is written out literally - which is what the same filter does in uBO, JS
// leaving an unmatched $5 alone. $0 is not a group reference in JS either, so
// it is literal too, though RE2 and ICU would both read it as the whole match.
std::string TranslateReplacementForRe2(std::string_view ubo_replacement,
                                       int max_group) {
  std::string out;
  out.reserve(ubo_replacement.size());
  for (size_t i = 0; i < ubo_replacement.size(); ++i) {
    const char c = ubo_replacement[i];
    if (c == '$' && i + 1 < ubo_replacement.size()) {
      const char next = ubo_replacement[i + 1];
      if (base::IsAsciiDigit(next)) {
        if (next != '0' && next - '0' <= max_group) {
          out.push_back('\\');
          out.push_back(next);
        } else {
          out.push_back('$');
          out.push_back(next);
        }
        ++i;
        continue;
      }
      if (next == '$') {
        // $$ is one literal dollar, which RE2 takes as itself.
        out.push_back('$');
        ++i;
        continue;
      }
    } else if (c == '\\' && i + 1 < ubo_replacement.size()) {
      const char next = ubo_replacement[++i];
      if (next == '\\') {
        out.append("\\\\");
      } else {
        out.push_back(next);
      }
      continue;
    } else if (c == '\\') {
      // A trailing backslash is a literal one.
      out.append("\\\\");
      continue;
    }
    out.push_back(c);
  }
  return out;
}

// ICU spells group references $1 the way uBO does, and absorbs following
// digits by exactly the rule JS uses - it takes another digit while the group
// number stays within what the pattern captures - so an in-range reference is
// passed straight through and $12 means group 12 where there are twelve, group
// 1 then a literal 2 where there are not, as in JS.
//
// What has to be handled is everything ICU treats as an error rather than as
// text, because an erroring replacement there returns an EMPTY string, not the
// input: a reference past the group count, a bare $, and a backslash, which
// escapes anything in an ICU replacement. $0 is literal in JS, so it is
// escaped here as well.
std::string TranslateReplacementForIcu(std::string_view ubo_replacement,
                                       int max_group) {
  std::string out;
  out.reserve(ubo_replacement.size());
  for (size_t i = 0; i < ubo_replacement.size(); ++i) {
    const char c = ubo_replacement[i];
    if (c == '$' && i + 1 < ubo_replacement.size()) {
      const char next = ubo_replacement[i + 1];
      if (base::IsAsciiDigit(next)) {
        if (next != '0' && next - '0' <= max_group) {
          // In range: hand both to ICU and let it read any further digits.
          out.push_back('$');
        } else {
          out.append("\\$");
        }
        out.push_back(next);
        ++i;
        continue;
      }
      if (next == '$') {
        out.append("\\$");
        ++i;
        continue;
      }
      // A dollar in front of anything else is text, and has to be said so.
      out.append("\\$");
      continue;
    } else if (c == '$') {
      // A trailing dollar is text too.
      out.append("\\$");
      continue;
    } else if (c == '\\' && i + 1 < ubo_replacement.size()) {
      const char next = ubo_replacement[++i];
      if (next == '\\' || next == '$') {
        out.push_back('\\');
      }
      out.push_back(next);
      continue;
    } else if (c == '\\') {
      out.append("\\\\");
      continue;
    }
    out.push_back(c);
  }
  return out;
}

// JS and ICU disagree about `$` when the `m` flag is off: JS matches it only
// at the very end of the body, while ICU, like Perl and Java, also matches it
// before a line terminator that ends the body. ICU spells the JS meaning
// `\z`, so every `$` that is an anchor - not escaped, not inside a character
// class - is translated to it.
//
// uBO's rules lean on this. The Twitch HLS rule ends its third alternative
// with `(?=(#EXT-X-DATERANGE|$))`, and against a playlist that ends in a
// newline the untranslated `$` stops the lookahead one character early, which
// leaves the rewrite one byte wider than uBO's and, where the alternation is
// the only way to match, leaves the body untouched.
//
// With `m` on, UREGEX_MULTILINE already means what JS's `m` means, so `$` is
// left alone.
std::string TranslatePatternForIcu(std::string_view pattern, bool multiline) {
  if (multiline) {
    return std::string(pattern);
  }
  std::string out;
  out.reserve(pattern.size() + 2);
  bool in_class = false;
  for (size_t i = 0; i < pattern.size(); ++i) {
    const char c = pattern[i];
    // An escape carries its next character through untouched, which is what
    // keeps `\$` a literal dollar and `\]` inside a class from closing it.
    if (c == '\\' && i + 1 < pattern.size()) {
      out.push_back(c);
      out.push_back(pattern[i + 1]);
      ++i;
      continue;
    }
    if (c == '[') {
      in_class = true;
    } else if (c == ']') {
      in_class = false;
    }
    if (c == '$' && !in_class) {
      out.append("\\z");
      continue;
    }
    out.push_back(c);
  }
  return out;
}

// static

}  // namespace

// Out of line because the pattern members point at types this header only
// forward-needs; defining them here keeps RE2 and ICU out of every includer.
ReplaceDirective::ReplaceDirective() = default;
ReplaceDirective::ReplaceDirective(ReplaceDirective&&) = default;
ReplaceDirective::~ReplaceDirective() = default;

std::optional<ReplaceDirective> ReplaceDirective::Parse(
    std::string_view directive) {
  // /pattern/replacement/flags, with an escaped delimiter allowed inside
  // either half. The adblock engine has already checked this shape, so a
  // failure here is a pattern neither engine will take rather than a malformed
  // directive.
  if (directive.empty() || directive.front() != '/') {
    return std::nullopt;
  }
  std::vector<size_t> cuts;
  bool escaped = false;
  for (size_t i = 1; i < directive.size(); ++i) {
    if (escaped) {
      escaped = false;
      continue;
    }
    if (directive[i] == '\\') {
      escaped = true;
    } else if (directive[i] == '/') {
      cuts.push_back(i);
    }
  }
  if (cuts.size() < 2) {
    return std::nullopt;
  }
  const std::string_view pattern = directive.substr(1, cuts[0] - 1);
  const std::string_view replacement =
      directive.substr(cuts[0] + 1, cuts[1] - cuts[0] - 1);
  const std::string_view flags = directive.substr(cuts[1] + 1);

  // An empty pattern matches at every position, so a global directive would
  // paste its replacement between every character of the body. The engine
  // rejects one as well; this is the same answer on this side of the bridge.
  if (pattern.empty()) {
    return std::nullopt;
  }

  re2::RE2::Options options;
  options.set_log_errors(false);
  options.set_never_capture(false);
  uint32_t icu_flags = 0;
  bool multiline = false;
  for (const char flag : flags) {
    switch (flag) {
      case 'i':
        options.set_case_sensitive(false);
        icu_flags |= UREGEX_CASE_INSENSITIVE;
        break;
      case 's':
        options.set_dot_nl(true);
        icu_flags |= UREGEX_DOTALL;
        break;
      case 'm':
        multiline = true;
        icu_flags |= UREGEX_MULTILINE;
        break;  // RE2 takes it as a (?m) prefix on the pattern
      case 'g':
      case 'u':
        break;  // `g` is held in global_; `u` is implicit in both engines
      default:
        return std::nullopt;
    }
  }

  ReplaceDirective held;
  held.global_ = flags.find('g') != std::string_view::npos;

  // RE2 reads ^ and $ as the bounds of the whole body, and its one_line option
  // only means anything under posix_syntax, which this is not. (?m) is how RE2
  // spells what uBO's m flag asks for; ICU has a flag for it.
  const std::string pattern_text =
      multiline ? base::StrCat({"(?m)", pattern}) : std::string(pattern);
  auto compiled = std::make_unique<const re2::RE2>(pattern_text, options);
  if (compiled->ok()) {
    held.engine_ = Engine::kRe2;
    held.replacement_ = TranslateReplacementForRe2(
        replacement, compiled->NumberOfCapturingGroups());
    held.pattern_ = std::move(compiled);
    return held;
  }

  // RE2 would not take it. That is usually lookahead or lookbehind, which it
  // has no equivalent for, so the pattern goes to ICU instead - at the cost of
  // backtracking, UTF-16 and the limits in Apply.
  UErrorCode status = U_ZERO_ERROR;
  const icu::UnicodeString icu_pattern_text =
      icu::UnicodeString::fromUTF8(TranslatePatternForIcu(pattern, multiline));
  std::unique_ptr<icu::RegexPattern> icu_compiled(
      icu::RegexPattern::compile(icu_pattern_text, icu_flags, status));
  if (U_FAILURE(status) || !icu_compiled) {
    return std::nullopt;
  }
  // RegexPattern does not report its group count; a matcher does, and one is
  // cheap to make and throw away once.
  int group_count = 0;
  {
    UErrorCode matcher_status = U_ZERO_ERROR;
    std::unique_ptr<icu::RegexMatcher> probe(
        icu_compiled->matcher(matcher_status));
    if (U_FAILURE(matcher_status) || !probe) {
      return std::nullopt;
    }
    group_count = probe->groupCount();
  }

  auto icu_held = std::make_unique<IcuReplacePattern>();
  icu_held->pattern = std::move(icu_compiled);
  icu_held->replacement = icu::UnicodeString::fromUTF8(
      TranslateReplacementForIcu(replacement, group_count));
  held.engine_ = Engine::kIcu;
  held.icu_pattern_ = std::move(icu_held);
  return held;
}

std::string ReplaceDirective::Apply(std::string body) const {
  return engine_ == Engine::kRe2 ? ApplyWithRe2(std::move(body))
                                 : ApplyWithIcu(std::move(body));
}

// static
void ReplaceDirective::SetIcuStepLimitForTesting(int32_t limit) {
  g_icu_step_limit = limit > 0 ? limit : kIcuStepLimit;
}

std::string ReplaceDirective::ApplyWithRe2(std::string body) const {
  if (global_) {
    re2::RE2::GlobalReplace(&body, *pattern_, replacement_);
  } else {
    re2::RE2::Replace(&body, *pattern_, replacement_);
  }
  return body;
}

std::string ReplaceDirective::ApplyWithIcu(std::string body) const {
  // ICU works in UTF-16, so the body has to be decoded - and a body that is
  // not valid UTF-8 comes back with every ill-formed byte turned into U+FFFD,
  // which is corruption rather than a rewrite. uBO regexes decoded text, so
  // declining here is the same answer with less damage; the directive simply
  // does not apply to a body we cannot read.
  if (!base::IsStringUTF8(body)) {
    return body;
  }

  UErrorCode status = U_ZERO_ERROR;
  std::unique_ptr<icu::RegexMatcher> matcher(
      icu_pattern_->pattern->matcher(status));
  if (U_FAILURE(status) || !matcher) {
    return body;
  }
  // fromUTF8 copies rather than aliasing, so from here there are two of the
  // body. reset() does alias, so `input` has to outlive the matcher.
  const icu::UnicodeString input = icu::UnicodeString::fromUTF8(body);
  matcher->reset(input);
  matcher->setTimeLimit(g_icu_step_limit, status);
  matcher->setStackLimit(kIcuStackLimitBytes, status);
  if (U_FAILURE(status)) {
    return body;
  }

  icu::UnicodeString out;
  bool matched = false;
  while (matcher->find(status)) {
    if (!matched) {
      matched = true;
      // The output is reserved once here rather than regrown as the body is
      // copied across it. ICU has no reserve(), so this is what it looks
      // like: take a buffer of the size wanted and give it straight back
      // empty, which keeps the capacity.
      if (out.getBuffer(input.length() + 1)) {
        out.releaseBuffer(0);
      }
      // And the UTF-8 copy goes back now rather than being held alongside the
      // UTF-16 one and the output for the length of the rewrite. No later
      // step reads `body`: the one path that still wants the original
      // re-encodes it from `input`, which round-trips exactly for the UTF-8 this
      // has already checked for. Doing this at the first match rather than at
      // the decode is what keeps a response with no match - most of them -
      // from paying for a re-encode it does not need.
      std::string released;
      body.swap(released);
    }
    matcher->appendReplacement(out, icu_pattern_->replacement, status);
    if (U_FAILURE(status)) {
      break;
    }
    if (!global_) {
      break;
    }
  }
  // A find() that ran out of steps returns false with the error set, which is
  // not the same answer as no match and must not be read as one.
  const bool failed = U_FAILURE(status);
  if (!matched) {
    // Nothing was replaced, so the body is still here to hand back.
    return body;
  }
  std::string result;
  if (failed) {
    input.toUTF8String(result);
    return result;
  }
  matcher->appendTail(out);
  out.toUTF8String(result);
  return result;
}

bool IsRewritableCharset(std::string_view charset) {
  // Absent is what most responses have, and text/* with no charset is read as
  // ASCII-compatible by everything downstream.
  if (charset.empty()) {
    return true;
  }
  for (const std::string_view exact : {"utf-8", "utf8", "us-ascii", "ascii"}) {
    if (base::EqualsCaseInsensitiveASCII(charset, exact)) {
      return true;
    }
  }
  // The single-byte families, by prefix because each has a dozen members and
  // all of them keep ASCII literal. `windows-125` rather than `windows-`,
  // because windows-31j is Shift_JIS under another name.
  for (const std::string_view prefix :
       {"iso-8859-", "iso8859-", "iso_8859-", "windows-125", "cp125",
        "x-cp125"}) {
    if (charset.size() >= prefix.size() &&
        base::EqualsCaseInsensitiveASCII(charset.substr(0, prefix.size()),
                                         prefix)) {
      return true;
    }
  }
  return false;
}

bool IsInlineContentDisposition(std::string_view disposition) {
  // Everything before the first `;` is the type; past it are parameters like
  // filename=, which say nothing about whether the body may be rewritten.
  // find() returning npos means there are none, and substr() takes that as
  // "to the end".
  const std::string_view type = base::TrimWhitespaceASCII(
      disposition.substr(0, disposition.find(';')), base::TRIM_ALL);
  // Empty is what a header that is there but says nothing looks like, and it
  // is no reason to decline - it is also what the caller passes when the
  // header is absent.
  return type.empty() || base::EqualsCaseInsensitiveASCII(type, "inline");
}

}  // namespace brave_shields
