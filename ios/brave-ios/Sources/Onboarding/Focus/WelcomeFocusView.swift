// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Foundation
import Introspect
import Strings
import SwiftUI

struct WelcomeFocusView: View {
  @State private var isAnimating = false
  @State private var isShimmering = false

  var body: some View {
    GeometryReader { geometry in
      ZStack {
        braveLogo
          .scaleEffect(isShimmering ? 1.015 : 1.0)
          .overlay(
            self.linearGradientView
              .frame(width: geometry.size.width, height: 2 * geometry.size.height)
              .position(x: geometry.size.width / 2, y: isShimmering ? 2 * geometry.size.height : -geometry.size.height)
              .scaleEffect(isShimmering ? 1.015 : 1.0)
              .mask(
                braveLogo
              )
          )
          .hidden(isHidden: self.isAnimating)

        Image("focus-icon-brave", bundle: .module)
          .scaleEffect(isAnimating ? 0.6 : 1.0)
          .position(x: geometry.size.width / 2, y: isAnimating ? 50 : geometry.size.height / 2)
      }
      .background(Color(braveSystemName: .pageBackground))
      .onAppear {
        withAnimation(.linear(duration: 1.5)) {
          self.isShimmering = true
        }
        Timer.scheduledTimer(withTimeInterval: 1.5, repeats: false) { timer in
          withAnimation(.easeInOut(duration: 1.5)) {
            self.isAnimating = true
          }
        }
      }
    }
  }

  private var linearGradientView: some View {
    Rectangle()
      .foregroundColor(.clear)
      .background(
        LinearGradient(
          stops: [
            Gradient.Stop(color: Color(UIColor(rgb: 0xFF5602)).opacity(0), location: 0.00),
            Gradient.Stop(color: Color(UIColor(rgb: 0xFF5602)).opacity(0.25), location: 0.38),
            Gradient.Stop(color: Color(UIColor(rgb: 0xFF2202)).opacity(0.25), location: 0.59),
            Gradient.Stop(color: Color(UIColor(rgb: 0xFF2302)).opacity(0), location: 1.00),
          ],
          startPoint: .init(x: 1.43, y: 0.07),
          endPoint: .init(x: 0.23, y: 1)
        )
      )
  }

  private var braveLogo: some View {
    Image("focus-logo-brave", bundle: .module)
      .resizable()
      .aspectRatio(contentMode: .fit)
      .frame(maxWidth: .infinity)
      .padding(.leading, 42)
  }
}

struct WelcomeFocusView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      WelcomeFocusView()
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}


extension View {
  /// Helper for `hidden()` modifier that accepts a boolean determining if we should hide the view or not.
  @ViewBuilder public func hidden(isHidden: Bool) -> some View {
    if isHidden {
      self.hidden()
    } else {
      self
    }
  }
}
