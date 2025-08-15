// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SwiftUI

private struct SharedConstants {
  static let defaultGridItemWidth: CGFloat = 108
}

struct OnboardingNetworkSelectionView: View {

  var keyringStore: KeyringStore
  let setupOption: OnboardingSetupOption
  // Used to dismiss all of Wallet
  let dismissAction: () -> Void
  /// All available networks
  @State var networks: [Selectable<BraveWallet.NetworkInfo>]
  /// If we are fetching networks
  @State private var isLoading: Bool = false
  /// If we are showing testnets section
  @State private var isShowingTestnets: Bool = false {
    didSet {
      guard !isShowingTestnets else { return }

    }
  }
  @State private var isShowingCreateNewWallet: Bool = false
  @State private var isShowingRestoreExistedWallet: Bool = false
  @ScaledMetric private var gridItemWidth: CGFloat = SharedConstants.defaultGridItemWidth

  init(
    keyringStore: KeyringStore,
    setupOption: OnboardingSetupOption,
    dismissAction: @escaping () -> Void,
    networks: [Selectable<BraveWallet.NetworkInfo>] = []
  ) {
    self.keyringStore = keyringStore
    self.setupOption = setupOption
    self.dismissAction = dismissAction
    self._networks = State(wrappedValue: networks)
  }

  private var featuredNetworks: [Selectable<BraveWallet.NetworkInfo>] {
    networks.filter { selectableNetwork in
      selectableNetwork.model.isPrimaryNetwork
    }.sorted(by: networkSort)
  }

  private var popularNetworks: [Selectable<BraveWallet.NetworkInfo>] {
    networks.filter { selectableNetwork in
      let isPrimaryNetwork = selectableNetwork.model.isPrimaryNetwork
      let isTestNetwork = selectableNetwork.model.isKnownTestnet
      return !isPrimaryNetwork && !isTestNetwork
    }.sorted(by: networkSort)
  }

  private var testnets: [Selectable<BraveWallet.NetworkInfo>] {
    networks.filter { selectableNetwork in
      selectableNetwork.model.isKnownTestnet
    }.sorted(by: networkSort)
  }

  /// Sorts mandatory networks first, then coin sort order, then sort by chain name.
  /// This groups mandatory networks at the front, and groups network coins together.
  private func networkSort(
    lhs: Selectable<BraveWallet.NetworkInfo>,
    rhs: Selectable<BraveWallet.NetworkInfo>
  ) -> Bool {
    if lhs.model.isMandatoryNetwork && !rhs.model.isMandatoryNetwork {
      // mandatory to the front
      return true
    } else if lhs.model.coin != rhs.model.coin {
      // if coins don't match, sort by coin sortOrder
      return lhs.model.coin.sortOrder < rhs.model.coin.sortOrder
    }
    // otherwise, sort by chainName
    return lhs.model.chainName < rhs.model.chainName
  }

