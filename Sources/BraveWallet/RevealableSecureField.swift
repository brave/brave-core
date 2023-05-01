// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI

struct RevealableSecureField: View {
  
  private enum Focus: Hashable {
    case secure
    case visible
  }

  let placeholder: String
  let showsRevealButton: Bool
  @Binding var text: String
  @Binding var isRevealed: Bool
  @FocusState private var focus: Focus?
  
  init(
    _ placeholder: String,
    showsRevealButton: Bool = true,
    text: Binding<String>,
    isRevealed: Binding<Bool>
  ) {
    self.placeholder = placeholder
    self.showsRevealButton = showsRevealButton
    self._text = text
    self._isRevealed = isRevealed
  }

  var body: some View {
    HStack(alignment: .firstTextBaseline, spacing: 4) {
      Group {
        if isRevealed {
          TextField(placeholder, text: $text)
            .focused($focus, equals: .visible)
        } else {
          SecureField(placeholder, text: $text)
            .focused($focus, equals: .secure)
        }
      }
      .keyboardType(.asciiCapable)
      .autocorrectionDisabled(true)
      .autocapitalization(.none)
      .introspectTextField {
        $0.autocorrectionType = .no
        $0.autocapitalizationType = .none
        $0.spellCheckingType = .no
      }
      if showsRevealButton {
        Button(action: {
          if let focus { // if user has a field focused
            // focus the new field automatically
            if focus == .secure {
              self.focus = .visible
            } else {
              self.focus = .secure
            }
          }
          self.isRevealed.toggle()
        }) {
          Image(braveSystemName: isRevealed ? "leo.eye.off" : "leo.eye.on")
            .padding(.vertical, 6)
            .padding(.horizontal, 4)
            .contentShape(Rectangle())
            .transition(.identity)
            .animation(nil, value: isRevealed)
        }
      }
    }
  }
}
