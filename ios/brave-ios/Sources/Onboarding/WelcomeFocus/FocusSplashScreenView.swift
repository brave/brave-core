// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct FocusSplashScreenView: View {
  var namespace: Namespace.ID

  @State private var isShimmering = false

  var body: some View {
    GeometryReader { geometry in
      ZStack {
        braveLogo
          .overlay(
            self.linearGradientView
              .frame(width: geometry.size.width, height: 2 * geometry.size.height)
              .position(
                x: geometry.size.width / 2,
                y: isShimmering ? 2 * geometry.size.height : -geometry.size.height
              )
              .mask(
                braveLogo
              )
          )
        Image("focus-icon-brave", bundle: .module)
          .resizable()
          .matchedGeometryEffect(id: "icon", in: namespace)
          .frame(width: 146, height: 146)
      }
      .background(Color(braveSystemName: .pageBackground))
      .onAppear {
        DispatchQueue.main.async {
          withAnimation(.linear(duration: 1.25)) {
            self.isShimmering = true
          }
        }
      }
    }
  }

  private var braveLogo: some View {
    Image("focus-logo-brave", bundle: .module)
      .resizable()
      .renderingMode(.template)
      .foregroundColor(Color(braveSystemName: .neutral20))
      .aspectRatio(contentMode: .fit)
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .padding(.leading, 42)
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
}

#if DEBUG
struct FocusSplashScreenView_Previews: PreviewProvider {
  @Namespace static var namespace

  static var previews: some View {
    FocusSplashScreenView(namespace: namespace)
  }
}
#endif
