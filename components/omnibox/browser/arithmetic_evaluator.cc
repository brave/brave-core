/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/arithmetic_evaluator.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <numeric>

#include "base/i18n/number_formatting.h"
#include "base/numerics/checked_math.h"
#include "base/strings/string_util.h"

namespace omnibox {

BASE_FEATURE(kBraveLocalCalculator, base::FEATURE_ENABLED_BY_DEFAULT);

namespace {

// Fractional answers are rendered through a double, so their significant
// digits are capped at what a double round-trips exactly. Integer answers are
// formatted straight from the int64 and aren't subject to this.
constexpr int kMaxSignificantDigits = 15;

// Bounds recursion on untrusted omnibox input. Operator runs are folded
// iteratively, so this only has to cover the two constructs that genuinely
// nest: parentheses, and the right-associative '^'.
constexpr int kMaxRecursionDepth = 16;

// Well below what would overflow: a larger power is more likely a typo than a
// calculation, and evaluating it is pointless once the result can't be shown.
constexpr int64_t kMaxExponent = 64;

using CheckedInt = base::CheckedNumeric<int64_t>;

// An exact rational. `den` is always positive, gcd(|num|, den) == 1, and
// neither field is int64 min, so both can always be negated.
struct Rational {
  int64_t num = 0;
  int64_t den = 1;
};

std::optional<Rational> MakeRational(CheckedInt numerator,
                                     CheckedInt denominator) {
  int64_t num = 0;
  int64_t den = 0;
  if (!numerator.AssignIfValid(&num) || !denominator.AssignIfValid(&den)) {
    return std::nullopt;
  }
  // int64 min is excluded so negation and std::gcd are always well defined.
  constexpr int64_t kMin = std::numeric_limits<int64_t>::min();
  if (den == 0 || num == kMin || den == kMin) {
    return std::nullopt;
  }
  if (den < 0) {
    num = -num;
    den = -den;
  }
  const int64_t divisor = std::gcd(num, den);
  return Rational{num / divisor, den / divisor};
}

std::optional<Rational> Add(const Rational& a, const Rational& b) {
  return MakeRational(CheckedInt(a.num) * b.den + CheckedInt(b.num) * a.den,
                      CheckedInt(a.den) * b.den);
}

std::optional<Rational> Subtract(const Rational& a, const Rational& b) {
  return MakeRational(CheckedInt(a.num) * b.den - CheckedInt(b.num) * a.den,
                      CheckedInt(a.den) * b.den);
}

std::optional<Rational> Multiply(const Rational& a, const Rational& b) {
  return MakeRational(CheckedInt(a.num) * b.num, CheckedInt(a.den) * b.den);
}

std::optional<Rational> Divide(const Rational& a, const Rational& b) {
  if (b.num == 0) {
    return std::nullopt;
  }
  return MakeRational(CheckedInt(a.num) * b.den, CheckedInt(a.den) * b.num);
}

std::optional<Rational> Power(const Rational& base, const Rational& exponent) {
  // A fractional exponent gives an irrational result for all but a handful of
  // inputs, so don't try.
  if (exponent.den != 1) {
    return std::nullopt;
  }
  // Negation is safe: `Rational` excludes int64 min.
  const int64_t magnitude = std::abs(exponent.num);
  if (magnitude > kMaxExponent) {
    return std::nullopt;
  }

  Rational result{1, 1};
  for (int64_t i = 0; i < magnitude; ++i) {
    auto next = Multiply(result, base);
    if (!next) {
      return std::nullopt;
    }
    result = *next;
  }
  if (exponent.num < 0) {
    return Divide(Rational{1, 1}, result);
  }
  return result;
}

// Recursive descent over:
//   sum     := product (('+' | '-') product)*
//   product := unary (('*' | '/') unary)*
//   unary   := ('+' | '-')* power
//   power   := primary ('^' unary)?
//   primary := '(' sum ')' | number
class Parser {
 public:
  explicit Parser(std::u16string_view text) : text_(text) {}

  std::optional<Rational> Parse() {
    auto value = ParseSum();
    SkipWhitespace();
    // Trailing junk means this wasn't an expression after all.
    if (!value || pos_ != text_.size()) {
      return std::nullopt;
    }
    return value;
  }

 private:
  void SkipWhitespace() {
    while (pos_ < text_.size() && base::IsUnicodeWhitespace(text_[pos_])) {
      ++pos_;
    }
  }

  bool ConsumeIf(char16_t c) {
    SkipWhitespace();
    if (pos_ < text_.size() && text_[pos_] == c) {
      ++pos_;
      return true;
    }
    return false;
  }

  std::optional<Rational> ParseSum() {
    auto value = ParseProduct();
    while (value) {
      if (ConsumeIf(u'+')) {
        auto rhs = ParseProduct();
        value = rhs ? Add(*value, *rhs) : std::nullopt;
      } else if (ConsumeIf(u'-')) {
        auto rhs = ParseProduct();
        value = rhs ? Subtract(*value, *rhs) : std::nullopt;
      } else {
        break;
      }
    }
    return value;
  }

  std::optional<Rational> ParseProduct() {
    auto value = ParseUnary();
    while (value) {
      if (ConsumeIf(u'*')) {
        auto rhs = ParseUnary();
        value = rhs ? Multiply(*value, *rhs) : std::nullopt;
      } else if (ConsumeIf(u'/')) {
        auto rhs = ParseUnary();
        value = rhs ? Divide(*value, *rhs) : std::nullopt;
      } else {
        break;
      }
    }
    return value;
  }

