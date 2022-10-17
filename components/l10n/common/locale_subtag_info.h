/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_L10N_COMMON_LOCALE_SUBTAG_INFO_H_
#define BRAVE_COMPONENTS_L10N_COMMON_LOCALE_SUBTAG_INFO_H_

#include <string>

namespace brave_l10n {

struct LocaleSubtagInfo final {
  LocaleSubtagInfo();

  LocaleSubtagInfo(const LocaleSubtagInfo& other);
  LocaleSubtagInfo& operator=(const LocaleSubtagInfo& other);

  LocaleSubtagInfo(LocaleSubtagInfo&& other) noexcept;
  LocaleSubtagInfo& operator=(LocaleSubtagInfo&& other) noexcept;

  ~LocaleSubtagInfo();

  // Language is a lowercase two-letter ISO 639-1 language code. For example,
  // Spanish is "es", English is "en" and French is "fr". See
  // https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes.
  std::string language;

  // Script is an optional sentence case four-letter script code which follows
  // the language code. If specified, it should be a valid script code. For
  // example, "en_Latn_US". See https://en.wikipedia.org/wiki/ISO_15924.
  std::string script;

  // Country is an uppercase two-letter ISO 3166-1 alpha-2 country code or UN
  // M.49 code. For example, "ES" represents Spain, "MX" represents Mexico, and
  // "001" represents the World. See
  // https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2 and
  // https://en.wikipedia.org/wiki/UN_M49.
  std::string country;

  // Charset specifier is optional. For example, "en_US.UTF-8".
  std::string charset;

  // Variant is optional and indicates additional, well-recognized variations
  // that define a language or its dialects that are not covered by other
  // available subtags. For example, "en_IE@currency=IEP".
  std::string variant;
};

bool operator==(const LocaleSubtagInfo& lhs, const LocaleSubtagInfo& rhs);
bool operator!=(const LocaleSubtagInfo& lhs, const LocaleSubtagInfo& rhs);

}  // namespace brave_l10n

#endif  // BRAVE_COMPONENTS_L10N_COMMON_LOCALE_SUBTAG_INFO_H_
