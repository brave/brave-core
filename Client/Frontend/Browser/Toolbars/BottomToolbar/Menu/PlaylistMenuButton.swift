// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import Shared
import DesignSystem

struct PlaylistMenuButton: View {
  var isAdded: Bool
  var action: () -> Void
  @Environment(\.colorScheme) private var colorScheme

  @ViewBuilder
  var fill: some View {
    if isAdded {
      Color(.braveSuccessLabel)
        .transition(AnyTransition.opacity.animation(.default))
    } else {
      LinearGradient(braveGradient: colorScheme == .dark ? .darkGradient02 : .lightGradient02)
        .transition(AnyTransition.opacity.animation(.default))
    }
  }

  var animation: Animation {
    .spring(response: 0.4, dampingFraction: 0.55)
  }

  var body: some View {
    Button(action: action) {
      HStack {
        HStack(spacing: 14) {
          fill
            .frame(width: 32, height: 32)
            .overlay(
              ZStack {
                Image(systemName: "plus")
                  .resizable()
                  .scaleEffect(isAdded ? 0.0001 : 1)
                  .opacity(isAdded ? 0 : 1)
                Image(systemName: "checkmark")
                  .resizable()
                  .foregroundColor(.white)
                  .scaleEffect(isAdded ? 1 : 0.0001)
                  .rotationEffect(isAdded ? .zero : .degrees(60))
              }
              .aspectRatio(contentMode: .fit)
              .padding(6)
              .font(Font.body.weight(.medium))
              .foregroundColor(.white)
            )
            .clipShape(
              RoundedRectangle(cornerRadius: 8, style: .continuous)
            )
            .shadow(color: Color.black.opacity(0.1), radius: 1, x: 0, y: 1)
            .animation(animation, value: isAdded)
          // Transition for default opacity animation
          if isAdded {
            Text(verbatim: Strings.PlayList.toastAddedToPlaylistTitle)
          } else {
            Text(verbatim: Strings.PlayList.toastAddToPlaylistTitle)
          }
        }
        .foregroundColor(Color(.braveLabel))
        Spacer()
        Button(action: action) {
          Text(Strings.PlayList.toastAddToPlaylistOpenButton)
        }
        .buttonStyle(BraveOutlineButtonStyle(size: .small))
        .opacity(isAdded ? 1 : 0)
        .scaleEffect(isAdded ? 1 : 0.85)
        .animation(animation, value: isAdded)
      }
      .padding(.horizontal, 14)
      .frame(maxWidth: .infinity, minHeight: 48.0, alignment: .leading)
    }
    .buttonStyle(TableCellButtonStyle())
  }
}

#if DEBUG
struct PlaylistMenuButton_Previews: PreviewProvider {
  struct InteractivePreview: View {
    @State var isAdded: Bool = false
    var body: some View {
      PlaylistMenuButton(isAdded: isAdded) {
        isAdded.toggle()
      }
    }
  }

  static var previews: some View {
    Group {
      InteractivePreview()
        .previewDisplayName("Interactive")
      ForEach([false, true], id: \.self) { isAdded in
        PlaylistMenuButton(isAdded: isAdded, action: {})
        PlaylistMenuButton(isAdded: isAdded, action: {})
          .preferredColorScheme(.dark)
      }
    }
    .background(Color(.secondaryBraveBackground))
    .previewLayout(.sizeThatFits)
  }
}
#endif
