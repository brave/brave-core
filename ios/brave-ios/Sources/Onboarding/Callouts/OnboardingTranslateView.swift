// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import DesignSystem
import Foundation
import Strings
import SwiftUI

public struct OnboardingTranslateView: View {

  public init() {}

  public var body: some View {
    ScrollView(.vertical) {
      VStack(spacing: 16.0) {
        Image("translate-onboarding-icon", bundle: .module)

        Text(Strings.BraveTranslateOnboarding.translateTitle)
          .font(.callout.weight(.semibold))
          .foregroundColor(Color(braveSystemName: .textPrimary))

        Text(Strings.BraveTranslateOnboarding.translateDescription)
          .font(.callout)
          .foregroundColor(Color(braveSystemName: .textSecondary))
      }
      .padding(24.0)
    }
    .multilineTextAlignment(.center)
    .scrollBounceBehavior(.basedOnSize)
  }
}

extension OnboardingTranslateView: PopoverContentComponent {
  public var popoverBackgroundColor: UIColor {
    .braveBackground
  }
}

#if DEBUG
struct OnboardingTranslateView_PreviewProvider: PreviewProvider {
  static var previews: some View {
    OnboardingTranslateView()
      .fixedSize(horizontal: false, vertical: true)
      .previewLayout(.sizeThatFits)
  }
}
#endif
