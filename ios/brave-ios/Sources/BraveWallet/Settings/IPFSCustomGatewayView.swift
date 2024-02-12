// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import DesignSystem

struct IPFSCustomGatewayView: View {
  
  private static let ipfsTestPath = "ipfs"
  private static let ipfsTestCID = "bafkqae2xmvwgg33nmuqhi3zajfiemuzahiwss"
  private static let ipfsTestContent = "Welcome to IPFS :-)"
  
  private enum SetButtonStatus {
    case enabled
    case disabled
    case loading
  }
  
  private let ipfsAPI: IpfsAPI
  private let isForNFT: Bool
  @State private var url: String = "https://"
  @State private var setButtonStatus: SetButtonStatus = .disabled
  @State private var isPresentingWrongGatewayAlert: Bool = false
  
  init(ipfsAPI: IpfsAPI, isForNFT: Bool = false) {
    self.ipfsAPI = ipfsAPI
    self.isForNFT = isForNFT
  }
  
  var body: some View {
    List {
      Section(footer: Text(Strings.Wallet.nftGatewayLongDescription)) {
        TextField(ipfsAPI.nftIpfsGateway?.absoluteString ?? "https://", text: $url)
          .font(.body)
          .keyboardType(.URL)
          .autocapitalization(.none)
          .disableAutocorrection(true)
          .foregroundColor(Color(.braveLabel))
          .onChange(of: url) { newValue in
            if newValue.count < 8 && "https://".hasPrefix(newValue) {
              self.url = "https://"
              setButtonStatus = .disabled
            } else {
              // The withSecureUrlScheme is used in order to force user to use secure url scheme
              // Instead of checking paste-board with every character entry, the textView text is analyzed
              // and according to what prefix copied or entered, text is altered to start with https://
              // this logic block repeating https:// and http:// schemes
              let textEntered = newValue.withSecureUrlScheme
              self.url = textEntered
              
              let oldValue = isForNFT ? ipfsAPI.nftIpfsGateway : ipfsAPI.ipfsGateway
              if let enteredURL = URL(string: textEntered), enteredURL != oldValue {
                setButtonStatus = .enabled
              } else {
                setButtonStatus = .disabled
              }
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(isForNFT ? Strings.Wallet.customizeIPFSNFTPublicGatewayNavTitle : Strings.Wallet.customizeIPFSPublicGatewayNavTitle)
    .navigationBarItems(
      // Have to use this instead of toolbar placement to have a custom button style
      trailing: Button(action: {
        validateURLAndSet()
      }) {
        if setButtonStatus == .loading {
          ProgressView()
        } else {
          Text(Strings.Wallet.setGatewayButtonTitle)
        }
      }
        .buttonStyle(BraveFilledButtonStyle(size: .small))
        .disabled(setButtonStatus != .enabled)
    )
    .alert(isPresented: $isPresentingWrongGatewayAlert) {
      Alert(
        title: Text(Strings.Wallet.wrongGatewayAlertTitle),
        message: Text(Strings.Wallet.wrongGatewayAlertDescription),
        dismissButton: .cancel(Text(Strings.OKString))
      )
    }
    .onAppear {
      // SwiftUI bug, has to wait a bit (#7044: bug only exists
      // in iOS 15. Will revisit once iOS 15 support is removed)
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
        if let url = isForNFT ? ipfsAPI.nftIpfsGateway?.absoluteString : ipfsAPI.ipfsGateway?.absoluteString {
          self.url = url
        }
      }
    }
  }
  
  private func validateURLAndSet() {
    Task { @MainActor in
      guard let enteredURL = URL(string: url), enteredURL.isSecureWebPage() else {
        isPresentingWrongGatewayAlert = true
        return
      }
      var testURL = enteredURL
      testURL.append(pathComponents: IPFSCustomGatewayView.ipfsTestPath, IPFSCustomGatewayView.ipfsTestCID)

      setButtonStatus = .loading
      resignFirstResponder()

      do {
        let (data, _) = try await URLSession.shared.data(from: testURL)
        if String(data: data, encoding: .utf8) == IPFSCustomGatewayView.ipfsTestContent {
          if isForNFT {
            ipfsAPI.nftIpfsGateway = enteredURL
          } else {
            ipfsAPI.ipfsGateway = enteredURL
          }
          setButtonStatus = .disabled
        } else {
          isPresentingWrongGatewayAlert = true
          setButtonStatus = .enabled
        }
      } catch {
        isPresentingWrongGatewayAlert = true
        setButtonStatus = .enabled
      }
    }
  }
  
  private func resignFirstResponder() {
    UIApplication.shared.sendAction(#selector(UIResponder.resignFirstResponder), to: nil, from: nil, for: nil)
  }
}
