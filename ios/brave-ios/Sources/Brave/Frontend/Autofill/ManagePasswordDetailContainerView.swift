// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import SwiftUI

struct ManagePasswordDetailContainerView: View {
  @Environment(\.redactionReasons) private var redactionReasons

  @State private var isPasswordRevealed = false
  @State private var isEditing = false

  let viewModel: ManagePasswordsViewModel
  let password: CWVPassword
  let redactedTitle = Strings.Autofill.managePasswordsTitle

  private var navigationTitle: String {
    guard !redactionReasons.contains(.privacy) else { return redactedTitle }
    return URL(string: password.site)?.baseDomain ?? password.title
  }

  var body: some View {
    Group {
      if isEditing {
        ManagePasswordDetailEditableView(
          isPasswordRevealed: $isPasswordRevealed,
          viewModel: viewModel,
          password: password,
          onFinishEditing: { isEditing = false }
        )
      } else {
        ManagePasswordDetailReadOnlyView(
          isPasswordRevealed: $isPasswordRevealed,
          password: password
        )
      }
    }
    .accessibility(hidden: redactionReasons.contains(.privacy) ? true : false)
    .navigationTitle(navigationTitle)
    .navigationBarTitleDisplayMode(.inline)
    .navigationBarBackButtonHidden(isEditing)
    .toolbar {
      if !isEditing {
        ToolbarItem(placement: .topBarTrailing) {
          Button(Strings.edit) {
            isEditing = true
          }
        }
      }
    }
    .overlay {
      if redactionReasons.contains(.privacy) {
        Color(uiColor: .systemGroupedBackground).ignoresSafeArea()
      }
    }
  }
}
