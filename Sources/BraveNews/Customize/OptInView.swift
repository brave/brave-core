// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveUI
import DesignSystem
import BraveStrings
import Lottie

struct OptInView: View {
  var tappedTurnOn: @Sendable () async throws -> Void
  var tappedLearnMore: () -> Void
  
  @State private var isLoading: Bool = false
  
  var body: some View {
    VStack(spacing: 36) {
      Image("opt-in-news-graphic", bundle: .module)
        .resizable()
        .aspectRatio(contentMode: .fit)
        .padding(.horizontal)
      VStack(spacing: 12) {
        Text(Strings.BraveNews.introCardTitle)
          .font(.title3.bold())
          .multilineTextAlignment(.center)
          .foregroundColor(Color(.bravePrimary))
        Text(Strings.BraveNews.introCardBody)
          .font(.subheadline)
          .multilineTextAlignment(.center)
          .foregroundColor(Color(.braveLabel))
      }
      VStack(spacing: 16) {
        Button(action: {
          isLoading = true
          Task {
            try await tappedTurnOn()
            isLoading = false
          }
        }) {
          Text(Strings.BraveNews.turnOnBraveNews)
            .opacity(isLoading ? 0 : 1)
            .overlay(
              ProgressView()
                .progressViewStyle(.braveCircular(size: .small, tint: .white))
                .opacity(isLoading ? 1 : 0)
            )
            .animation(.default, value: isLoading)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        Button(action: tappedLearnMore) {
          Text(Strings.BraveNews.learnMoreTitle)
            .font(.subheadline.weight(.semibold))
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
    .padding()
    .accessibilityEmbedInScrollView()
  }
}

#if DEBUG
struct OptInView_PreviewProvider: PreviewProvider {
  static var previews: some View {
    OptInView(
      tappedTurnOn: {
        try await Task.sleep(nanoseconds: NSEC_PER_SEC * 2)
      },
      tappedLearnMore: { }
    )
  }
}
#endif
