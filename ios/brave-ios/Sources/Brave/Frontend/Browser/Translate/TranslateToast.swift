// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import SwiftUI

struct TranslateToast: View {
  @Environment(\.dismiss)
  private var dismiss

  var languageInfo: BraveTranslateLanguageInfo
  var presentSettings: (() -> Void)?
  var revertTranslation: (() -> Void)?

  var currentLanguageName: String {
    if let languageCode = languageInfo.currentLanguage.languageCode?.identifier,
      let languageName = Locale.current.localizedString(forLanguageCode: languageCode)
    {
      return languageName
    }
    return "Unknown Language"
  }

  var pageLanguageName: String {
    if let languageCode = languageInfo.pageLanguage?.languageCode?.identifier,
      let languageName = Locale.current.localizedString(forLanguageCode: languageCode)
    {
      return languageName
    }
    return "Unknown Language"
  }

  var body: some View {
    HStack {
      Image(braveSystemName: "leo.product.translate")
        .symbolRenderingMode(.monochrome)
        .foregroundStyle(
          LinearGradient(
            braveGradient: .init(
              stops: [
                .init(color: UIColor(rgb: 0xFA7250), position: 0.0),
                .init(color: UIColor(rgb: 0xFF1893), position: 0.43),
                .init(color: UIColor(rgb: 0xA78AFF), position: 1.0),
              ],
              angle: .figmaDegrees(314.42)
            )
          )
        )
        .padding(.trailing)
      VStack {
        Text("Page Translations")
          .font(.callout.weight(.semibold))
          .foregroundColor(Color(braveSystemName: .textPrimary))

        Text(
          "\(pageLanguageName) To \(currentLanguageName)"
        )
        .font(.callout)
        .foregroundColor(Color(braveSystemName: .textSecondary))
      }
      .padding(.trailing)

      Spacer()

      Button {
        dismiss()
        presentSettings?()
      } label: {
        Image(braveSystemName: "leo.settings")
          .foregroundStyle(Color(braveSystemName: .iconDefault))
      }

      Color(braveSystemName: .dividerSubtle)
        .frame(width: 1.0)
        .padding([.top, .bottom], 8.0)
        .padding([.leading, .trailing])

      Button {
        dismiss()
        revertTranslation?()
      } label: {
        Text("Undo")
          .foregroundStyle(Color(braveSystemName: .textInteractive))
      }
    }
    .padding()
  }
}

extension TranslateToast: PopoverContentComponent {
  public var popoverBackgroundColor: UIColor {
    .braveBackground
  }
}

#Preview {
  TranslateToast(languageInfo: .init())
}
