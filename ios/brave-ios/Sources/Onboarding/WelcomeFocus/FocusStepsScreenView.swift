// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct FocusStepsView: View {
  var namespace: Namespace.ID

  @State var activeIndex = 0

  var body: some View {
    VStack {
      Image("focus-icon-brave", bundle: .module)
        .resizable()
        .matchedGeometryEffect(id: "icon", in: namespace)
        .frame(width: 78, height: 78)

      FocusStepsHeaderTitleView(activeIndex: $activeIndex)

      TabView(selection: $activeIndex) {
        FocusAdTrackerSliderContentView()
          .tag(0)
        FocusVideoAdSliderContentView()
          .tag(1)
      }
      .frame(maxWidth: .infinity)
      .frame(height: 420)
      .background(Color(braveSystemName: .pageBackground))
      .tabViewStyle(PageTabViewStyle(indexDisplayMode: .never))
      .animation(.easeInOut, value: activeIndex)
      .transition(.slide)
      .padding(.bottom, 24)

      Button(
        action: {
          activeIndex += 1
        },
        label: {
          Text("Continue")
            .font(.body.weight(.semibold))
            .foregroundColor(Color(.white))
            .padding()
            .foregroundStyle(.white)
            .frame(maxWidth: .infinity)
            .background(Color(braveSystemName: .buttonBackground))
        }
      )
      .clipShape(RoundedRectangle(cornerRadius: 12.0))
      .overlay(RoundedRectangle(cornerRadius: 12.0).strokeBorder(Color.black.opacity(0.2)))
      .padding(.bottom, 24)

      FocusStepsPagingIndicator(totalPages: 4, activeIndex: $activeIndex)

      Spacer()

    }
    .padding(.horizontal, 20)
    .background(Color(braveSystemName: .pageBackground))
  }
}

struct FocusStepsHeaderTitleView: View {
  @Binding var activeIndex: Int

  var body: some View {
    let title = activeIndex == 0 ? "Fewer ads & trackers." : "No more video ads."
    let description =
      activeIndex == 0 ? "Browse faster and use less data." : "Seriously, we got rid of them."

    Text(title)
      .font(Font.largeTitle)

    Text(description)
      .font(.body.weight(.medium))
      .foregroundColor(Color(braveSystemName: .textTertiary))
      .padding(.bottom, 24)
  }
}

struct FocusStepsPagingIndicator: View {
  var totalPages: Int

  var activeTint = Color(braveSystemName: .textDisabled)
  var inActiveTint = Color(braveSystemName: .dividerStrong)

  @Binding var activeIndex: Int

  var body: some View {
    HStack(spacing: 10) {
      ForEach(0..<totalPages, id: \.self) { index in
        Capsule()
          .fill(
            index == activeIndex
              ? FocusOnboarding.activeIndicatorTint : FocusOnboarding.inactiveIndicatorTint
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