  var body: some View {
    ScrollView {
      VStack(spacing: 0) {
        Text(Strings.Wallet.onboardingNetworkSelectionTitle)
          .font(.title.weight(.bold))
          .foregroundColor(Color(braveSystemName: .textPrimary))
        Spacer(minLength: 16)
        Text(Strings.Wallet.onboardingNetworkSelectionDescription)
          .font(.body)
          .foregroundColor(Color(braveSystemName: .textSecondary))
          .multilineTextAlignment(.center)
        Spacer(minLength: 16)
        Toggle(
          isOn: $isShowingTestnets,
          label: {
            Text(Strings.Wallet.showTestnets)
          }
        )
        .toggleStyle(SwitchToggleStyle(tint: .accentColor))
        .fixedSize(horizontal: true, vertical: false)
        .frame(maxWidth: .infinity, alignment: .trailing)
        LazyVGrid(
          columns: [
            GridItem(
              .adaptive(minimum: gridItemWidth, maximum: gridItemWidth),
              alignment: .top
            )
          ],
          alignment: .leading,
          spacing: 8,
          pinnedViews: [.sectionHeaders],
          content: {
            networksSection(
              for: featuredNetworks,
              title: Strings.Wallet.featured,
              showsSelectAllButton: false
            )
            if !popularNetworks.isEmpty {
              networksSection(
                for: popularNetworks,
                title: Strings.Wallet.popular,
                showsSelectAllButton: !isLoading
              )
            }
            if isShowingTestnets && !testnets.isEmpty {
              networksSection(
                for: testnets,
                title: Strings.Wallet.testnets,
                showsSelectAllButton: !isLoading
              )
            }
          }
        )

      }
      .padding()
    }
    .background(Color(braveSystemName: .containerBackground))
    .overlay(alignment: .top) {
      // sticky section headers don't enter top safe area
      Color(braveSystemName: .containerBackground)
        .ignoresSafeArea()
        .frame(height: 0)
    }
    .safeAreaInset(edge: .bottom) {
      continueButton
    }
    .transparentUnlessScrolledNavigationAppearance()
    .background(
      NavigationLink(
        destination: CreateWalletView(
          keyringStore: keyringStore,
          setupSelections: .init(
            setupOption: setupOption,
            networks: networks
          ),
          dismissAction: dismissAction
        ),
        isActive: $isShowingCreateNewWallet,
        label: {
          EmptyView()
        }
      )
    )
    .background(
      NavigationLink(
        destination: RestoreWalletView(
          keyringStore: keyringStore,
          setupSelections: .init(
            setupOption: setupOption,
            networks: networks
          ),
          dismissAction: dismissAction
        ),
        isActive: $isShowingRestoreExistedWallet,
        label: {
          EmptyView()
        }
      )
    )
    .task {
      guard networks.isEmpty else { return }
      self.networks = await keyringStore.onboardingNetworks()
    }
  }

  // MARK: Subviews

  private func networksSection(
    for networks: [Selectable<BraveWallet.NetworkInfo>],
    title: String,
    showsSelectAllButton: Bool
  ) -> some View {
    Section(
      content: {
        if isLoading {
          LoadingGridItemView()
          LoadingGridItemView()
          LoadingGridItemView()
        } else {
          ForEach(networks) { selectableNetwork in
            NetworkGridItemView(
              network: selectableNetwork,
              didSelect: didSelect(network:)
            )
          }
        }
      },
      header: {
        SelectAllHeaderView(
          title: title,
          showsSelectAllButton: showsSelectAllButton,
          verticalPadding: 6,
          allModels: networks,
          selectedModels: networks.filter(\.isSelected),
          select: { selectableNetwork in
            didSelect(network: selectableNetwork.model)
          }
        )
        .background(Color(braveSystemName: .containerBackground))
        .transaction { transaction in
          transaction.disablesAnimations = true
        }
      }
    )
  }

  private var continueButton: some View {
    Button(
      action: {
        if setupOption == .new {
          isShowingCreateNewWallet = true
        } else {
          isShowingRestoreExistedWallet = true
        }
      },
      label: {
        Text(
          String.localizedStringWithFormat(
            Strings.Wallet.onboardingNetworkSelectionContinue,
            networks.filter(\.isSelected).count
          )
        )
      }
    )
    .buttonStyle(BraveFilledButtonStyle(size: .large))
    .frame(maxWidth: .infinity)
    .padding(.top)
    .background(
      LinearGradient(
        stops: [
          .init(
            color: Color(braveSystemName: .containerBackground).opacity(0),
            location: 0
          ),
          .init(
            color: Color(braveSystemName: .containerBackground).opacity(1),
            location: 0.05
          ),
          .init(
            color: Color(braveSystemName: .containerBackground).opacity(1),
            location: 1
          ),
        ],
        startPoint: .top,
        endPoint: .bottom
      )
      .ignoresSafeArea()
      .allowsHitTesting(false)
    )
  }

  // MARK: Helper functions

