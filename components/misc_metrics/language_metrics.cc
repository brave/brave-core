// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/misc_metrics/language_metrics.h"

#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_split.h"
#include "components/language/core/browser/pref_names.h"
#include "components/language/core/common/locale_util.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

struct LanguageSynonymPair {
  const char* deprecated_code;
  const char* active_code;
};

constexpr auto kOrderedLangCodes = base::MakeFixedFlatSet<std::string_view>(
    base::sorted_unique,
    {
        "aa",  // Afar
        "ab",  // Abkhazian
        "ae",  // Avestan
        "af",  // Afrikaans
        "ak",  // Akan
        "am",  // Amharic
        "an",  // Aragonese
        "ar",  // Arabic
        "as",  // Assamese
        "av",  // Avaric
        "ay",  // Aymara
        "az",  // Azerbaijani
        "ba",  // Bashkir
        "be",  // Belarusian
        "bg",  // Bulgarian
        "bh",  // Bihari languages
        "bi",  // Bislama
        "bm",  // Bambara
        "bn",  // Bengali
        "bo",  // Tibetan
        "br",  // Breton
        "bs",  // Bosnian
        "ca",  // Catalan; Valencian
        "ce",  // Chechen
        "ch",  // Chamorro
        "co",  // Corsican
        "cr",  // Cree
        "cs",  // Czech
        "cu",  // Church Slavic; Old Slavonic; Church Slavonic; Old Bulgarian;
               // Old Church Slavonic
        "cv",  // Chuvash
        "cy",  // Welsh
        "da",  // Danish
        "de",  // German
        "dv",  // Divehi; Dhivehi; Maldivian
        "dz",  // Dzongkha
        "ee",  // Ewe
        "el",  // "Greek, Modern (1453-)"
        "en",  // English
        "eo",  // Esperanto
        "es",  // Spanish; Castilian
        "et",  // Estonian
        "eu",  // Basque
        "fa",  // Persian
        "ff",  // Fulah
        "fi",  // Finnish
        "fj",  // Fijian
        "fo",  // Faroese
        "fr",  // French
        "fy",  // Western Frisian
        "ga",  // Irish
        "gd",  // Gaelic; Scottish Gaelic
        "gl",  // Galician
        "gn",  // Guarani
        "gu",  // Gujarati
        "gv",  // Manx
        "ha",  // Hausa
        "he",  // Hebrew
        "hi",  // Hindi
        "ho",  // Hiri Motu
        "hr",  // Croatian
        "ht",  // Haitian; Haitian Creole
        "hu",  // Hungarian
        "hy",  // Armenian
        "hz",  // Herero
        "ia",  // Interlingua (International Auxiliary Language Association)
        "id",  // Indonesian
        "ie",  // Interlingue; Occidental
        "ig",  // Igbo
        "ii",  // Sichuan Yi; Nuosu
        "ik",  // Inupiaq
        "io",  // Ido
        "is",  // Icelandic
        "it",  // Italian
        "iu",  // Inuktitut
        "ja",  // Japanese
        "jv",  // Javanese
        "ka",  // Georgian
        "kg",  // Kongo
        "ki",  // Kikuyu; Gikuyu
        "kj",  // Kuanyama; Kwanyama
        "kk",  // Kazakh
        "kl",  // Kalaallisut; Greenlandic
        "km",  // Central Khmer
        "kn",  // Kannada
        "ko",  // Korean
        "kr",  // Kanuri
        "ks",  // Kashmiri
        "ku",  // Kurdish
        "kv",  // Komi
        "kw",  // Cornish
        "ky",  // Kirghiz; Kyrgyz
        "la",  // Latin
        "lb",  // Luxembourgish; Letzeburgesch
        "lg",  // Ganda
        "li",  // Limburgan; Limburger; Limburgish
        "ln",  // Lingala
        "lo",  // Lao
        "lt",  // Lithuanian
        "lu",  // Luba-Katanga
        "lv",  // Latvian
        "mg",  // Malagasy
        "mh",  // Marshallese
        "mi",  // Maori
        "mk",  // Macedonian
        "ml",  // Malayalam
        "mn",  // Mongolian
        "mr",  // Marathi
        "ms",  // Malay
        "mt",  // Maltese
        "my",  // Burmese
        "na",  // Nauru
        "nb",  // "Bokmål, Norwegian; Norwegian Bokmål"
        "nd",  // "Ndebele, North; North Ndebele"
        "ne",  // Nepali
        "ng",  // Ndonga
        "nl",  // Dutch; Flemish
        "nn",  // "Norwegian Nynorsk; Nynorsk, Norwegian"
        "no",  // Norwegian
        "nr",  // "Ndebele, South; South Ndebele"
        "nv",  // Navajo; Navaho
        "ny",  // Chichewa; Chewa; Nyanja
        "oc",  // Occitan (post 1500)
        "oj",  // Ojibwa
        "om",  // Oromo
        "or",  // Oriya
        "os",  // Ossetian; Ossetic
        "pa",  // Panjabi; Punjabi
        "pi",  // Pali
        "pl",  // Polish
        "ps",  // Pushto; Pashto
        "pt",  // Portuguese
        "qu",  // Quechua
        "rm",  // Romansh
        "rn",  // Rundi
        "ro",  // Romanian; Moldavian; Moldovan
        "ru",  // Russian
        "rw",  // Kinyarwanda
        "sa",  // Sanskrit
        "sc",  // Sardinian
        "sd",  // Sindhi
        "se",  // Northern Sami
        "sg",  // Sango
        "si",  // Sinhala; Sinhalese
        "sk",  // Slovak
        "sl",  // Slovenian
        "sm",  // Samoan
        "sn",  // Shona
        "so",  // Somali
        "sq",  // Albanian
        "sr",  // Serbian
        "ss",  // Swati
        "st",  // "Sotho, Southern"
        "su",  // Sundanese
        "sv",  // Swedish
        "sw",  // Swahili
        "ta",  // Tamil
        "te",  // Telugu
        "tg",  // Tajik
        "th",  // Thai
        "ti",  // Tigrinya
        "tk",  // Turkmen
        "tl",  // Tagalog
        "tn",  // Tswana
        "to",  // Tonga (Tonga Islands)
        "tr",  // Turkish
        "ts",  // Tsonga
        "tt",  // Tatar
        "tw",  // Twi
        "ty",  // Tahitian
        "ug",  // Uighur; Uyghur
        "uk",  // Ukrainian
        "ur",  // Urdu
        "uz",  // Uzbek
        "ve",  // Venda
        "vi",  // Vietnamese
        "vo",  // Volapük
        "wa",  // Walloon
        "wo",  // Wolof
        "xh",  // Xhosa
        "yi",  // Yiddish
        "yo",  // Yoruba
        "za",  // Zhuang; Chuang
        "zh",  // Chinese
        "zu",  // Zulu
    });

