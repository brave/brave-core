/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import SwiftUI
import BraveCore
import Introspect
import BraveUI

@available(iOS 14.0, *)
public struct CryptoView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: EthNetworkStore
  
  // in iOS 15, PresentationMode will be available in SwiftUI hosted by UIHostingController
  // but for now we'll have to manage this ourselves
  var dismissAction: (() -> Void)?
  
  public init(keyringStore: KeyringStore, networkStore: EthNetworkStore) {
    self.keyringStore = keyringStore
    self.networkStore = networkStore
  }
  
  private var isShowingUnlockView: Bool {
    keyringStore.keyring.isLocked
  }
  
  private var isShowingOnboarding: Bool {
    !keyringStore.keyring.isDefaultKeyringCreated || keyringStore.isOnboardingVisible
  }
  
  @ToolbarContentBuilder
  private var dismissButtonToolbarContents: some ToolbarContent {
    ToolbarItemGroup(placement: .cancellationAction) {
      Button(action: { dismissAction?() }) {
        Image("wallet-dismiss")
          .renderingMode(.template)
      }
    }
  }
  
  public var body: some View {
    ZStack {
      if !isShowingUnlockView {
        UIKitController(
          UINavigationController(
            rootViewController: CryptoPagesViewController(
              keyringStore: keyringStore,
              networkStore: networkStore
            )
          )
        )
        .transition(.identity)
      }
      if isShowingUnlockView {
        NavigationView {
          UIKitScrollView(axis: .vertical) {
            UnlockWalletView(keyringStore: keyringStore)
          }
          .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
          .navigationTitle("Crypto") // NSLocalizedString
          .navigationBarTitleDisplayMode(.inline)
          .toolbar {
            dismissButtonToolbarContents
          }
        }
        .navigationViewStyle(StackNavigationViewStyle())
        .transition(.move(edge: .bottom))
        .zIndex(1)  // Needed or the dismiss animation messes up
      }
      if isShowingOnboarding {
        NavigationView {
          SetupCryptoView(keyringStore: keyringStore)
            .toolbar {
              dismissButtonToolbarContents
            }
        }
        .navigationViewStyle(StackNavigationViewStyle())
        .transition(.move(edge: .bottom))
        .zIndex(2)  // Needed or the dismiss animation messes up
      }
    }
    .animation(.default, value: isShowingUnlockView) // Animate unlock dismiss (required for some reason)
    .animation(.default, value: isShowingOnboarding) // Animate onboarding dismiss (required for some reason)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .onDisappear {
      keyringStore.lock()
    }
  }
}
