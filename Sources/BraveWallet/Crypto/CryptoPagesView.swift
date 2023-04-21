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

  @State private var isShowingSettings: Bool = false
  @State private var isShowingSearch: Bool = false
  @State private var fetchedPendingRequestsThisSession: Bool = false

  @Environment(\.openURL) private var openWalletURL
  
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
      isConfirmationsButtonVisible: isConfirmationButtonVisible
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
        Menu {
          Button(action: {
            keyringStore.lock()
          }) {
            Label(Strings.Wallet.lock, braveSystemImage: "leo.lock")
              .imageScale(.medium)  // Menu inside nav bar implicitly gets large
          }
          Button(action: { isShowingSettings = true }) {
            Label(Strings.Wallet.settings, braveSystemImage: "leo.settings")
              .imageScale(.medium)  // Menu inside nav bar implicitly gets large
          }
          Divider()
          Button(action: { openWalletURL(WalletConstants.braveWalletSupportURL) }) {
            Label(Strings.Wallet.helpCenter, braveSystemImage: "leo.info.outline")
          }
        } label: {
          Label(Strings.Wallet.otherWalletActionsAccessibilityTitle, systemImage: "ellipsis.circle")
            .labelStyle(.iconOnly)
            .foregroundColor(Color(.braveBlurpleTint))
        }
        .accessibilityLabel(Strings.Wallet.otherWalletActionsAccessibilityTitle)
      }
    }
  }

  private struct _CryptoPagesView: UIViewControllerRepresentable {
    var keyringStore: KeyringStore
    var cryptoStore: CryptoStore
    var isShowingPendingRequest: Binding<Bool>
    var isConfirmationsButtonVisible: Bool

    func makeUIViewController(context: Context) -> CryptoPagesViewController {
      CryptoPagesViewController(
        keyringStore: keyringStore,
        cryptoStore: cryptoStore,
        buySendSwapDestination: context.environment.buySendSwapDestination,
        isShowingPendingRequest: isShowingPendingRequest
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
  private let swapButton = SwapButton()
  let pendingRequestsButton = ConfirmationsButton()

  @Binding private var buySendSwapDestination: BuySendSwapDestination?
  @Binding private var isShowingPendingRequest: Bool

  init(
    keyringStore: KeyringStore,
    cryptoStore: CryptoStore,
    buySendSwapDestination: Binding<BuySendSwapDestination?>,
    isShowingPendingRequest: Binding<Bool>
  ) {
    self.keyringStore = keyringStore
    self.cryptoStore = cryptoStore
    self._buySendSwapDestination = buySendSwapDestination
    self._isShowingPendingRequest = isShowingPendingRequest
    super.init(nibName: nil, bundle: nil)
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
    ]

    view.addSubview(swapButton)
    swapButton.snp.makeConstraints {
      $0.centerX.equalToSuperview()
      $0.bottom.equalTo(view.safeAreaLayoutGuide).priority(.high)
      $0.bottom.lessThanOrEqualTo(view).inset(8)
    }

    pages.forEach {
      $0.additionalSafeAreaInsets = .init(top: 0, left: 0, bottom: swapButton.intrinsicContentSize.height + 8, right: 0)
    }

    swapButton.addTarget(self, action: #selector(tappedSwapButton), for: .touchUpInside)

    view.addSubview(pendingRequestsButton)
    pendingRequestsButton.snp.makeConstraints {
      $0.trailing.equalToSuperview().inset(16)
      $0.centerY.equalTo(swapButton)
      $0.bottom.lessThanOrEqualTo(view).inset(8)
    }
    pendingRequestsButton.addTarget(self, action: #selector(tappedPendingRequestsButton), for: .touchUpInside)
  }

  @objc private func tappedPendingRequestsButton() {
    isShowingPendingRequest = true
  }

  @objc private func tappedSwapButton() {
    let controller = FixedHeightHostingPanModalController(
      rootView: BuySendSwapView(
        networkStore: cryptoStore.networkStore,
        action: { [weak self] destination in
          self?.dismiss(
            animated: true,
            completion: {
              self?.buySendSwapDestination = destination
            })
        })
    )
    presentPanModal(
      controller,
      sourceView: swapButton,
      sourceRect: swapButton.bounds
    )
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
