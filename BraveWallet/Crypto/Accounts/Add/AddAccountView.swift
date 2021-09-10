/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveUI

@available(iOS 14.0, *)
struct AddAccountView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  @State private var name: String = ""
  @State private var privateKey: String = ""
  @State private var isPresentingImport: Bool = false
  @State private var isLoadingFile: Bool = false
  @State private var originPassword: String = ""
  @State private var failedToImport: Bool = false
  @ScaledMetric(relativeTo: .body) private var privateKeyFieldHeight: CGFloat = 140.0
  @Environment(\.presentationMode) @Binding var presentationMode
  
  private func addAccount() {
    if privateKey.isEmpty {
      // Add normal account
      keyringStore.addPrimaryAccount(name) { success in
        // TODO: Error state
        if success {
          presentationMode.dismiss()
        }
      }
    } else {
      let handler: (Bool, String) -> Void = { success, _ in
        if success {
          presentationMode.dismiss()
        } else {
          failedToImport = true
        }
      }
      if isJSONImported {
        keyringStore.addSecondaryAccount(name, json: privateKey, password: originPassword, completion: handler)
      } else {
        keyringStore.addSecondaryAccount(name, privateKey: privateKey, completion: handler)
      }
    }
  }
  
  private var isJSONImported: Bool {
    guard let data = privateKey.data(using: .utf8) else {
      return false
    }
    do {
      let _ = try JSONSerialization.jsonObject(with: data, options: [])
      return true
    } catch {
      return false
    }
  }
  
  var body: some View {
    List {
      accountNameSection
      if isJSONImported {
        originPasswordSection
      }
      privateKeySection
    }
    .listStyle(InsetGroupedListStyle())
    .sheet(isPresented: $isPresentingImport) {
      DocumentOpenerView(allowedContentTypes: [.text, .json]) { urls in
        guard let fileURL = urls.first else { return }
        self.isLoadingFile = true
        DispatchQueue.global(qos: .userInitiated).async {
          do {
            let data = try String(contentsOf: fileURL)
            DispatchQueue.main.async {
              self.privateKey = data
              self.isLoadingFile = false
            }
          } catch {
            DispatchQueue.main.async {
              // Error: Couldn't load file
              self.isLoadingFile = false
            }
          }
        }
      }
    }
    .animation(.default, value: isJSONImported)
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle("Add Account") // NSLocalizedString
    .navigationBarItems(
      // Have to use this instead of toolbar placement to have a custom button style
      trailing: Button(action: addAccount) {
        Text("Add")
      }
      .buttonStyle(BraveFilledButtonStyle(size: .small))
      .disabled(name.isEmpty)
    )
    .toolbar {
      ToolbarItemGroup(placement: .cancellationAction) {
        Button(action: { presentationMode.dismiss() }) {
          Text("Cancel")
            .foregroundColor(.accentColor)
        }
      }
    }
    .alert(isPresented: $failedToImport) {
      Alert(
        title: Text("Failed to import account."), // NSLocalizedString
        message: Text("Please try again."), // NSLocalizedString
        dismissButton: .cancel(Text("OK")) // NSLocalizedString
      )
    }
  }
  
  private var accountNameSection: some View {
    Section(
      header: WalletListHeaderView(
        title: Text("Account Name") // NSLocalizedString
          .font(.subheadline.weight(.semibold))
          .foregroundColor(Color(.bravePrimary))
      )
      .osAvailabilityModifiers { content in
        if #available(iOS 15.0, *) {
          content // Padding already applied
        } else {
          content
            .padding(.top)
        }
      }
    ) {
      TextField("Account 2", text: $name) // NSLocalizedString
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
  }
  
  private var originPasswordSection: some View {
    Section(
      header: WalletListHeaderView(
        title: Text("Origin Password") // NSLocalizedString
          .font(.subheadline.weight(.semibold))
          .foregroundColor(Color(.bravePrimary))
      )
    ) {
      SecureField("Password", text: $originPassword)
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
  }
  
  private var privateKeySection: some View {
    Section(
      header: WalletListHeaderView(
        title: Text("You can create a secondary account by importing your private key.") // NSLocalizedString
          .font(.subheadline.weight(.semibold))
          .foregroundColor(Color(.bravePrimary))
      )
    ) {
      TextEditor(text: $privateKey)
        .font(.system(.body, design: .monospaced))
        .frame(height: privateKeyFieldHeight)
        .background(
          Text("Enter, paste, or import your private key string file or JSON.") // NSLocalizedString
            .padding(.vertical, 8)
            .padding(.horizontal, 4) // To match the TextEditor's editing insets
            .frame(maxWidth: .infinity, alignment: .leading)
            .foregroundColor(Color(.placeholderText))
            .opacity(privateKey.isEmpty ? 1 : 0),
          alignment: .top
        )
        .introspectTextView { textView in
          textView.smartQuotesType = .no
        }
      Button(action: { isPresentingImport = true }) {
        HStack {
          Text("Import…") // NSLocalizedString
            .foregroundColor(.accentColor)
            .font(.callout)
          if isLoadingFile {
            ProgressView()
              .progressViewStyle(CircularProgressViewStyle())
          }
        }
        .frame(maxWidth: .infinity)
      }
      .disabled(isLoadingFile)
    }
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
  }
}

#if DEBUG
@available(iOS 14.0, *)
struct AddAccountView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      AddAccountView(keyringStore: .previewStore)
    }
  }
}
#endif