  private func didSelect(network: BraveWallet.NetworkInfo) {
    guard
      !network.isMandatoryNetwork,
      let networkIndex = networks.firstIndex(
        where: { $0.model.chainId == network.chainId }
      ),
      let selectableNetwork = networks[safe: networkIndex]
    else {
      return
    }
    networks[networkIndex] = .init(
      isSelected: !selectableNetwork.isSelected,
      model: network
    )
  }

  private func deselectTestNetworks() {
    for (index, network) in networks.enumerated() where network.model.isKnownTestnet {
      networks[index] = .init(isSelected: false, model: network.model)
    }
  }
}

#if DEBUG
#Preview {
  NavigationView {
    OnboardingNetworkSelectionView(
      keyringStore: .previewStore,
      setupOption: .new,
      dismissAction: {},
      networks: [
        // mandatory networks get sorted to front
        .mockFilecoinMainnet,
        .mockBitcoinMainnet,
        .mockMainnet,
        .mockSolana,
        .mockPolygon,
        .mockSepolia,
        .mockSolanaTestnet,
        .mockFilecoinTestnet,
        .mockBitcoinTestnet,
      ].map {
        .init(isSelected: !$0.isKnownTestnet, model: $0)
      }
    )
  }
  .accentColor(Color(.braveBlurple))
}
#endif

private struct SelectableGridItemView<Content: View, Item: Identifiable & Equatable>: View {

  let item: Selectable<Item>
  let isSelectable: Bool
  @ViewBuilder let content: (Selectable<Item>) -> Content
  let didSelect: (Item) -> Void

  @ScaledMetric private var width: CGFloat = SharedConstants.defaultGridItemWidth
  @ScaledMetric private var height: CGFloat = 96

  var body: some View {
    Group {
      if isSelectable {
        Button(
          action: {
            didSelect(item.model)
          },
          label: {
            content(item)
          }
        )
        .buttonStyle(.plain)
      } else {
        content(item)
      }
    }
    .padding(12)
    .frame(width: width, height: height)
    .overlay(alignment: .topTrailing) {
      WalletCheckbox(
        isChecked: Binding(
          get: { item.isSelected },
          set: { _ in
            guard isSelectable else { return }
            didSelect(item.model)
          }
        ),
        colorOverride: isSelectable ? nil : UIColor.braveDisabled
      )
      .padding(.top, 8)
      .padding(.trailing, 8)
      .disabled(!isSelectable)
    }
    .overlay {
      ContainerRelativeShape()
        .strokeBorder(
          Color(
            braveSystemName: item.isSelected && isSelectable
              ? .buttonBackground : .dividerSubtle
          )
        )
    }
    .containerShape(RoundedRectangle(cornerRadius: 8))
  }
}

private struct LoadingGridItemView: View {

  private struct LoadingItem: Identifiable, Equatable {
    let id = UUID()
  }

  var body: some View {
    SelectableGridItemView(
      item: Selectable(isSelected: false, model: LoadingItem()),
      isSelectable: false,
      content: { _ in
        VStack {
          Circle()
            .fill(Color(white: 0.9))
          Text("Loading text...")
            .multilineTextAlignment(.center)
            .redacted(reason: .placeholder)
        }
      },
      didSelect: { _ in }
    )
    .shimmer(true)
  }
}

private struct NetworkGridItemView: View {

  let network: Selectable<BraveWallet.NetworkInfo>
  let didSelect: (BraveWallet.NetworkInfo) -> Void

  var body: some View {
    SelectableGridItemView(
      item: network,
      isSelectable: !WalletConstants.mandatoryNetworkChainIds.contains(network.model.chainId),
      content: { network in
        VStack {
          NetworkIconView(
            network: network.model,
            length: 24,
            maxLength: 24
          )
          Text(network.model.chainName)
            .multilineTextAlignment(.center)
            .minimumScaleFactor(0.75)
            .allowsTightening(true)
            .foregroundColor(Color(braveSystemName: .textPrimary))
        }
      },
      didSelect: didSelect
    )
  }
}
