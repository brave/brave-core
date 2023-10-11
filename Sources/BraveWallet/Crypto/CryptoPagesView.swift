/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import SwiftUI
import BraveCore
import PanModal
import BraveUI
import Strings

struct CryptoPagesView: View {
  @ObservedObject var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore

  @State private var isShowingMainMenu: Bool = false
  @State private var isShowingSettings: Bool = false
  @State private var isShowingSearch: Bool = false
  @State private var fetchedPendingRequestsThisSession: Bool = false
  @State private var selectedPageIndex: Int = 0
  
  private var isConfirmationButtonVisible: Bool {
    if case .transactions(let txs) = cryptoStore.pendingRequest {
      return !txs.isEmpty
    }
    return cryptoStore.pendingRequest != nil
  }

  var body: some View {
    _CryptoPagesView(
      keyringStore: keyringStore,
      cryptoStore: cryptoStore,
      isShowingPendingRequest: $cryptoStore.isPresentingPendingRequest,
      isConfirmationsButtonVisible: isConfirmationButtonVisible,
      selectedIndexChanged: { selectedIndex in
        selectedPageIndex = selectedIndex
      }
    )
    .onAppear {
      // If a user chooses not to confirm/reject their requests we shouldn't
      // do it again until they close and re-open wallet
      if !fetchedPendingRequestsThisSession {
        // Give the animation time
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
          self.fetchedPendingRequestsThisSession = true
          self.cryptoStore.prepare(isInitialOpen: true)
        }
      }
    }
    .ignoresSafeArea()
    .navigationTitle(Strings.Wallet.cryptoTitle)
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController(customize: { vc in
      vc.navigationItem.do {
        let appearance: UINavigationBarAppearance = {
          let appearance = UINavigationBarAppearance()
          appearance.configureWithOpaqueBackground()
          appearance.titleTextAttributes = [.foregroundColor: UIColor.braveLabel]
          appearance.largeTitleTextAttributes = [.foregroundColor: UIColor.braveLabel]
          appearance.backgroundColor = .braveBackground
          appearance.shadowColor = .clear
          return appearance
        }()
        $0.standardAppearance = appearance
        $0.compactAppearance = appearance
        $0.scrollEdgeAppearance = appearance
      }
    })
    .background(
      NavigationLink(
        destination: Web3SettingsView(
          settingsStore: cryptoStore.settingsStore,
          networkStore: cryptoStore.networkStore,
          keyringStore: keyringStore
        ),
        isActive: $isShowingSettings
      ) {
        Text(Strings.Wallet.settings)
      }
      .hidden()
    )
    .background(
      Color.clear
        .sheet(isPresented: $cryptoStore.isPresentingAssetSearch) {
          AssetSearchView(
            keyringStore: keyringStore,
            cryptoStore: cryptoStore,
            userAssetsStore: cryptoStore.portfolioStore.userAssetsStore
          )
        }
    )
    .toolbar {
      ToolbarItemGroup(placement: .navigationBarTrailing) {
        Button(action: {
          cryptoStore.isPresentingAssetSearch = true
        }) {
          Label(Strings.Wallet.searchTitle, systemImage: "magnifyingglass")
            .labelStyle(.iconOnly)
            .foregroundColor(Color(.braveBlurpleTint))
        }
        Button(action: { self.isShowingMainMenu = true }) {
          Label(Strings.Wallet.otherWalletActionsAccessibilityTitle, braveSystemImage: "leo.more.horizontal")
            .labelStyle(.iconOnly)
            .foregroundColor(Color(.braveBlurpleTint))
        }
        .accessibilityLabel(Strings.Wallet.otherWalletActionsAccessibilityTitle)
      }
    }
    .sheet(isPresented: $isShowingMainMenu) {
      MainMenuView(
        isFromPortfolio: selectedPageIndex == 0,
        isShowingSettings: $isShowingSettings,
        keyringStore: keyringStore
      )
    }
  }

  private struct _CryptoPagesView: UIViewControllerRepresentable {
    var keyringStore: KeyringStore
    var cryptoStore: CryptoStore
    var isShowingPendingRequest: Binding<Bool>
    var isConfirmationsButtonVisible: Bool
    var selectedIndexChanged: (Int) -> Void

    func makeUIViewController(context: Context) -> CryptoPagesViewController {
      CryptoPagesViewController(
        keyringStore: keyringStore,
        cryptoStore: cryptoStore,
        isShowingPendingRequest: isShowingPendingRequest,
        selectedIndexChanged: selectedIndexChanged
      )
    }
    func updateUIViewController(_ uiViewController: CryptoPagesViewController, context: Context) {
      uiViewController.pendingRequestsButton.isHidden = !isConfirmationsButtonVisible
    }
  }
}

