// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import DesignSystem
import Foundation
import Strings
import SwiftUI

struct CredentialDetailView: View {
  @ObservedObject var model: CredentialListModel
  var credential: any Credential

  @State private var isPasswordVisible: Bool = false

  var body: some View {
    Form {
      HStack {
        Text(Strings.CredentialProvider.detailsFormURLField)
        Spacer()
        Text(credential.serviceIdentifier ?? "")
      }
      .menuController(
        prompt: Strings.CredentialProvider.copyMenuItemTitle,
        action: {
          UIPasteboard.general.string = credential.serviceIdentifier ?? ""
        }
      )
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      HStack {
        Text(Strings.CredentialProvider.detailsFormUsernameField)
        Spacer()
        Text(credential.username ?? "")
      }
      .menuController(
        prompt: Strings.CredentialProvider.copyMenuItemTitle,
        action: {
          // Not sure if we need to set this securely
          UIPasteboard.general.string = credential.username ?? ""
        }
      )
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      HStack {
        Text(Strings.CredentialProvider.detailsFormPasswordField)
        Spacer()
        if isPasswordVisible {
          if let password = credential.password {
            Text(password)
          }
        } else {
          Text(verbatim: String(repeating: "•", count: 6))
        }
        Button {
          isPasswordVisible.toggle()
        } label: {
          Label {
            Text(
              isPasswordVisible
                ? Strings.CredentialProvider.hidePasswordMenuItemTitle
                : Strings.CredentialProvider.revealPasswordMenuItemTitle
            )
          } icon: {
            Image(braveSystemName: isPasswordVisible ? "leo.eye.off" : "leo.eye.on")
          }
          .labelStyle(.iconOnly)
          .foregroundColor(Color(braveSystemName: .iconInteractive))
        }
        .buttonStyle(.plain)
      }
      .menuController(
        prompt: isPasswordVisible
          ? Strings.CredentialProvider.copyMenuItemTitle
          : Strings.CredentialProvider.revealPasswordMenuItemTitle,
        action: {
          if isPasswordVisible {
            if let password = credential.password {
              UIPasteboard.general.setSecureString(password)
            }
          } else {
            isPasswordVisible = true
          }
        }
      )
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .foregroundColor(Color(braveSystemName: .textPrimary))
    .listBackgroundColor(Color(.braveGroupedBackground))
    .toolbar {
      ToolbarItemGroup(placement: .confirmationAction) {
        Button {
          model.actionHandler?(.selectedCredential(credential))
        } label: {
          Text(Strings.CredentialProvider.useButtonTitle)
        }
        .tint(Color(braveSystemName: .textInteractive))
      }
    }
  }
}
