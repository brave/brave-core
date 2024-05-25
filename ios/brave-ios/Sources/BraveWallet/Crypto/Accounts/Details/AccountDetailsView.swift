// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
// For some reason SwiftLint thinks this is a duplicate import
// swiftlint:disable:next duplicate_imports
import BraveShared
import BraveUI
import CoreImage
import CoreImage.CIFilterBuiltins
import Strings
import SwiftUI
import UIKit

struct AccountDetailsView: View {
  @ObservedObject var keyringStore: KeyringStore
  var account: BraveWallet.AccountInfo
  var editMode: Bool

  @State private var name: String = ""
  @State private var isFieldFocused: Bool = false
  @State private var isPresentingRemoveConfirmation: Bool = false

  @Environment(\.presentationMode) @Binding private var presentationMode

  private var isDoneDisabled: Bool {
    name.isEmpty || !name.isValidAccountName
  }

  private func renameAccountAndDismiss() {
    guard !name.isEmpty && name.isValidAccountName else {
      return
    }
    keyringStore.renameAccount(account, name: name)
    presentationMode.dismiss()
  }

  var body: some View {
    NavigationView {
      List {
        Section {
          // TODO: Cleanup with brave-browser#37029
          AccountDetailsHeaderView(account: account)
            .frame(maxWidth: .infinity)
            .listRowInsets(.zero)
            .listRowBackground(Color(.braveGroupedBackground))
        }
        Section(
          content: {
            TextField(
              Strings.Wallet.accountDetailsNamePlaceholder,
              text: $name,
              axis: .vertical
            )
            .introspectTextField { tf in
              if editMode && !isFieldFocused && !tf.isFirstResponder {
                isFieldFocused = tf.becomeFirstResponder()
              }
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          },
          header: {
            WalletListHeaderView(
              title: Text(Strings.Wallet.accountDetailsNameTitle)
                .font(.subheadline.weight(.semibold))
                .foregroundColor(Color(.bravePrimary))
            )
          },
          footer: {
            SectionFooterErrorView(
              errorMessage: name.isValidAccountName
                ? nil
                : String.localizedStringWithFormat(
                  Strings.Wallet.accountNameLengthError,
                  BraveWallet.AccountNameMaxCharacterLength
                )
            )
          }
        )
        if account.coin != .btc {
          Section {
            NavigationLink(
              destination: AccountPrivateKeyView(keyringStore: keyringStore, account: account)
            ) {
              Text(Strings.Wallet.accountPrivateKey)
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
        }
        if account.isImported {
          Section {
            Button {
              isPresentingRemoveConfirmation = true
            } label: {
              Text(Strings.Wallet.accountRemoveButtonTitle)
                .foregroundColor(.red)
                .multilineTextAlignment(.center)
                .frame(maxWidth: .infinity)
            }
            .sheet(
              isPresented: $isPresentingRemoveConfirmation,
              content: {
                RemoveAccountConfirmationView(
                  account: account,
                  keyringStore: keyringStore
                )
              }
            )
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
        }
      }
      .listStyle(InsetGroupedListStyle())
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button {
            presentationMode.dismiss()
          } label: {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
        ToolbarItemGroup(placement: .confirmationAction) {
          Button(action: renameAccountAndDismiss) {
            Text(Strings.done)
          }
          .disabled(isDoneDisabled)
        }
      }
    }
    .accentColor(Color(.braveBlurpleTint))  // needed for navigation bar back button(s)
    .onAppear {
      if name.isEmpty {
        // Wait until next runloop pass to fix bug where body isn't recomputed based on state change
        DispatchQueue.main.async {
          // Setup TextField state binding
          name = account.name
        }
      }
    }
  }
}

struct AccountDetailsHeaderView: View {
  var account: BraveWallet.AccountInfo

  var body: some View {
    VStack(spacing: 12) {
      RoundedRectangle(cornerRadius: 10, style: .continuous)
        .fill(Color.white)
        .frame(width: 220, height: 220)
        .overlay(
          Group {
            if let image = account.address.qrCodeImage?.cgImage {
              Image(uiImage: UIImage(cgImage: image))
                .resizable()
                .interpolation(.none)
                .scaledToFit()
                .padding()
                .accessibilityHidden(true)
            }
          }
        )
      Button {
        UIPasteboard.general.string = account.address
      } label: {
        HStack {
          Text(account.address)
            .foregroundColor(Color(.secondaryBraveLabel))
          Label(Strings.Wallet.copyToPasteboard, braveSystemImage: "leo.copy.plain-text")
            .labelStyle(.iconOnly)
            .foregroundColor(Color(.braveLabel))
        }
      }
      .buttonStyle(.plain)
      .font(.title3.weight(.semibold))
    }
    .padding(.horizontal)
  }
}

#if DEBUG
struct AccountDetailsView_Previews: PreviewProvider {
  static var previews: some View {
    AccountDetailsView(
      keyringStore: .previewStoreWithWalletCreated,
      account: KeyringStore.previewStoreWithWalletCreated.allAccounts.first!,
      editMode: false
    )
    .previewColorSchemes()
  }
}
#endif
