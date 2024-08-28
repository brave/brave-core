// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Foundation
import Strings
import SwiftUI

public struct OnboardingTranslateView: View {
  @Environment(\.dismiss) private var dismiss

  private var onContinueButtonPressed: (() -> Void)?
  private var onDisableFeature: (() -> Void)?

  public init(onContinueButtonPressed: (() -> Void)? = nil, onDisableFeature: (() -> Void)? = nil) {
    self.onContinueButtonPressed = onContinueButtonPressed
    self.onDisableFeature = onDisableFeature
  }

  public var body: some View {
    ScrollView(.vertical) {
      VStack(spacing: 0.0) {
        Image(braveSystemName: "leo.product.translate")
          .font(.title)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .padding(16.0)
          .background(
            ContainerRelativeShape()
              .fill(Color.white)
          )
          .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
          .padding([.top, .bottom], 32.0)

        Text("Page Translations")
          .font(.callout.weight(.semibold))
          .foregroundColor(Color.white)
          .padding(.bottom)

        Text(
          "Pages can be translated to languages supported by your iOS device. You may be required to configure languages if this is your first time using this feature."
        )
        .font(.callout)
        .foregroundColor(Color.white)
        .padding(.bottom, 24.0)

        Button(
          action: {
            dismiss()
            onContinueButtonPressed?()
          },
          label: {
            Text("Continue")
              .font(.body.weight(.semibold))
              .padding()
              .frame(maxWidth: .infinity)
              .foregroundStyle(Color(braveSystemName: .schemesOnPrimary))
              .background(
                ContainerRelativeShape()
                  .fill(Color(braveSystemName: .primary40))
              )
              .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
          }
        )
        .buttonStyle(.plain)
        .padding(.bottom)

        Button(
          action: {
            dismiss()
            onDisableFeature?()
          },
          label: {
            Text("Disable This Feature")
              .font(.body.weight(.semibold))
              .padding()
              .frame(maxWidth: .infinity)
              .foregroundStyle(Color(braveSystemName: .schemesOnPrimary))
              .background(
                ContainerRelativeShape()
                  .fill(Color(.braveBlurpleTint))
              )
              .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
          }
        )
        .buttonStyle(.plain)
      }
    }
    .padding()
    .frame(
      minWidth: 200,
      maxWidth: BraveUX.baseDimensionValue
    )
    .background(Color(.braveBlurpleTint))
    .multilineTextAlignment(.center)
    .osAvailabilityModifiers { content in
      #if compiler(>=5.8)
      if #available(iOS 16.4, *) {
        content
          .scrollBounceBehavior(.basedOnSize)
      } else {
        content
          .introspectScrollView { scrollView in
            scrollView.alwaysBounceVertical = false
          }
      }
      #else
      content
        .introspectScrollView { scrollView in
          scrollView.alwaysBounceVertical = false
        }
      #endif
    }
  }
}

extension OnboardingTranslateView: PopoverContentComponent {
  public var popoverBackgroundColor: UIColor {
    .braveBlurpleTint
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
