// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Shared
import Strings
import SwiftUI
import UIKit

struct PortfolioSegmentedControl: View {

  enum Item: Int, Equatable, CaseIterable, Identifiable, WalletSegmentedControlItem {
    case assets
    case nfts

    var title: String {
      switch self {
      case .assets: return Strings.Wallet.assetsTitle
      case .nfts: return Strings.Wallet.nftsTitle
      }
    }

    var id: Int { rawValue }
  }

  @Binding var selected: Item

  var body: some View {
    WalletSegmentedControl(
      items: Item.allCases,
      selected: $selected
    )
  }
}

#if DEBUG
struct PortfolioSegmentedControl_Previews: PreviewProvider {
  static var previews: some View {
    PortfolioSegmentedControl(
      selected: .constant(.nfts)
    )
  }
}
#endif

protocol WalletSegmentedControlItem: Equatable, Hashable, Identifiable {
  var title: String { get }
}

struct WalletSegmentedControl<Item: WalletSegmentedControlItem>: View {

  let items: [Item]
  @Binding var selected: Item
  let dynamicTypeRange = (...DynamicTypeSize.xxxLarge)

  var minHeight: CGFloat = 40
  @ScaledMetric var height: CGFloat = 40
  var maxHeight: CGFloat = 60

  @State private var viewSize: CGSize = .zero
  @State private var location: CGPoint = .zero
  @GestureState private var isDragGestureActive: Bool = false

  var body: some View {
    RoundedRectangle(cornerRadius: 8)
      .fill(Color(braveSystemName: .containerHighlight))
      .overlay {  // selected item
        RoundedRectangle(cornerRadius: 8)
          .fill(Color(braveSystemName: .containerBackground))
          .padding(4)
          .frame(width: itemWidth)
          .position(location)
      }
      .overlay {  // text for each item
        HStack {
          ForEach(items) { item in
            titleView(for: item)

            if item != items.last {
              Spacer()
            }
          }
        }
      }
      .readSize { size in
        viewSize = size
      }
      .frame(height: min(max(height, minHeight), maxHeight))
      .gesture(dragGesture)
      .onChange(of: isDragGestureActive) { isDragGestureActive in
        if !isDragGestureActive {  // cancellation of gesture, ex while scrolling
          if let itemForLocation = item(for: location) {
            select(itemForLocation)
          }
        }
      }
      .onChange(of: viewSize) { viewSize in
        if location == .zero {
          // set initial location
          select(selected, animated: false)
        } else if !isDragGestureActive {
          // possible when accessibility size changes
          select(selected, animated: false)
        }
      }
      .simultaneousGesture(tapGesture)
      .accessibilityRepresentation {
        Picker(selection: $selected) {
          ForEach(items) { item in
            Text(item.title).tag(item.id)
          }
        } label: {
          EmptyView()
        }
        .pickerStyle(.segmented)
      }
  }

  private func select(_ item: Item, animated: Bool = true) {
    selected = item
    withAnimation(animated ? .spring() : nil) {
      location = location(for: item)
    }
  }

  private func titleView(for item: Item) -> some View {
    Text(item.title)
      .font(.subheadline.weight(.semibold))
      .foregroundColor(Color(braveSystemName: selected == item ? .textPrimary : .textSecondary))
      .dynamicTypeSize(dynamicTypeRange)
      .allowsHitTesting(false)
      .frame(width: itemWidth)
      .hoverEffect()
  }

  private var dragGesture: some Gesture {
    DragGesture()
      .updating($isDragGestureActive) { value, state, transaction in
        state = true
      }
      .onChanged { value in
        // `location` is the middle of RoundedRectangle
        let minX = itemWidth / 2
        let maxX = viewSize.width - minX
        let newX = min(max(value.location.x, minX), maxX)
        location = CGPoint(
          x: newX,
          y: location.y
        )
      }
      .onEnded { value in
        if let itemForLocation = item(for: value.predictedEndLocation) {
          select(itemForLocation)
        }
      }
  }

  @available(iOS 16, *)
  private var tapGesture: some Gesture {
    SpatialTapGesture()
      .onEnded { value in
        if let itemForLocation = item(for: value.location) {
          select(itemForLocation)
        }
      }
  }

  private var itemWidth: CGFloat {
    viewSize.width / CGFloat(items.count)
  }

  private func location(for item: Item) -> CGPoint {
    CGPoint(
      x: xPosition(for: item),
      y: viewSize.height / 2
    )
  }

  private func xPosition(for item: Item) -> CGFloat {
    let itemWidth = viewSize.width / CGFloat(items.count)
    let firstItemPosition = itemWidth / 2
    let selectedIndex = items.firstIndex(of: item) ?? 0
    let newX = firstItemPosition + (CGFloat(selectedIndex) * itemWidth)
    return newX
  }

  private func item(for location: CGPoint) -> Item? {
    let percent = location.x / viewSize.width
    let itemIndex = Int(percent * CGFloat(items.count))
    return items[safe: itemIndex]
  }
}

// https://www.fivestars.blog/articles/swiftui-share-layout-information/
struct SizePreferenceKey: PreferenceKey {
  static var defaultValue: CGSize = .zero
  static func reduce(value: inout CGSize, nextValue: () -> CGSize) {}
}

extension View {
  func readSize(onChange: @escaping (CGSize) -> Void) -> some View {
    background(
      GeometryReader { geometryProxy in
        Color.clear
          .preference(key: SizePreferenceKey.self, value: geometryProxy.size)
      }
    )
    .onPreferenceChange(SizePreferenceKey.self, perform: onChange)
  }
}
