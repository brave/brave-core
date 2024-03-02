// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import DesignSystem

public struct AIChatTermsAndConditionsView: View {
  var onTermsAccepted: () -> Void
  var onOpenURL: (URL) -> Void
  
  @Environment(\.dismiss)
  private var dismiss
  
  public init(onTermsAccepted: @escaping () -> Void, onOpenURL: @escaping (URL) -> Void) {
    self.onTermsAccepted = onTermsAccepted
    self.onOpenURL = onOpenURL
  }
  
  public var body: some View {
    VStack(spacing: 16.0) {
      Text(Strings.AIChat.termsConditionsTitle)
        .multilineTextAlignment(.leading)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
        .font(.body.weight(.semibold))
        .foregroundStyle(Color(braveSystemName: .textPrimary))
      
      Text(LocalizedStringKey(String.localizedStringWithFormat(
        Strings.AIChat.termsConditionsDescription,
        AIChatConstants.braveLeoWikiURL.absoluteString,
        AIChatConstants.braveLeoPrivacyPolicyURL.absoluteString)))
        .multilineTextAlignment(.leading)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .tint(Color(braveSystemName: .primary50))
        .environment(\.openURL, OpenURLAction { url in
          dismiss()
          onOpenURL(url)
          return .handled
        })
      
      Button(action: {
        onTermsAccepted()
      }) {
        Text(Strings.AIChat.termsConditionsApprovalActionTitle)
          .font(.subheadline.weight(.semibold))
          .padding([.top, .bottom], 12)
          .padding([.leading, .trailing], 16)
          .frame(maxWidth: .infinity)
          .foregroundStyle(.white)
          .background(
            Color(braveSystemName: .buttonBackground),
            in: Capsule()
          )
      }
      .buttonStyle(.plain)
      .padding(16.0)
    }
  }
}

#if DEBUG
struct AIChatTermsAndConditionsView_Preview: PreviewProvider {
  static var previews: some View {
    AIChatTermsAndConditionsView() {} onOpenURL: { _ in }
      .previewLayout(.sizeThatFits)
  }
}
#endif
