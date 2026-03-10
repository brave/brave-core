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
  @Environment(\.openURL) private var openURL
  @Environment(\.dismiss) private var dismiss
  @State private var isPasswordRevealed = false
  @State private var isSceneActive = true

  let viewModel: ManagePasswordsViewModel
  let password: CWVPassword

  var body: some View {
    Form {
      Section {
        LabeledContent {
          Menu {
            ControlGroup {
              Button {
                UIPasteboard.general.string = password.site
              } label: {
                Text(Strings.menuItemCopyTitle)
              }
              Button {
                if let url = URL(string: password.site) {
                  openURL(url)
                }
              } label: {
                Text(Strings.openWebsite)
              }
            }
            .controlGroupStyle(.compactMenu)
          } label: {
            Text(password.site).lineLimit(1)
              .contentShape(.rect)
          }
        } label: {
          Text(Strings.Login.loginInfoDetailsWebsiteFieldTitle)
        }

        LabeledContent {
          Menu {
            Button {
              UIPasteboard.general.string = password.username
            } label: {
              Text(Strings.menuItemCopyTitle)
            }
          } label: {
            Text(password.username ?? "")
              .lineLimit(1)
              .contentShape(.rect)
          }
        } label: {
          Text(Strings.Login.loginInfoDetailsUsernameFieldTitle)
        }

        LabeledContent {
          HStack(spacing: 8) {
            Menu {
              Button {
                UIPasteboard.general.setSecureString(password.password ?? "")
              } label: {
                Text(Strings.menuItemCopyTitle)
              }
            } label: {
              HStack {
                Spacer()
                if isPasswordRevealed {
                  Text(password.password ?? "")
                    .lineLimit(1)
                } else {
                  SecureField(
                    Strings.Autofill.managePasswordDetailInputPasswordPlaceholder,
                    text: .constant(password.password ?? "")
                  )
                  .lineLimit(1)
                  .allowsHitTesting(false)
                  .multilineTextAlignment(.trailing)
                  .frame(maxWidth: .infinity, alignment: .trailing)
                }
              }
              .contentShape(.rect)
            }

            Button {
              isPasswordRevealed.toggle()
            } label: {
              Image(braveSystemName: isPasswordRevealed ? "leo.eye.on" : "leo.eye.off")
                .foregroundStyle(Color(braveSystemName: .textInteractive))
            }
            .buttonStyle(.plain)
          }
        } label: {
          Text(Strings.Login.loginInfoDetailsPasswordFieldTitle)
        }
      }
    }
    .foregroundStyle(Color(braveSystemName: .textPrimary))
    .navigationTitle(URL(string: password.site)?.baseDomain ?? password.title)
    .navigationBarTitleDisplayMode(.inline)
    .toolbarBackground(.visible, for: .navigationBar)
    .overlay {
      if !isSceneActive {
        Color(.braveGroupedBackground)
          .ignoresSafeArea()
      }
    }
    .onReceive(NotificationCenter.default.publisher(for: UIScene.willDeactivateNotification)) { _ in
      isSceneActive = false
    }
    .onReceive(NotificationCenter.default.publisher(for: UIScene.didActivateNotification)) { _ in
      isSceneActive = true
    }
  }
}
