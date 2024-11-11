// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import BraveUI
import DesignSystem
import SwiftUI

private struct TranslationOptionsView: View {
  @Environment(\.dismiss)
  private var dismiss

  @State
  private var searchText = ""

  @Binding
  var language: Locale.Language?

  var body: some View {
    NavigationStack {
      List {
        ForEach(filteredLanguages, id: \.self) { language in
          Button(
            action: {
              self.language = language
              dismiss()
            },
            label: {
              Text(languageName(for: language))
            }
          )
        }
      }
      .listStyle(.plain)
      .navigationTitle(Strings.BraveTranslate.languageSelectionButtonTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItem(placement: .navigationBarLeading) {
          Button(Strings.CancelString) {
            dismiss()
          }
        }
      }
    }
    .searchable(
      text: $searchText,
      placement: .navigationBarDrawer(displayMode: .always),
      prompt: Strings.BraveTranslate.searchInputTitle
    )
  }

  private var filteredLanguages: [Locale.Language] {
    return searchText.isEmpty
      ? languages
      : languages.filter({
        languageName(for: $0).lowercased().contains(searchText.lowercased())
      })
  }

  // TODO: Take from Brave-Core's list
  // TODO: Take from Apple's list
  private var languages: [Locale.Language] {
    return [
      "af",  // Afrikaans
      "ak",  // Twi
      "am",  // Amharic
      "ar",  // Arabic
      "as",  // Assamese
      "ay",  // Aymara
      "az",  // Azerbaijani
      "be",  // Belarusian
      "bg",  // Bulgarian
      "bho",  // Bhojpuri
      "bm",  // Bambara
      "bn",  // Bengali
      "bs",  // Bosnian
      "ca",  // Catalan
      "ceb",  // Cebuano
      "ckb",  // Kurdish (Sorani)
      "co",  // Corsican
      "cs",  // Czech
      "cy",  // Welsh
      "da",  // Danish
      "de",  // German
      "doi",  // Dogri
      "dv",  // Dhivehi
      "ee",  // Ewe
      "el",  // Greek
      "en",  // English
      "eo",  // Esperanto
      "es",  // Spanish
      "et",  // Estonian
      "eu",  // Basque
      "fa",  // Persian
      "fi",  // Finnish
      "fr",  // French
      "fy",  // Frisian
      "ga",  // Irish
      "gd",  // Scots Gaelic
      "gl",  // Galician
      "gom",  // Konkani
      "gu",  // Gujarati
      "ha",  // Hausa
      "haw",  // Hawaiian
      "hi",  // Hindi
      "hmn",  // Hmong
      "hr",  // Croatian
      "ht",  // Haitian Creole
      "hu",  // Hungarian
      "hy",  // Armenian
      "id",  // Indonesian
      "ig",  // Igbo
      "ilo",  // Ilocano
      "is",  // Icelandic
      "it",  // Italian
      "iw",  // Hebrew - Chrome uses "he"
      "ja",  // Japanese
      "jw",  // Javanese - Chrome uses "jv"
      "ka",  // Georgian
      "kk",  // Kazakh
      "km",  // Khmer
      "kn",  // Kannada
      "ko",  // Korean
      "kri",  // Krio
      "ku",  // Kurdish
      "ky",  // Kyrgyz
      "la",  // Latin
      "lb",  // Luxembourgish
      "lg",  // Luganda
      "ln",  // Lingala
      "lo",  // Lao
      "lt",  // Lithuanian
      "lus",  // Mizo
      "lv",  // Latvian
      "mai",  // Maithili
      "mg",  // Malagasy
      "mi",  // Maori
      "mk",  // Macedonian
      "ml",  // Malayalam
      "mn",  // Mongolian
      "mni-Mtei",  // Manipuri (Meitei Mayek)
      "mr",  // Marathi
      "ms",  // Malay
      "mt",  // Maltese
      "my",  // Burmese
      "ne",  // Nepali
      "nl",  // Dutch
      "no",  // Norwegian - Chrome uses "nb"
      "nso",  // Sepedi
      "ny",  // Nyanja
      "om",  // Oromo
      "or",  // Odia (Oriya)
      "pa",  // Punjabi
      "pl",  // Polish
      "ps",  // Pashto
      "pt",  // Portuguese
      "qu",  // Quechua
      "ro",  // Romanian
      "ru",  // Russian
      "rw",  // Kinyarwanda
      "sa",  // Sanskrit
      "sd",  // Sindhi
      "si",  // Sinhala
      "sk",  // Slovak
      "sl",  // Slovenian
      "sm",  // Samoan
      "sn",  // Shona
      "so",  // Somali
      "sq",  // Albanian
      "sr",  // Serbian
      "st",  // Southern Sotho
      "su",  // Sundanese
      "sv",  // Swedish
      "sw",  // Swahili
      "ta",  // Tamil
      "te",  // Telugu
      "tg",  // Tajik
      "th",  // Thai
      "ti",  // Tigrinya
      "tk",  // Turkmen
      "tl",  // Tagalog - Chrome uses "fil"
      "tr",  // Turkish
      "ts",  // Tsonga
      "tt",  // Tatar
      "ug",  // Uyghur
      "uk",  // Ukrainian
      "ur",  // Urdu
      "uz",  // Uzbek
      "vi",  // Vietnamese
      "xh",  // Xhosa
      "yi",  // Yiddish
      "yo",  // Yoruba
      "zh-CN",  // Chinese (Simplified)
      "zh-TW",  // Chinese (Traditional)
      "zu",  // Zulu
    ].map({
      Locale.Language.init(identifier: $0)
    })
  }