constexpr LanguageSynonymPair kLanguageSynonyms[] = {
    {"in", "id"}, {"iw", "he"}, {"ji", "yi"},
    {"jw", "jv"}, {"mo", "ro"}, {"gsw", "de"}};

LanguageMetrics::LanguageMetrics(PrefService* profile_prefs)
    : profile_prefs_(profile_prefs) {
  pref_change_registrar_.Init(profile_prefs);
  pref_change_registrar_.Add(
      language::prefs::kAcceptLanguages,
      base::BindRepeating(&LanguageMetrics::RecordLanguageMetric,
                          base::Unretained(this)));

  RecordLanguageMetric();
}

LanguageMetrics::~LanguageMetrics() = default;

void LanguageMetrics::RecordLanguageMetric() {
  auto languages = profile_prefs_->GetString(language::prefs::kAcceptLanguages);
  auto languages_split = base::SplitString(
      languages, ",", base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);
  if (languages_split.empty()) {
    // Suspend metric if primary language not detected
    UMA_HISTOGRAM_EXACT_LINEAR(kPrimaryLanguageHistogramName, INT_MAX - 1,
                               kOrderedLangCodes.size());
    return;
  }

  auto primary_language = language::ExtractBaseLanguage(languages_split[0]);
  for (const auto& synonym_pair : kLanguageSynonyms) {
    if (primary_language == synonym_pair.deprecated_code) {
      primary_language = synonym_pair.active_code;
    }
  }

  // set answer to fallback which will suspend the metric
  // if the language is not found in the list
  int answer = INT_MAX - 1;

  const auto language_it = kOrderedLangCodes.find(primary_language);
  if (language_it != kOrderedLangCodes.end()) {
    answer = std::distance(kOrderedLangCodes.begin(), language_it);
  }

  UMA_HISTOGRAM_EXACT_LINEAR(kPrimaryLanguageHistogramName, answer,
                             kOrderedLangCodes.size());
}

}  // namespace misc_metrics
