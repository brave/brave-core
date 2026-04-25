// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Foundation
import Strings
import SwiftUI

public struct CredentialProviderOnboardingView: View {
  public var action: () -> Void
  public init(action: @escaping () -> Void) {
    self.action = action
  }
  public var body: some View {
    VStack(spacing: 40) {
      Image(sharedName: "brave.logo")
        .resizable()
        .aspectRatio(contentMode: .fit)
        .frame(height: 120)
      VStack {
        VStack(spacing: 12) {
          Text(Strings.CredentialProvider.onboardingViewTitle)
            .font(.title.bold())
            .foregroundColor(Color(braveSystemName: .textPrimary))
          Text(Strings.CredentialProvider.onboardingViewSubtitle)
            .font(.subheadline)
            .foregroundColor(Color(braveSystemName: .textSecondary))
          Text(Strings.CredentialProvider.onboardingViewFootnote)
            .font(.footnote)
            .foregroundColor(Color(braveSystemName: .textTertiary))
        }
        .multilineTextAlignment(.center)
        .padding(.horizontal)
        Spacer()
        Button {
          action()
        } label: {
          Text(Strings.CredentialProvider.onboardingViewContinueCTA)
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
      }
    }
    .padding(.horizontal, 20)
    .padding(.vertical, 40)
    .background(Color(braveSystemName: .containerBackground))
  }
}

#if DEBUG
#Preview {
  CredentialProviderOnboardingView(action: {})
}
#endif
