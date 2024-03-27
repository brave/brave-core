// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Growth
import Preferences
import SwiftUI
import os.log

struct FocusStepsView: View {
  var namespace: Namespace.ID

  @Environment(\.dismiss) private var dismiss

  @State private var indicatorIndex = 0
  @State private var opacity = 0.0
  @State private var isP3AViewPresented = false
  @State private var shouldDismiss = false

  private let attributionManager: AttributionManager?
  private let p3aUtilities: BraveP3AUtils?

  let dynamicTypeRange = (...DynamicTypeSize.xLarge)

  public init(
    namespace: Namespace.ID,
    attributionManager: AttributionManager? = nil,
    p3aUtilities: BraveP3AUtils? = nil
  ) {
    self.namespace = namespace
    self.attributionManager = attributionManager
    self.p3aUtilities = p3aUtilities
  }

  var body: some View {
    NavigationView {
      VStack {
        Image("focus-icon-brave", bundle: .module)
          .resizable()
          .matchedGeometryEffect(id: "icon", in: namespace)
          .frame(width: 78, height: 78)

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
          .tabViewStyle(PageTabViewStyle(indexDisplayMode: .never))
          .animation(.easeInOut, value: indicatorIndex)
          .transition(.slide)
          .padding(.bottom, 24)

          Spacer()

          VStack(spacing: 28) {
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

            FocusStepsPagingIndicator(totalPages: 4, activeIndex: $indicatorIndex)
          }
          .padding(.bottom, 20)
        }
        .opacity(opacity)
        .onAppear {
          withAnimation(.easeInOut(duration: 1.5).delay(1.5)) {
            opacity = 1.0
          }
        }
      }
      .padding(.horizontal, 20)
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
        Button(
          action: {
            attributionManager?.pingDAUServer(p3aUtilities?.isP3AEnabled ?? false)

            shouldDismiss = true
          },
          label: {
            Image("focus-icon-close", bundle: .module)
              .resizable()
              .frame(width: 32, height: 32)
              .padding(.trailing, 24)
          }
        )
      }
      .onChange(of: shouldDismiss) { shouldDismiss in
        if shouldDismiss {
          Preferences.Onboarding.basicOnboardingCompleted.value = OnboardingState.completed.rawValue
          Preferences.AppState.shouldDeferPromotedPurchase.value = false
          Preferences.FocusOnboarding.focusOnboardingFinished.value = true

          dismiss()
        }
      }
      .navigationBarHidden(true)
    }
  }
}

struct FocusStepsHeaderTitleView: View {
  @Binding var activeIndex: Int

  let dynamicTypeRange = (...DynamicTypeSize.large)

  var body: some View {
    let title = activeIndex == 0 ? "Fewer ads & trackers." : "No more video ads."
    let description =
      activeIndex == 0 ? "Browse faster and use less data." : "Seriously, we got rid of them."

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
        .fixedSize(horizontal: false, vertical: true)
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
    FocusStepsView(namespace: namespace)
  }
}
