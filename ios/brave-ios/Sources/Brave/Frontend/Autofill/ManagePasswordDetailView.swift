// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import LocalAuthentication
import SwiftUI
import UIKit

struct ManagePasswordDetailView: View {
  @Bindable var viewModel: ManagePasswordDetailViewModel
  @Environment(\.autofillManagementContext) private var context
  @State private var isPasswordRevealed = false
  @State private var showDeleteAlert = false
  @State private var showSetPasscodeAlert = false
  @State private var isSceneActive = true
  @Environment(\.dismiss) private var dismiss

  private var navigationTitle: String {
    URL(string: viewModel.site)?.baseDomain ?? ""
  }

  var body: some View {
    Form {
      Section {
        LabeledContent {
          Text(viewModel.site)
            .lineLimit(1)
        } label: {
          Text(Strings.Login.loginInfoDetailsWebsiteFieldTitle)
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        .contentShape(Rectangle())
        .contextMenu {
          Button {
            UIPasteboard.general.string = viewModel.site
          } label: {
            Label(Strings.menuItemCopyTitle, braveSystemImage: "leo.copy")
          }
          Button {
            if let url = URL(string: viewModel.site) {
              context?.openURLInNewTab(url)
              dismiss()
            }
          } label: {
            Label(Strings.openWebsite, braveSystemImage: "leo.discover")
          }
        }

        LabeledContent {
          Text(viewModel.username)
            .lineLimit(1)
        } label: {
          Text(Strings.Login.loginInfoDetailsUsernameFieldTitle)
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
        }
        .contentShape(Rectangle())
        .contextMenu {
          Button {
            UIPasteboard.general.string = viewModel.username
          } label: {
            Label(Strings.menuItemCopyTitle, braveSystemImage: "leo.copy")
          }
        }

        LabeledContent {
          HStack(spacing: 8) {
            Spacer()
            if isPasswordRevealed {
              Text(viewModel.passwordValue)
                .lineLimit(1)
            } else {
              SecureField(
                Strings.Autofill.managePasswordDetailsInputPasswordPlaceholder,
                text: $viewModel.passwordValue
              )
              .lineLimit(1)
              .allowsHitTesting(false)
              .multilineTextAlignment(.trailing)
              .frame(maxWidth: .infinity, alignment: .trailing)
            }
            Button {
              togglePasswordReveal()
            } label: {
              Image(braveSystemName: isPasswordRevealed ? "leo.eye.on" : "leo.eye.off")
                .foregroundColor(Color(braveSystemName: .primary40))
            }
          }
        } label: {
          Text(Strings.Login.loginInfoDetailsPasswordFieldTitle)
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        .contentShape(Rectangle())
        .contextMenu {
          Button {
            context?.askForAuthentication { success, error in
              if success {
                UIPasteboard.general.setSecureString(viewModel.passwordValue)
              } else if error == .passcodeNotSet {
                showSetPasscodeAlert = true
              }
            }
          } label: {
            Label(Strings.menuItemCopyTitle, braveSystemImage: "leo.copy")
          }
        }

      }
    }
    .background(Color(.braveGroupedBackground))
    .scrollContentBackground(.hidden)
    .navigationTitle(navigationTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbarBackground(.visible, for: .navigationBar)
  }

  private func togglePasswordReveal() {
    if isPasswordRevealed {
      isPasswordRevealed = false
    } else {
      context?.askForAuthentication { [self] success, error in
        if success {
          isPasswordRevealed = true
        } else if error == .passcodeNotSet {
          showSetPasscodeAlert = true
        }
      }
    }
  }
}