  private func languageName(for language: Locale.Language) -> String {
    if let languageCode = language.languageCode?.identifier,
      let languageName = Locale.current.localizedString(forLanguageCode: languageCode)
    {
      return languageName
    }
    return "Unknown Language"
  }
}

struct TranslateToast: View {
  @Environment(\.dismiss)
  private var dismiss

  @State
  private var showSourceLanguageSelection: Bool = false

  @State
  private var showTargetLanguageSelection: Bool = false

  @ObservedObject
  var languageInfo: BraveTranslateLanguageInfo

  var onLanguageSelectionChanged: ((BraveTranslateLanguageInfo) -> Void)?

  var body: some View {
    HStack {
      Image(braveSystemName: "leo.product.translate")
        .symbolRenderingMode(.monochrome)
        .foregroundStyle(
          LinearGradient(braveSystemName: .iconsActive)
        )
        .padding(.trailing)
      VStack(alignment: .leading) {
        Text(Strings.BraveTranslate.pageTranslatedTitle)
          .font(.callout.weight(.semibold))
          .foregroundColor(Color(braveSystemName: .textPrimary))
          .padding(.bottom, 8.0)

        HStack {
          Button {
            showSourceLanguageSelection = true
          } label: {
            Text(
              "\(pageLanguageName)"
            )
            .font(.callout)
            .foregroundColor(Color(braveSystemName: .textSecondary))
            .padding([.top, .bottom], 4.0)
            .padding([.leading, .trailing], 8.0)
            .background(
              ContainerRelativeShape()
                .fill(Color(braveSystemName: .pageBackground))
            )
            .containerShape(RoundedRectangle(cornerRadius: 4.0, style: .continuous))
          }

          Text(Strings.BraveTranslate.translateFromToTitle)
            .font(.callout)
            .foregroundColor(Color(braveSystemName: .textSecondary))

          Button {
            showTargetLanguageSelection = true
          } label: {
            Text(
              "\(currentLanguageName)"
            )
            .font(.callout)
            .foregroundColor(Color(braveSystemName: .textSecondary))
            .padding([.top, .bottom], 4.0)
            .padding([.leading, .trailing], 8.0)
            .background(
              ContainerRelativeShape()
                .fill(Color(braveSystemName: .pageBackground))
            )
            .containerShape(RoundedRectangle(cornerRadius: 4.0, style: .continuous))
          }
        }
      }
    }
    .padding()
    .frame(alignment: .leading)
    .bravePopup(isPresented: $showSourceLanguageSelection) {
      TranslationOptionsView(
        language: Binding(
          get: {
            languageInfo.pageLanguage
          },
          set: {
            languageInfo.pageLanguage = $0
            onLanguageSelectionChanged?(languageInfo)
          }
        )
      )
      .onDisappear {
        showTargetLanguageSelection = false
      }
    }
    .bravePopup(isPresented: $showTargetLanguageSelection) {
      TranslationOptionsView(
        language: Binding(
          get: {
            languageInfo.currentLanguage
          },
          set: {
            languageInfo.currentLanguage = $0 ?? Locale.current.language
            onLanguageSelectionChanged?(languageInfo)
          }
        )
      )
      .onDisappear {
        showSourceLanguageSelection = false
      }
    }
  }

  private var currentLanguageName: String {
    if let languageCode = languageInfo.currentLanguage.languageCode?.identifier,
      let languageName = Locale.current.localizedString(forLanguageCode: languageCode)
    {
      return languageName
    }
    return Strings.BraveTranslate.unknownLanguageTitle
  }

  private var pageLanguageName: String {
    if let languageCode = languageInfo.pageLanguage?.languageCode?.identifier,
      let languageName = Locale.current.localizedString(forLanguageCode: languageCode)
    {
      return languageName
    }
    return Strings.BraveTranslate.unknownLanguageTitle
  }
}

extension TranslateToast: PopoverContentComponent {
  public var popoverBackgroundColor: UIColor {
    .braveBackground
  }
}

#Preview {
  TranslateToast(languageInfo: .init(), onLanguageSelectionChanged: nil)
}
