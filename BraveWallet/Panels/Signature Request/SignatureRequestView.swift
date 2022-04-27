// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import struct Shared.Strings
import BraveShared
import BraveUI

struct SignatureRequestView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.presentationMode) @Binding private var presentationMode
  @ScaledMetric private var blockieSize = 54
  
  private enum ViewMode: Int {
    case message
    case data
  }
  
  @State private var viewMode: ViewMode = .message
  
  var body: some View {
    NavigationView {
      ScrollView(.vertical) {
        VStack {
          VStack(spacing: 8) {
            Blockie(address: keyringStore.selectedAccount.address)
              .frame(width: blockieSize, height: blockieSize)
            Text(keyringStore.selectedAccount.name)
              .font(.subheadline.weight(.semibold))
              .foregroundColor(Color(.secondaryBraveLabel))
            Text("Your signature is being requested")
              .font(.headline)
          }
          .padding(.vertical, 32)
          VStack(spacing: 12) {
            Picker("", selection: $viewMode) {
              Text("Message").tag(ViewMode.message)
              Text("Data").tag(ViewMode.data)
            }
            .pickerStyle(.segmented)
            Group {
              switch viewMode {
              case .message:
                StaticTextView(text: "To avoid digital cat burglars, sign below to authenticate with CryptoKitties.", isMonospaced: false)
                  .frame(maxWidth: .infinity)
                  .frame(height: 200)
                  .background(Color(.tertiaryBraveGroupedBackground))
                  .clipShape(RoundedRectangle(cornerRadius: 5, style: .continuous))
                  .padding()
              case .data:
                StaticTextView(text: "0x546F2061766F6964206469676974616C2063617420627572676C6172732C207369676E2062656C6F7720746F2061757468656E74696361746520776974682043727970746F4B6974746965732E")
                  .frame(maxWidth: .infinity)
                  .frame(height: 200)
                  .background(Color(.tertiaryBraveGroupedBackground))
                  .clipShape(RoundedRectangle(cornerRadius: 5, style: .continuous))
                  .padding()
              }
            }
            .frame(maxWidth: .infinity)
            .background(
              Color(.secondaryBraveGroupedBackground)
            )
            .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
          }
          buttonsContainer
            .padding(.top)
            .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
            .accessibility(hidden: sizeCategory.isAccessibilityCategory)
        }
        .padding()
      }
      .overlay(
        Group {
          if sizeCategory.isAccessibilityCategory {
            buttonsContainer
              .frame(maxWidth: .infinity)
              .padding(.top)
              .background(
                LinearGradient(
                  stops: [
                    .init(color: Color(.braveGroupedBackground).opacity(0), location: 0),
                    .init(color: Color(.braveGroupedBackground).opacity(1), location: 0.05),
                    .init(color: Color(.braveGroupedBackground).opacity(1), location: 1),
                  ],
                  startPoint: .top,
                  endPoint: .bottom
                )
                  .ignoresSafeArea()
                  .allowsHitTesting(false)
              )
          }
        },
        alignment: .bottom
      )
      .frame(maxWidth: .infinity)
      .navigationTitle("Signature Requested")
      .navigationBarTitleDisplayMode(.inline)
      .foregroundColor(Color(.braveLabel))
      .background(Color(.braveGroupedBackground).edgesIgnoringSafeArea(.all))
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: { presentationMode.dismiss() }) {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
    .navigationViewStyle(.stack)
  }
  
  @ViewBuilder private var buttonsContainer: some View {
    if sizeCategory.isAccessibilityCategory {
      VStack {
        buttons
      }
    } else {
      HStack {
        buttons
      }
    }
  }
  
  @ViewBuilder private var buttons: some View {
//    Button(action: {
//
//    }) {
//      Text(Strings.cancelButtonTitle)
//    }
//    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    Button(action: {
      
    }) {
      Label("Sign", image: "brave.key")
        .imageScale(.large)
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
  }
}

#if DEBUG
struct SignatureRequestView_Previews: PreviewProvider {
  static var previews: some View {
    SignatureRequestView(keyringStore: .previewStoreWithWalletCreated)
  }
}
#endif
