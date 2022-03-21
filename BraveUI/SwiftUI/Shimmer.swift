// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

private struct ShimmerViewModifier: ViewModifier {
  var isShimmering: Bool
  var duration: Double

  func body(content: Content) -> some View {
    content
      .mask(
        Group {
          if isShimmering {
            _GradientView(animation: .easeInOut(duration: duration).repeatForever(autoreverses: false))
              .transition(.opacity.animation(.default))
          } else {
            Color.black
              .transition(.opacity.animation(.default))
          }
        }
      )
  }

  struct _GradientView: View {
    var animation: Animation

    // LinearGradient unfortunately cannot animate its colors, so we'll animate its start/end points
    @State private var points: (UnitPoint, UnitPoint) = (UnitPoint(x: -2, y: 0.5), .leading)

    var body: some View {
      LinearGradient(
        colors: [.black.opacity(0.3), .black, .black.opacity(0.3)],
        startPoint: points.0,
        endPoint: points.1
      )
      .onAppear {
        DispatchQueue.main.async { [self] in  // Need this due to a SwiftUI bug…
          withAnimation(animation) {
            points = (.trailing, UnitPoint(x: 2, y: 0.5))
          }
        }
      }
    }
  }
}

extension View {
  /// Applies a shimmering mask and animation to the view to denote that some activity is happening
  ///
  /// While shimmering can be applied to any type of `View`, it's best to apply it to redacted views
  /// to create a placeholder-style loading indicator, for example:
  ///
  ///     Text("This is some placeholder text that a user will never see")
  ///        .shimmer(true)
  ///        .redacted(reason: .placeholder)
  public func shimmer(_ shimmering: Bool, duration: Double = 2) -> some View {
    modifier(ShimmerViewModifier(isShimmering: shimmering, duration: duration))
  }
}

#if DEBUG
struct ShimmerViewModifier_PreviewProvider: PreviewProvider {
  struct PreviewView: View {
    @State private var isShimmering: Bool = false
    var body: some View {
      VStack {
        Text("Donec ullamcorper nulla non metus auctor fringilla. Donec id elit non mi porta gravida at eget metus. Cras justo odio, dapibus ac facilisis in, egestas eget quam.")
          .shimmer(isShimmering)
          .redacted(reason: .placeholder)
        Toggle(isOn: $isShimmering) {
          Text("Shimmering")
        }
      }
      .padding()
    }
  }

  static var previews: some View {
    PreviewView()
  }
}
#endif
