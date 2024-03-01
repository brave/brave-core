// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import DesignSystem
struct AIChatPageContextView<InfoView>: View where InfoView: View {
  @State
  private var shouldShowInformationPopover = false
  
  @Binding
  var isToggleOn: Bool
  
  @ViewBuilder
  var infoContent: (() -> InfoView)
  
  var body: some View {
    Toggle(isOn: $isToggleOn, label: {
      HStack {
        Text(Strings.AIChat.infoAboutPageContext)
          .font(.footnote)
          .foregroundStyle(Color(braveSystemName: .textTertiary))
        
        Button(action: {
          shouldShowInformationPopover = true
        }, label: {
          Label {
            Text(Strings.AIChat.leoPageContextInfoDescriptionTitle)
          } icon: {
            Image(braveSystemName: "leo.info.outline")
              .foregroundStyle(Color(braveSystemName: .iconDefault))
              .font(.footnote)
          }
          .labelStyle(.iconOnly)
        })
        .popover(isPresented: $shouldShowInformationPopover) {
          infoContent()
        }
      }
    })
    .tint(Color(braveSystemName: isToggleOn ? .primary60 : .gray30))
    .padding([.vertical, .trailing], 8.0)
    .padding(.leading, 12.0)
    .background(
      RoundedRectangle(cornerRadius: 8.0, style: .continuous)
        .fill(Color(braveSystemName: .pageBackground))
        .shadow(color: .black.opacity(0.15), radius: 4.0, x: 0.0, y: 1.0)
    )
  }
}

#if DEBUG
struct AIChatPageContextView_Preview: PreviewProvider {
  static var previews: some View {
    AIChatPageContextView(isToggleOn: .constant(true)) {}
      .previewLayout(.sizeThatFits)
  }
}
#endif
