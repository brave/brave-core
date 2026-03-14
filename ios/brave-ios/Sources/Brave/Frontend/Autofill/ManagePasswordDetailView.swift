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
  @Environment(\.autofillPrivacyLock) private var privacyLock
  @State private var isPasswordRevealed = false

  let viewModel: ManagePasswordsViewModel
  let password: CWVPassword

  var body: some View {
    Form {
      Section {
        LabeledContent {
          Menu {
            Button {
              UIPasteboard.general.string = password.site
            } label: {
              Text(Strings.menuItemCopyTitle)
            }
            Button {
              if let url = URL(string: password.site),
                url.schemeIsValid,
                url.scheme == "http" || url.scheme == "https"
              {
                openURL(url)
              }
            } label: {
              Text(Strings.openWebsite)
            }
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
              UIPasteboard.general.setSecureString(password.username ?? "")
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
                  Text(String(repeating: "•", count: 8))
                    .lineLimit(1)
                    .allowsHitTesting(false)
                    .accessibility(hidden: !isPasswordRevealed)
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
    .accessibility(hidden: privacyLock?.isLocked ?? false)
    .navigationTitle(URL(string: password.site)?.baseDomain ?? password.title)
    .navigationBarTitleDisplayMode(.inline)
    .toolbarBackground(.visible, for: .navigationBar)
    .toolbar(!(privacyLock?.isLocked ?? true) ? .visible : .hidden, for: .automatic)
    .overlay {
      if privacyLock?.isLocked == true { Color(.braveGroupedBackground).ignoresSafeArea() }
    }
  }
}
