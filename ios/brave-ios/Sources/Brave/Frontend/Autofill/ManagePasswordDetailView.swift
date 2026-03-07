// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import SwiftUI
import UIKit

struct ManagePasswordDetailView: View {
  @Environment(\.autofillManagementContext) private var context
  @Environment(\.dismiss) private var dismiss
  @State private var isPasswordRevealed = false
  @Bindable var viewModel: ManagePasswordDetailViewModel

  private var navigationTitle: String {
    URL(string: viewModel.site)?.baseDomain ?? ""
  }

  var body: some View {
    List {
      Section {
        Menu {
          Button {
            UIPasteboard.general.string = viewModel.site
          } label: {
            Text(Strings.menuItemCopyTitle)
          }
          Button {
            if let url = URL(string: viewModel.site) {
              context?.openURLInNewTab(url)
              dismiss()
            }
          } label: {
            Text(Strings.openWebsite)
          }
        } label: {
          LabeledContent {
            Text(viewModel.site)
              .lineLimit(1)
          } label: {
            Text(Strings.Login.loginInfoDetailsWebsiteFieldTitle)
          }
          .contentShape(Rectangle())
        }

        Menu {
          Button {
            UIPasteboard.general.string = viewModel.username
          } label: {
            Text(Strings.menuItemCopyTitle)
          }
        } label: {
          LabeledContent {
            Text(viewModel.username)
              .lineLimit(1)
          } label: {
            Text(Strings.Login.loginInfoDetailsUsernameFieldTitle)
          }
          .contentShape(Rectangle())
        }

        LabeledContent {
          HStack(spacing: 8) {
            Menu {
              Button {
                UIPasteboard.general.setSecureString(viewModel.passwordValue)
              } label: {
                Text(Strings.menuItemCopyTitle)
              }
            } label: {
              HStack {
                Spacer()
                if isPasswordRevealed {
                  Text(viewModel.passwordValue)
                    .lineLimit(1)
                } else {
                  // allowsHitTesting(false) intentionally prevents editing in View only mode
                  SecureField(
                    Strings.Autofill.managePasswordDetailsInputPasswordPlaceholder,
                    text: $viewModel.passwordValue
                  )
                  .lineLimit(1)
                  .allowsHitTesting(false)
                  .multilineTextAlignment(.trailing)
                  .frame(maxWidth: .infinity, alignment: .trailing)
                }
              }
              .contentShape(Rectangle())
            }
            Label {
              Text(Strings.Autofill.managePasswordDetailsRevealPassword)
            } icon: {
              Image(braveSystemName: isPasswordRevealed ? "leo.eye.on" : "leo.eye.off")
                .foregroundStyle(Color(braveSystemName: .textInteractive))
            }
            .labelStyle(.iconOnly)
            .onTapGesture {
              isPasswordRevealed.toggle()
            }
          }
        } label: {
          Text(Strings.Login.loginInfoDetailsPasswordFieldTitle)
        }
      }
    }
    .foregroundStyle(Color(braveSystemName: .textPrimary))
    .navigationTitle(navigationTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbarBackground(.visible, for: .navigationBar)
  }
}
