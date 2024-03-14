// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Foundation
import Introspect
import Strings
import SwiftUI

public struct WelcomeFocusView: View {
  @State private var splash = true
  @Namespace var namespace

  public init() {}

  public var body: some View {
    VStack {
      if splash {
        SplashScreen(namespace: namespace)
      } else {
        Steps(namespace: namespace)
          .padding(.horizontal, 20)
      }
    }
    .onAppear {
      withAnimation(.easeInOut(duration: 1.5).delay(1.5)) {
        splash = false
      }
    }
  }
}

struct SplashScreen: View {
  var namespace: Namespace.ID

  @State private var isShimmering = false

  var body: some View {
    GeometryReader { geometry in
      ZStack {
        braveLogo
          .scaleEffect(isShimmering ? 1.015 : 1.0)
          .overlay(
            self.linearGradientView
              .frame(width: geometry.size.width, height: 2 * geometry.size.height)
              .position(
                x: geometry.size.width / 2,
                y: isShimmering ? 2 * geometry.size.height : -geometry.size.height
              )
              .scaleEffect(isShimmering ? 1.015 : 1.0)
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
        withAnimation(.linear(duration: 1.5)) {
          self.isShimmering = true
        }
      }
    }
  }

  private var braveLogo: some View {
    Image("focus-logo-brave", bundle: .module)
      .resizable()
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
}

struct Steps: View {
  var namespace: Namespace.ID

  @State var tabSelection = 0

  var body: some View {
    VStack {
      Image("focus-icon-brave", bundle: .module)
        .resizable()
        .matchedGeometryEffect(id: "icon", in: namespace)
        .frame(width: 78, height: 78)

      Text("Fewer ads & trackers.")
        .font(Font.largeTitle)

      Text("Browse faster and use less data.")
        .font(.body.weight(.medium))
        .foregroundColor(Color(braveSystemName: .textTertiary))
        .padding(.bottom, 28)

      TabView(selection: $tabSelection) {
        AdTrackerSliderContentView()
          .tag(0)
        Text("A - Tab 1")
          .tag(1)
        AdTrackerSliderContentView()
          .tag(2)
        Text("A - Tab 1")
          .tag(3)
      }
      .frame(maxWidth: /*@START_MENU_TOKEN@*/.infinity/*@END_MENU_TOKEN@*/)
      .frame(height: 420)
      .background(Color(braveSystemName: .pageBackground))
      .tabViewStyle(PageTabViewStyle(indexDisplayMode: .never))
      .animation(.easeInOut, value: tabSelection)
      .transition(.slide)
      .padding(.bottom, 28)

      Button(
        action: {
          tabSelection += 1
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

      PaginIndicator(totalPages: 4, activeIndex: $tabSelection)

      Spacer()

    }
    .background(Color(braveSystemName: .pageBackground))
  }
}

struct WelcomeFocusView_Previews: PreviewProvider {
  static var previews: some View {
    WelcomeFocusView()
  }
}

struct PaginIndicator: View {
  var activeTint: Color = .red
  var inActiveTint: Color = .blue
  var clipEdges: Bool = false

  var totalPages = 4

  @Binding var activeIndex: Int

  var body: some View {
    HStack(spacing: 10) {
      ForEach(0..<totalPages, id: \.self) { index in
        Capsule()
          .fill(index == activeIndex ? activeTint : inActiveTint)
          .frame(width: index == activeIndex ? 18 : 8, height: 8)
      }
    }
    .frame(maxWidth: .infinity)
  }
}
