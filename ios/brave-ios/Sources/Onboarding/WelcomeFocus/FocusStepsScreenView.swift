// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Growth
import Preferences
import SwiftUI

struct FocusStepsView: View {
  var namespace: Namespace.ID

  @Environment(\.colorScheme) private var colorScheme
  @Environment(\.verticalSizeClass) private var verticalSizeClass: UserInterfaceSizeClass?
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass: UserInterfaceSizeClass?
  @Environment(\.dismiss) private var dismiss

  @ScaledMetric private var iconSize: CGFloat = 32.0

  @State private var isP3AViewPresented = false
  @Binding var shouldDismiss: Bool

  private let attributionManager: AttributionManager?
  private let p3aUtilities: BraveP3AUtils?

  private var shouldUseExtendedDesign: Bool {
    return horizontalSizeClass == .regular && verticalSizeClass == .regular
  }

  private let dynamicTypeRange = (...DynamicTypeSize.xLarge)

  public init(
    namespace: Namespace.ID,
    attributionManager: AttributionManager? = nil,
    p3aUtilities: BraveP3AUtils? = nil,
    shouldDismiss: Binding<Bool>
  ) {
    self.namespace = namespace
    self.attributionManager = attributionManager
    self.p3aUtilities = p3aUtilities
    self._shouldDismiss = shouldDismiss
  }

  var body: some View {
    if shouldUseExtendedDesign {
      VStack(spacing: 40) {
        stepsContentView
          .background(colorScheme == .dark ? .black : .white)
          .clipShape(RoundedRectangle(cornerRadius: 16.0, style: .continuous))
          .frame(maxWidth: 616, maxHeight: 895)
          .shadow(color: .black.opacity(0.1), radius: 18, x: 0, y: 8)
          .shadow(color: .black.opacity(0.05), radius: 0, x: 0, y: 1)
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .background {
        Image("focus-background-large", bundle: .module)
          .resizable()
          .aspectRatio(contentMode: .fill)
          .opacity(0.20)
      }
      .background {
        NavigationLink("", isActive: $isP3AViewPresented) {
          FocusP3AScreenView(
            attributionManager: attributionManager,
            p3aUtilities: p3aUtilities,
            shouldDismiss: $shouldDismiss
          )
        }
      }
    } else {
      VStack {
        stepsContentView
      }
      .padding(.bottom, 20)
      .background(Color(braveSystemName: .pageBackground))
      .background {
        NavigationLink("", isActive: $isP3AViewPresented) {
          FocusP3AScreenView(
            attributionManager: attributionManager,
            p3aUtilities: p3aUtilities,
            shouldDismiss: $shouldDismiss
          )
        }
      }
    }
  }

  private var stepsContentView: some View {
    VStack {
      if shouldUseExtendedDesign {
        HStack {
          Image("focus-icon-brave", bundle: .module)
            .resizable()
            .matchedGeometryEffect(id: "icon", in: namespace)
            .frame(width: 78, height: 78)

          Image("focus-brave-watermark", bundle: .module)
            .resizable()
            .frame(width: 111, height: 31)
        }
      } else {
        Image("focus-icon-brave", bundle: .module)
          .resizable()
          .matchedGeometryEffect(id: "icon", in: namespace)
          .frame(width: 78, height: 78)
      }
      VStack(spacing: 0) {
        VStack(spacing: 10) {
          Text(Strings.FocusOnboarding.noVideoAdsScreenTitle)
            .font(
              Font.custom("Poppins-SemiBold", size: 28)
            )
            .opacity(0.9)
          Text(Strings.FocusOnboarding.noVideoAdsScreenDescription)
            .font(
              Font.custom("Poppins-Medium", size: 17)
            )
            .foregroundColor(Color(braveSystemName: .textTertiary))
        }
        .multilineTextAlignment(.center)
        .dynamicTypeSize(dynamicTypeRange)
        .fixedSize(horizontal: false, vertical: true)
        .padding(.bottom, 24)

        FocusVideoAdSliderContentView()
          .background(Color(braveSystemName: .pageBackground))
          .clipShape(RoundedRectangle(cornerRadius: 16.0, style: .continuous))
          .tabViewStyle(PageTabViewStyle(indexDisplayMode: .never))
          .padding(.bottom, 24)

        Spacer()

        Button(
          action: {
            isP3AViewPresented = true
          },
          label: {
            Text(Strings.FocusOnboarding.continueButtonTitle)
              .font(.body.weight(.semibold))
              .foregroundColor(Color(braveSystemName: .schemesOnPrimary))
              .dynamicTypeSize(dynamicTypeRange)
              .padding()
              .frame(maxWidth: .infinity)
              .background(Color(braveSystemName: .buttonBackground))
          }
        )
        .clipShape(RoundedRectangle(cornerRadius: 12.0, style: .continuous))
        .overlay(
          RoundedRectangle(cornerRadius: 12.0, style: .continuous).strokeBorder(
            Color.black.opacity(0.2)
          )
        )
      }
    }
    .padding(.vertical, shouldUseExtendedDesign ? 64 : 20)
    .padding(.horizontal, shouldUseExtendedDesign ? 60 : 20)
  }

  private var cancelButton: some View {
    Button(
      action: {
        isP3AViewPresented = true
      },
      label: {
        Image("focus-icon-close", bundle: .module)
          .resizable()
      }
    )
  }
}

#if DEBUG
struct FocusStepsView_Previews: PreviewProvider {
  @Namespace static var namespace

  static var previews: some View {
    @State var shouldDismiss: Bool = false

    FocusStepsView(namespace: namespace, shouldDismiss: $shouldDismiss)
  }
}
#endif
