// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

/// A View that is displayed prior to the first onboarding step that animates the Brave
/// logo and animates the Brave icon down to the first step when in `fullscreen` layout
struct OnboardingSplashView: View {
  @State private var isShimmering: Bool = false
  @Environment(\.onboardingLayoutStyle) private var layoutStyle
  @Environment(\.onboardingNamespace) private var namespace
  @Namespace private var fallbackNamespace

  private var braveTextLogo: some View {
    Image("focus-logo-brave", bundle: .module)
      .resizable()
      .renderingMode(.template)
      .foregroundColor(Color(braveSystemName: .neutral20))
      .aspectRatio(contentMode: .fit)
      .frame(maxWidth: .infinity, maxHeight: .infinity)
  }

  private var logoSize: CGFloat {
    return layoutStyle == .fullscreen ? 96 : 150
  }

  private var linearGradientView: some View {
    Rectangle()
      .foregroundColor(.clear)
      .background(
        LinearGradient(
          stops: [
            Gradient.Stop(color: Color(UIColor(rgb: 0xFFFFFF)).opacity(0), location: 0.00),
            Gradient.Stop(color: Color(UIColor(rgb: 0xFF471A)).opacity(0.25), location: 0.20),
            Gradient.Stop(color: Color(UIColor(rgb: 0xE61987)).opacity(0.25), location: 0.51),
            Gradient.Stop(color: Color(UIColor(rgb: 0x860AC2)).opacity(0.25), location: 0.80),
            Gradient.Stop(color: Color(UIColor(rgb: 0xFFFFFF)).opacity(0), location: 1.00),
          ],
          startPoint: .init(x: 1.43, y: 0.07),
          endPoint: .init(x: 0.23, y: 1)
        )
      )
  }

  var body: some View {
    GeometryReader { proxy in
      braveTextLogo
        .overlay(
          linearGradientView
            .frame(width: proxy.size.width, height: 2 * proxy.size.height)
            .position(
              x: proxy.size.width / 2,
              y: isShimmering ? 2 * proxy.size.height : -proxy.size.height
            )
            .mask(braveTextLogo)
        )
        .overlay {
          BraveAppIcon(
            size: logoSize,
            matchedGeometryInfo: .init(namespace: namespace ?? fallbackNamespace)
          )
          .padding(.bottom, logoSize / 1.4)
        }
    }
    .ignoresSafeArea()
    .onAppear {
      withAnimation(.linear(duration: 1.25)) {
        isShimmering = true
      }
    }
  }
}

#if DEBUG
#Preview {
  OnboardingSplashView()
}
#endif
