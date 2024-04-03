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
  @Environment(\.verticalSizeClass) var verticalSizeClass: UserInterfaceSizeClass?
  @Environment(\.horizontalSizeClass) var horizontalSizeClass: UserInterfaceSizeClass?
  @Environment(\.dismiss) private var dismiss

  @State private var indicatorIndex = 0
  @State private var opacity = 0.0
  @State private var isP3AViewPresented = false
  @Binding var shouldDismiss: Bool

  private let attributionManager: AttributionManager?
  private let p3aUtilities: BraveP3AUtils?

  var shouldUseExtendedDesign: Bool {
    return horizontalSizeClass == .regular && verticalSizeClass == .regular
  }

  let dynamicTypeRange = (...DynamicTypeSize.xLarge)

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
    NavigationView {
      if shouldUseExtendedDesign {
        VStack(spacing: 40) {
          VStack {
            stepsContentView
              .background(colorScheme == .dark ? .black : .white)
          }
          .clipShape(RoundedRectangle(cornerRadius: 16.0))
          .frame(maxWidth: 616, maxHeight: 895)
          .shadow(color: .black.opacity(0.1), radius: 18, x: 0, y: 8)
          .shadow(color: .black.opacity(0.05), radius: 0, x: 0, y: 1)
          .overlay(alignment: .topTrailing) {
            cancelButton
              .frame(width: 32, height: 32)
              .padding(24)
          }

          FocusStepsPagingIndicator(totalPages: 4, activeIndex: .constant(2))
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
          FocusStepsPagingIndicator(totalPages: 4, activeIndex: $indicatorIndex)
            .opacity(opacity)
            .onAppear {
              withAnimation(.easeInOut(duration: 1.5).delay(1.25)) {
                opacity = 1.0
              }
            }
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
        .overlay(alignment: .topTrailing) {
          cancelButton
            .frame(width: 32, height: 32)
            .padding(24)
        }
      }
    }
    .navigationViewStyle(StackNavigationViewStyle())
    .navigationBarHidden(true)
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
        FocusStepsHeaderTitleView(activeIndex: $indicatorIndex)
          .padding(.bottom, 24)

        TabView(selection: $indicatorIndex) {
          FocusAdTrackerSliderContentView()
            .tag(0)
          FocusVideoAdSliderContentView()
            .tag(1)
        }
        .background(Color(braveSystemName: .pageBackground))
        .clipShape(RoundedRectangle(cornerRadius: 16.0))
        .tabViewStyle(PageTabViewStyle(indexDisplayMode: .never))
        .animation(.easeInOut, value: indicatorIndex)
        .transition(.slide)
        .padding(.bottom, 24)

        Spacer()

        Button(
          action: {
            if indicatorIndex > 0 {
              isP3AViewPresented = true
            } else {
              indicatorIndex += 1
            }
          },
          label: {
            Text(Strings.FocusOnboarding.continueButtonTitle)
              .font(.body.weight(.semibold))
              .foregroundColor(Color(.white))
              .dynamicTypeSize(dynamicTypeRange)
              .padding()
              .foregroundStyle(.white)
              .frame(maxWidth: .infinity)
              .background(Color(braveSystemName: .buttonBackground))
          }
        )
        .clipShape(RoundedRectangle(cornerRadius: 12.0))
        .overlay(RoundedRectangle(cornerRadius: 12.0).strokeBorder(Color.black.opacity(0.2)))
      }
      .opacity(opacity)
      .onAppear {
        withAnimation(.easeInOut(duration: 1.5).delay(1.25)) {
          opacity = 1.0
        }
      }
      .onChange(of: indicatorIndex) { newValue in
        if indicatorIndex == 1 {
          Timer.scheduledTimer(withTimeInterval: 3, repeats: false) { _ in
            UIImpactFeedbackGenerator(style: .medium).vibrate()
          }
        }
      }
    }
    .padding(.vertical, shouldUseExtendedDesign ? 64 : 20)
    .padding(.horizontal, shouldUseExtendedDesign ? 60 : 20)
  }

  private var cancelButton: some View {
    Button(
      action: {
        attributionManager?.pingDAUServer(p3aUtilities?.isP3AEnabled ?? false)

        shouldDismiss = true
      },
      label: {
        Image("focus-icon-close", bundle: .module)
          .resizable()
      }
    )
  }
}

struct FocusStepsHeaderTitleView: View {
  @Binding var activeIndex: Int

  let dynamicTypeRange = (...DynamicTypeSize.large)

  var body: some View {
    let title =
      activeIndex == 0
      ? Strings.FocusOnboarding.movingAdsScreenTitle
      : Strings.FocusOnboarding.noVideoAdsScreenTitle

    let description =
      activeIndex == 0
      ? Strings.FocusOnboarding.movingAdsScreenDescription
      : Strings.FocusOnboarding.noVideoAdsScreenDescription

    VStack(spacing: 10) {
      Text(title)
        .font(
          Font.custom("FlechaM-Medium", size: 32)
        )
        .dynamicTypeSize(dynamicTypeRange)
        .fixedSize(horizontal: false, vertical: true)
        .opacity(0.9)

      Text(description)
        .font(
          Font.custom("Poppins-Medium", size: 17)
        )
        .dynamicTypeSize(dynamicTypeRange)
        .fixedSize(horizontal: false, vertical: true)
        .multilineTextAlignment(.center)
        .foregroundColor(Color(braveSystemName: .textTertiary))
    }
  }
}

struct FocusStepsPagingIndicator: View {
  var totalPages: Int
  @Binding var activeIndex: Int

  var body: some View {
    HStack(spacing: 10) {
      ForEach(0..<totalPages, id: \.self) { index in
        Capsule()
          .fill(
            index == activeIndex
              ? Color(braveSystemName: .textDisabled) : Color(braveSystemName: .dividerStrong)
          )
          .frame(width: index == activeIndex ? 24 : 8, height: 8)
      }
    }
    .frame(maxWidth: .infinity)
  }
}

struct FocusStepsView_Previews: PreviewProvider {
  @Namespace static var namespace

  static var previews: some View {
    @State var shouldDismiss: Bool = false

    FocusStepsView(namespace: namespace, shouldDismiss: $shouldDismiss)
  }
}
