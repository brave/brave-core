/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_REPLACE_BODY_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_REPLACE_BODY_HANDLER_H_

#include <cstdint>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace re2 {
class RE2;
}  // namespace re2

namespace brave_shields {

// The ICU side of a directive, defined in the .cc so that neither ICU nor RE2
// reaches an includer of this header.
struct IcuReplacePattern;

// One parsed $replace directive: /pattern/replacement/flags as the filter
// wrote it, split and compiled once.
class ReplaceDirective {
 public:
  // Which engine took the pattern. RE2 wherever it can, because it cannot
  // backtrack and so cannot be made to run away on a body; ICU for what RE2
  // refuses, which in practice means lookahead and lookbehind - uBO's lists
  // carry a handful, the Twitch HLS rule among them, and RE2 will never have
  // them because a finite automaton cannot express them.
  enum class Engine { kRe2, kIcu };

  // Returns nullopt for a directive neither engine will take. The adblock
  // engine deliberately does not compile patterns, so this is where a bad one
  // is found - and it costs one directive, not the list.
  static std::optional<ReplaceDirective> Parse(std::string_view directive);

  ReplaceDirective(ReplaceDirective&&);
  ~ReplaceDirective();

  // Applies this directive to `body`, returning the result. Replaces once
  // unless the directive carries the `g` flag, which is what uBO does with
  // it: it builds `new RegExp(pattern, flags)` and leaves the rest to JS.
  //
  // Returns `body` unchanged where the directive cannot be applied safely -
  // on the ICU path, a body that is not valid UTF-8, or a match that exceeds
  // the ICU step and stack limits (see SetIcuStepLimitForTesting). A rule
  // that does not apply is uBO's own outcome
  // when it gives up on a response, and the alternative here is corrupting a
  // body or holding a sequence for seconds.
  std::string Apply(std::string body) const;

  Engine engine() const { return engine_; }

  // The ICU path stops a match that runs past a step limit, which is sized
  // for a 10 MB body; a test that wants to see it fire without one sets the
  // limit small and puts it back. Zero or less restores the default.
  static void SetIcuStepLimitForTesting(int32_t limit);

 private:
  ReplaceDirective();

  std::string ApplyWithRe2(std::string body) const;
  std::string ApplyWithIcu(std::string body) const;

  Engine engine_ = Engine::kRe2;
  // Exactly one of these is set, per `engine_`.
  std::unique_ptr<const re2::RE2> pattern_;
  std::unique_ptr<const IcuReplacePattern> icu_pattern_;
  // The RE2 path's replacement, already translated - the two engines spell
  // group references and escapes differently, and both treat an invalid
  // replacement as a reason to replace nothing at all. The ICU path's lives
  // in IcuReplacePattern, converted to UTF-16 once at parse time.
  std::string replacement_;
  bool global_ = false;
};

// Whether a response's charset lets its bytes be rewritten: true for an
// absent charset and for the encodings in which ASCII is literal - every byte
// below 0x80 is that ASCII character and never part of a multi-byte one.
//
// This matters because the RE2 path matches the body as bytes. In Shift_JIS,
// Big5 and GBK a trail byte can be an ASCII letter, so an ASCII pattern can
// match the second half of a character and a replacement then leaves a lead
// byte with nothing after it - a broken character, from a rule that looked
// like it only touched Latin text. ISO-2022-JP shifts meaning with escape
// sequences, and UTF-16 is not byte-addressable at all.
//
// uBO reaches the same answer from the other end: it decodes before matching,
// and declines whatever TextDecoder cannot be trusted with - its
// normalizedCharset map is utf-8 and the windows-125x family after aliasing,
// and anything missing from it makes canFilter() refuse. EUC-JP and EUC-KR
// happen to keep ASCII literal but are declined here too, because they are
// not in that map and parity is worth more than two encodings.
bool IsRewritableCharset(std::string_view charset);

// Whether a `Content-Disposition` header value lets the body be rewritten:
// true for an absent, empty or `inline` disposition, false for anything else.
// A response the browser is going to save rather than show is not what a
// filter list is describing, and rewriting a file on its way to disk is a
// different act from rewriting a page.
//
// This is uBO's gate - bodyFilterer.canFilter(), `disposition.startsWith(
// 'inline')` - with two deliberate differences, both narrowing. The type is
// matched case-insensitively, because the HTTP token is case-insensitive and a
// capital I should not quietly switch a filter rule off; and it is compared as
// a token rather than a prefix, so `inline-but-not-really` is declined where
// uBO's startsWith accepts it.
bool IsInlineContentDisposition(std::string_view disposition);


}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_REPLACE_BODY_HANDLER_H_
