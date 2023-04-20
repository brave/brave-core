// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveStrings

struct FollowToggle: View {
  @Binding var isFollowing: Bool
  
  var body: some View {
    Toggle(isOn: $isFollowing) {
      Text(isFollowing ? Strings.BraveNews.unfollowToggleTitle : Strings.BraveNews.followToggleTitle)
        .lineLimit(1)
        .fixedSize(horizontal: true, vertical: false)
    }
    .animation(.spring(response: 0.4, dampingFraction: 0.75, blendDuration: 0), value: isFollowing)
    .toggleStyle(FollowToggleStyle())
  }

  private struct FollowToggleStyle: ToggleStyle {
    @Environment(\.pixelLength) private var pixelLength
    
    func makeBody(configuration: Configuration) -> some View {
      let isFollowing = configuration.isOn
      Button {
        // For some reason we need this `withAnimation` for the `animation` in `FollowToggle` to be used
        withAnimation {
          configuration.isOn.toggle()
        }
      } label: {
        configuration.label
          .foregroundColor(isFollowing ? Color(.secondaryBraveLabel) : Color.white)
          .font(.footnote.bold())
          .padding(.horizontal, 12)
          .padding(.vertical, 4)
          .background(
            (isFollowing ? Color(.braveBackground) : Color(.braveBlurpleTint))
              .clipShape(Capsule())
          )
          .overlay(
            Capsule()
              .strokeBorder(Color(.secondaryButtonTint).opacity(0.2), lineWidth: pixelLength)
              .opacity(isFollowing ? 1 : 0)
          )
      }
      .buttonStyle(SpringButtonStyle())
    }
  }
  
  private struct SpringButtonStyle: ButtonStyle {
    func makeBody(configuration: Configuration) -> some View {
      configuration.label
        .scaleEffect(configuration.isPressed ? 0.9 : 1, anchor: .center)
        .animation(.spring(), value: configuration.isPressed)
    }
  }
}

#if DEBUG
struct FollowToggle_PreviewProvider: PreviewProvider {
  struct InteractivePreview: View {
    @State private var isOn: Bool = false
    var body: some View {
      FollowToggle(isFollowing: $isOn)
    }
  }
  static var previews: some View {
    VStack {
      Text("Static")
        .font(.headline)
      HStack {
        FollowToggle(isFollowing: .constant(false))
        FollowToggle(isFollowing: .constant(true))
      }
      Divider()
        .padding()
      Text("Interactive")
        .font(.headline)
      InteractivePreview()
    }
    .padding()
    .previewLayout(.sizeThatFits)
  }
}
#endif
