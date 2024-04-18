// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import SwiftUI

struct AIChatPageContextView: View {
  @State
  private var shouldShowInformationPopover = false

  @Binding
  var isToggleOn: Bool

  var url: URL?

  var pageTitle: String

  var body: some View {
    Toggle(
      isOn: $isToggleOn,
      label: {
        HStack {
          Text(Strings.AIChat.infoAboutPageContext)
            .font(.footnote)
            .foregroundStyle(Color(braveSystemName: .textTertiary))
            .fixedSize(horizontal: false, vertical: true)

          Button(
            action: {
              shouldShowInformationPopover = true
            },
            label: {
              Label {
                Text(Strings.AIChat.leoPageContextInfoDescriptionTitle)
              } icon: {
                Image(braveSystemName: "leo.info.outline")
                  .foregroundStyle(Color(braveSystemName: .iconDefault))
                  .font(.footnote)
              }
              .labelStyle(.iconOnly)
            }
          )
          .bravePopover(isPresented: $shouldShowInformationPopover) {
            PopoverWrapperView(
              backgroundColor: UIColor(braveSystemName: .containerBackground)
            ) {
              AIChatPageContextInfoView(url: url, pageTitle: pageTitle)
                .background(Color(braveSystemName: .containerBackground))
                .frame(maxWidth: 400.0)
            }
          }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    )
    .tint(Color(braveSystemName: isToggleOn ? .primitivePrimary60 : .primitiveGray30))
    .padding([.vertical, .trailing], 8.0)
    .padding(.leading, 12.0)
    .background(
      RoundedRectangle(cornerRadius: 8.0, style: .continuous)
        .fill(Color(braveSystemName: .pageBackground))
        .shadow(color: .black.opacity(0.10), radius: 4.0, x: 0.0, y: 1.0)
    )
  }
}

#if DEBUG
struct AIChatPageContextView_Preview: PreviewProvider {
  static var previews: some View {
    AIChatPageContextView(
      isToggleOn: .constant(true),
      url: URL(string: "https://brave.com"),
      pageTitle: "Brave Private Browser"
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
