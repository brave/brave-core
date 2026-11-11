// Copyright 2026 The Brave Authors. All rights reserved.
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
  @State private var isShowingTestnets: Bool = false
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
        .tint(Color(braveSystemName: .primitivePrimary40))
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
    .navigationDestination(
      isPresented: $isShowingCreateNewWallet,
      destination: {
        CreateWalletView(
          keyringStore: keyringStore,
          setupSelections: .init(
            setupOption: setupOption,
            networks: networks
          ),
          dismissAction: dismissAction
        )
      }
    )
    .navigationDestination(
      isPresented: $isShowingRestoreExistedWallet,
      destination: {
        RestoreWalletView(
          keyringStore: keyringStore,
          setupSelections: .init(
            setupOption: setupOption,
            networks: networks
          ),
          dismissAction: dismissAction
        )
      }
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
              network: selectableNetwork.model,
              isSelected: self.$networks[isSelected: selectableNetwork.id]
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
            self.networks[isSelected: selectableNetwork.id].toggle()
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
    .buttonStyle(.filled)
    .controlSize(.large)
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

  private func deselectTestNetworks() {
    for (index, network) in networks.enumerated() where network.model.isKnownTestnet {
      networks[index] = .init(isSelected: false, model: network.model)
    }
  }
}

#if DEBUG
#Preview {
  NavigationStack {
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
  .accentColor(Color(braveSystemName: .primitivePrimary40))
}
#endif

extension Array where Element == Selectable<BraveWallet.NetworkInfo> {
  /// Whether or not the network matching `id` is selected. Mandatory networks cannot be deselected.
  fileprivate subscript(isSelected id: BraveWallet.NetworkInfo.ID) -> Bool {
    get { first(where: { $0.id == id })?.isSelected ?? false }
    set {
      guard let index = firstIndex(where: { $0.id == id }),
        !self[index].model.isMandatoryNetwork
      else {
        return
      }
      self[index] = .init(isSelected: newValue, model: self[index].model)
    }
  }
}

private struct SelectableGridItemView<Content: View, Item: Identifiable & Equatable>: View {

  let item: Item
  @Binding var isSelected: Bool
  let isSelectable: Bool
  @ViewBuilder let content: (Item) -> Content

  @ScaledMetric private var width: CGFloat = SharedConstants.defaultGridItemWidth
  @ScaledMetric private var height: CGFloat = 96

  var body: some View {
    Group {
      if isSelectable {
        Button(
          action: {
            isSelected.toggle()
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
      Toggle(isOn: $isSelected) {
        EmptyView()
      }
      .padding([.top, .trailing], 8)
      .disabled(!isSelectable)
      .toggleStyle(CheckboxToggleStyle())
    }
    .overlay {
      ContainerRelativeShape()
        .strokeBorder(
          Color(
            braveSystemName: isSelected && isSelectable
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
      item: LoadingItem(),
      isSelected: .constant(false),
      isSelectable: false,
      content: { _ in
        VStack {
          Circle()
            .fill(Color(white: 0.9))
          Text("Loading text...")
            .multilineTextAlignment(.center)
            .redacted(reason: .placeholder)
        }
      }
    )
    .shimmer(true)
  }
}

private struct NetworkGridItemView: View {

  let network: BraveWallet.NetworkInfo
  @Binding var isSelected: Bool

  var body: some View {
    SelectableGridItemView(
      item: network,
      isSelected: $isSelected,
      isSelectable: !network.isMandatoryNetwork,
      content: { network in
        VStack {
          NetworkIconView(
            network: network,
            length: 24,
            maxLength: 24
          )
          Text(network.chainName)
            .multilineTextAlignment(.center)
            .minimumScaleFactor(0.75)
            .allowsTightening(true)
            .foregroundColor(Color(braveSystemName: .textPrimary))
        }
      }
    )
  }
}
