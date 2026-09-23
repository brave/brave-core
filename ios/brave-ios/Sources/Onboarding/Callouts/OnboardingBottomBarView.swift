// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import BraveUI
import Shared
import SwiftUI

public struct OnboardingBottomBarView: View {
  public var dismiss: (() -> Void)?
  public var switchBottomBar: (() -> Void)?

  public init() {}

  public var body: some View {
    VStack {
      Button {
        dismiss?()
      } label: {
        Image(braveSystemName: "leo.close")
          .renderingMode(.template)
          .foregroundColor(Color(braveSystemName: .textPrimary))
      }
      .frame(maxWidth: .infinity, alignment: .trailing)
      VStack(spacing: 24) {
        Image("bottom-bar-logo-bottom", bundle: .module)
          .aspectRatio(contentMode: .fit)
        Text(Strings.Callout.bottomBarCalloutTitle)
          .font(.title2.weight(.semibold))
          .foregroundColor(Color(braveSystemName: .textPrimary))
          .multilineTextAlignment(.center)
        Text(Strings.Callout.bottomBarCalloutDescription)
          .font(.body)
          .multilineTextAlignment(.center)
          .foregroundColor(Color(braveSystemName: .textPrimary))
          .padding(.horizontal, 16)
      }
      .padding(.bottom, 16)
      Button {
        switchBottomBar?()
      } label: {
        Text(Strings.Callout.bottomBarCalloutButtonTitle)
          .frame(maxWidth: .infinity, maxHeight: .infinity)
          .font(.title3.weight(.medium))
          .padding()
      }
      .frame(height: 44)
      .background(Color(braveSystemName: .buttonBackground))
      .accentColor(Color(braveSystemName: .schemesOnPrimary))
      .clipShape(Capsule())
      .padding(.horizontal, 16)
      Button {
        dismiss?()
      } label: {
        Text(Strings.Callout.bottomBarCalloutDismissButtonTitle)
          .frame(maxWidth: .infinity, maxHeight: .infinity)
          .font(.title3.weight(.medium))
          .foregroundColor(Color(braveSystemName: .textPrimary))
      }
      .frame(height: 44)
      .background(Color(.clear))
      .accentColor(Color(.white))
    }
    .frame(maxWidth: BraveUX.baseDimensionValue)
    .padding()
    .background(Color(braveSystemName: .containerBackground))
    .accessibilityEmbedInScrollView()
  }
}

#if DEBUG
struct OnboardingBottomBarView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      BraveUI.PopupView {
        OnboardingBottomBarView()
      }
      .previewDevice("iPhone 12 Pro")

      BraveUI.PopupView {
        OnboardingBottomBarView()
      }
      .previewDevice("iPad Pro (9.7-inch)")
    }
  }
}
#endif