  std::optional<Rational> ParseUnary() {
    // Signs are folded in a loop rather than by recursing per sign, so that a
    // long run of them ("+++...+1") can't exhaust the stack.
    bool negate = false;
    while (true) {
      if (ConsumeIf(u'-')) {
        negate = !negate;
      } else if (!ConsumeIf(u'+')) {
        break;
      }
    }

    auto value = ParsePower();
    if (!value || !negate) {
      return value;
    }
    // Negating in place is exact: `Rational` is reduced and excludes int64
    // min, so the sign flip can't overflow or need re-reducing.
    return Rational{-value->num, value->den};
  }

  // Right associative, and binds tighter than unary minus, so -2^2 is -4.
  std::optional<Rational> ParsePower() {
    auto base = ParsePrimary();
    if (!base || !ConsumeIf(u'^')) {
      return base;
    }
    // Right associativity means recursing per '^', so this needs the same
    // depth bound that nested parentheses get.
    if (++depth_ > kMaxRecursionDepth) {
      return std::nullopt;
    }
    auto exponent = ParseUnary();
    --depth_;
    return exponent ? Power(*base, *exponent) : std::nullopt;
  }

  std::optional<Rational> ParsePrimary() {
    if (ConsumeIf(u'(')) {
      if (++depth_ > kMaxRecursionDepth) {
        return std::nullopt;
      }
      auto value = ParseSum();
      --depth_;
      if (!value || !ConsumeIf(u')')) {
        return std::nullopt;
      }
      return value;
    }
    return ParseNumber();
  }

  // Only ASCII digits and '.' as the decimal mark. Grouping separators are
  // deliberately unsupported: "1,5" means different things in different
  // locales, and guessing wrong would produce a confidently wrong answer.
  std::optional<Rational> ParseNumber() {
    SkipWhitespace();
    CheckedInt digits = 0;
    CheckedInt scale = 1;
    bool any_digits = false;

    while (pos_ < text_.size() && base::IsAsciiDigit(text_[pos_])) {
      digits = digits * 10 + (text_[pos_] - u'0');
      ++pos_;
      any_digits = true;
    }
    if (pos_ < text_.size() && text_[pos_] == u'.') {
      ++pos_;
      while (pos_ < text_.size() && base::IsAsciiDigit(text_[pos_])) {
        digits = digits * 10 + (text_[pos_] - u'0');
        scale = scale * 10;
        ++pos_;
        any_digits = true;
      }
    }
    if (!any_digits) {
      return std::nullopt;
    }
    return MakeRational(digits, scale);
  }

  std::u16string_view text_;
  size_t pos_ = 0;
  int depth_ = 0;
};

// Whether something an operator could apply to ends at `pos`, which is what
// separates a binary operator from a leading sign.
bool HasLeftOperand(std::u16string_view text, size_t pos) {
  while (pos > 0) {
    const char16_t c = text[--pos];
    if (base::IsUnicodeWhitespace(c)) {
      continue;
    }
    return base::IsAsciiDigit(c) || c == u')';
  }
  return false;
}

// Every operator must be binary, because a leading sign on a bare number is
// how phone numbers are written: "+15551234567" is not a calculation, and it
// is exactly the kind of input this path exists to keep off the network.
//
// '-' and '/' additionally need spaces around them, since they also delimit
// card numbers, SSNs and dates -- otherwise "1234-5678-9012-3456" would be
// answered as "= -13912".
//
// This only decides whether evaluating is worth attempting; the parser is what
// decides whether the text is actually an expression.
bool LooksLikeArithmetic(std::u16string_view text) {
  for (size_t i = 0; i < text.size(); ++i) {
    const char16_t c = text[i];
    if (c == u'+' || c == u'*' || c == u'^') {
      if (HasLeftOperand(text, i)) {
        return true;
      }
    }
    if ((c == u'-' || c == u'/') && i > 0 && i + 1 < text.size() &&
        base::IsUnicodeWhitespace(text[i - 1]) &&
        base::IsUnicodeWhitespace(text[i + 1]) && HasLeftOperand(text, i)) {
      return true;
    }
  }

  return false;
}

std::optional<std::u16string> Format(const Rational& value) {
  if (value.den == 1) {
    // Exact across the whole int64 range; no double involved.
    return base::FormatNumber(value.num);
  }

  // A fraction can only be shown if its decimal expansion terminates, i.e. the
  // reduced denominator is 2^twos * 5^fives.
  int64_t remaining = value.den;
  int twos = 0;
  int fives = 0;
  while (remaining % 2 == 0) {
    remaining /= 2;
    ++twos;
  }
  while (remaining % 5 == 0) {
    remaining /= 5;
    ++fives;
  }
  if (remaining != 1) {
    return std::nullopt;  // e.g. 10/3.
  }

  const int fraction_digits = std::max(twos, fives);
  int integer_digits = 1;
  for (int64_t whole = value.num / value.den; whole >= 10 || whole <= -10;
       whole /= 10) {
    ++integer_digits;
  }
  if (integer_digits + fraction_digits > kMaxSignificantDigits) {
    return std::nullopt;
  }

  return base::FormatDouble(static_cast<double>(value.num) / value.den,
                            /*min_fractional_digits=*/0, fraction_digits);
}

}  // namespace

std::optional<std::u16string> EvaluateArithmeticExpression(
    std::u16string_view text) {
  if (!LooksLikeArithmetic(text)) {
    return std::nullopt;
  }
  auto value = Parser(text).Parse();
  if (!value) {
    return std::nullopt;
  }
  return Format(*value);
}

}  // namespace omnibox
