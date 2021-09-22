/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SwiftUI
import BraveCore
import CoreImage
import CoreImage.CIFilterBuiltins

struct AccountDetailsView: View {
  @ObservedObject var keyringStore: KeyringStore
  var account: BraveWallet.AccountInfo
  var editMode: Bool
  
  @State private var name: String = ""
  @State private var isFieldFocused: Bool = false
  @State private var isPresentingRemoveConfirmation: Bool = false
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  private func removeAccount() {
    keyringStore.removeSecondaryAccount(forAddress: account.address)
  }
  
  private func renameAccountAndDismiss() {
    if name.isEmpty {
      // Show error?
      return
    }
    keyringStore.renameAccount(account, name: name)
    presentationMode.dismiss()
  }
  
  var body: some View {
    NavigationView {
      List {
        Section {
          AccountDetailsHeaderView(address: account.address)
            .frame(maxWidth: .infinity)
            .listRowInsets(.zero)
            .listRowBackground(Color(.braveGroupedBackground))
        }
        Section(
          header: WalletListHeaderView(
            title: Text("Account Name") // NSLocalizedString
              .font(.subheadline.weight(.semibold))
              .foregroundColor(Color(.bravePrimary))
          )
        ) {
          TextField("Account 2", text: $name) // NSLocalizedString
            .introspectTextField { tf in
              if editMode && !isFieldFocused && !tf.isFirstResponder {
                isFieldFocused = tf.becomeFirstResponder()
              }
            }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        if account.isImported {
          Section {
            Button(action: { isPresentingRemoveConfirmation = true }) {
              Text("Remove Account") // NSLocalizedString
                .foregroundColor(.red)
                .multilineTextAlignment(.center)
                .frame(maxWidth: .infinity)
            }
            .alert(isPresented: $isPresentingRemoveConfirmation) {
              Alert(
                title: Text("Remove this account?"), // NSLocalizedString
                message: Text("Are you sure?"), // NSLocalizedString
                primaryButton: .destructive(Text("Yes"), action: removeAccount), // NSLocalizedString
                secondaryButton: .cancel(Text("No")) // NSLocalizedString
              )
            }
            // TODO: iOS 15/Xcode 13, set button role to `destructive`
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
      .onAppear {
        if name.isEmpty {
          // Setup TextField state binding
          name = account.name
        }
      }
      .listStyle(InsetGroupedListStyle())
      .navigationTitle("Account Details") // NSLocalizedString
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: { presentationMode.dismiss() }) {
            Text("Cancel") // NSLocalizedString
              .foregroundColor(Color(.braveOrange))
          }
        }
        ToolbarItemGroup(placement: .confirmationAction) {
          Button(action: renameAccountAndDismiss) {
            Text("Done") // NSLocalizedString
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
  }
}

private struct AccountDetailsHeaderView: View {
  var address: String
  
  private var qrCodeImage: UIImage? {
    guard let addressData = address.data(using: .utf8) else { return nil }
    let context = CIContext()
    let filter = CIFilter.qrCodeGenerator()
    filter.message = addressData
    filter.correctionLevel = "H"
    if let image = filter.outputImage,
       let cgImage = context.createCGImage(image, from: image.extent) {
      return UIImage(cgImage: cgImage)
    }
    return nil
  }
  
  var body: some View {
    VStack(spacing: 12) {
      RoundedRectangle(cornerRadius: 10, style: .continuous)
        .fill(Color.white)
        .frame(width: 220, height: 220)
        .overlay(
          Group {
            if let image = qrCodeImage?.cgImage {
              Image(uiImage: UIImage(cgImage: image))
                .resizable()
                .interpolation(.none)
                .scaledToFit()
                .padding()
            }
          }
        )
      HStack {
        Text(address.truncatedAddress)
          .foregroundColor(Color(.secondaryBraveLabel))
        Button(action: { UIPasteboard.general.string = address }) {
          Image("brave.clipboard")
            .foregroundColor(Color(.braveLabel))
        }
      }
      .font(.title3.weight(.semibold))
    }
    .padding(.horizontal)
  }
}

#if DEBUG
struct AccountDetailsViewController_Previews: PreviewProvider {
  static var previews: some View {
    AccountDetailsView(
      keyringStore: .previewStoreWithWalletCreated,
      account: KeyringStore.previewStoreWithWalletCreated.keyring.accountInfos.first!,
      editMode: false
    )
    .previewColorSchemes()
  }
}
#endif