private class CryptoPagesViewController: TabbedPageViewController {
  private let keyringStore: KeyringStore
  private let cryptoStore: CryptoStore
  let pendingRequestsButton = ConfirmationsButton()
  let selectedIndexChanged: (Int) -> Void

  @Binding private var isShowingPendingRequest: Bool

  init(
    keyringStore: KeyringStore,
    cryptoStore: CryptoStore,
    isShowingPendingRequest: Binding<Bool>,
    selectedIndexChanged: @escaping (Int) -> Void
  ) {
    self.keyringStore = keyringStore
    self.cryptoStore = cryptoStore
    self._isShowingPendingRequest = isShowingPendingRequest
    self.selectedIndexChanged = selectedIndexChanged
    super.init(selectedIndexChanged: selectedIndexChanged)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.Wallet.cryptoTitle
    navigationItem.largeTitleDisplayMode = .never
    view.backgroundColor = .braveGroupedBackground

    pages = [
      UIHostingController(
        rootView: PortfolioView(
          cryptoStore: cryptoStore,
          keyringStore: keyringStore,
          networkStore: cryptoStore.networkStore,
          portfolioStore: cryptoStore.portfolioStore
        )
      ).then {
        $0.title = Strings.Wallet.portfolioPageTitle
      },
      UIHostingController(
        rootView: TransactionsActivityView(
          store: cryptoStore.transactionsActivityStore,
          networkStore: cryptoStore.networkStore
        )
      ).then {
        $0.title = Strings.Wallet.activityPageTitle
      },
      UIHostingController(
        rootView: AccountsView(
          cryptoStore: cryptoStore,
          keyringStore: keyringStore
        )
      ).then {
        $0.title = Strings.Wallet.accountsPageTitle
      },
      UIHostingController(
        rootView: MarketView(
          cryptoStore: cryptoStore,
          keyringStore: keyringStore
        )
      ).then {
        $0.title = Strings.Wallet.marketPageTitle
      },
    ]

    view.addSubview(pendingRequestsButton)
    pendingRequestsButton.snp.makeConstraints {
      $0.trailing.equalToSuperview().inset(16)
      $0.bottom.equalTo(view.safeAreaLayoutGuide).priority(.high)
      $0.bottom.lessThanOrEqualTo(view).inset(8)
    }
    pendingRequestsButton.addTarget(self, action: #selector(tappedPendingRequestsButton), for: .touchUpInside)
  }

  @objc private func tappedPendingRequestsButton() {
    isShowingPendingRequest = true
  }
}

private class ConfirmationsButton: SpringButton {
  private let imageView = UIImageView(
    image: UIImage(braveSystemNamed: "leo.notification.dot")!
      .applyingSymbolConfiguration(.init(pointSize: 18))
  ).then {
    $0.tintColor = .white
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .braveBlurpleTint
    addSubview(imageView)

    imageView.snp.makeConstraints {
      $0.center.equalToSuperview()
    }
    snp.makeConstraints {
      $0.width.equalTo(snp.height)
    }

    layer.shadowColor = UIColor.black.cgColor
    layer.shadowOffset = .init(width: 0, height: 1)
    layer.shadowRadius = 1
    layer.shadowOpacity = 0.3

    accessibilityLabel = Strings.Wallet.confirmTransactionsTitle
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    layer.cornerRadius = bounds.height / 2.0
    layer.shadowPath = UIBezierPath(ovalIn: bounds).cgPath
  }

  override var intrinsicContentSize: CGSize {
    .init(width: 36, height: 36)
  }
}
