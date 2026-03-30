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
  @Environment(\.redactionReasons) private var redactionReasons
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
              if let url = URL(string: password.site), url.isWebPage() {
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
        .listRowBackground(Color(.secondaryBraveGroupedBackground))

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
        .listRowBackground(Color(.secondaryBraveGroupedBackground))

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
                    .accessibility(hidden: true)
                    .multilineTextAlignment(.trailing)
                    .frame(maxWidth: .infinity, alignment: .trailing)
                }
              }
              .contentShape(.rect)
            }

            Button {
              isPasswordRevealed.toggle()
            } label: {
              Label(
                Strings.Autofill.managePasswordDetailRevealPassword,
                braveSystemImage: isPasswordRevealed ? "leo.eye.on" : "leo.eye.off"
              )
              .foregroundStyle(Color(braveSystemName: .textInteractive))
              .labelStyle(.iconOnly)
            }
            .buttonStyle(.plain)
          }
        } label: {
          Text(Strings.Login.loginInfoDetailsPasswordFieldTitle)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .scrollContentBackground(.hidden)
    .background((Color(.braveGroupedBackground)))
    .foregroundStyle(Color(braveSystemName: .textPrimary))
    .accessibility(hidden: redactionReasons.contains(.privacy) ? true : false)
    .navigationTitle(URL(string: password.site)?.baseDomain ?? password.title)
    .navigationBarTitleDisplayMode(.inline)
    .toolbarBackground(.visible, for: .navigationBar)
    .toolbar(redactionReasons.contains(.privacy) ? .hidden : .visible, for: .automatic)
    .overlay {
      if redactionReasons.contains(.privacy) { Color(.braveGroupedBackground).ignoresSafeArea() }
    }
  }
}
