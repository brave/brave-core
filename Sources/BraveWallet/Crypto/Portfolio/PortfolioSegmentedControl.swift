/* Copyright 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SwiftUI
import BraveCore
import Strings
import DesignSystem
import BraveUI
import Shared

struct PortfolioSegmentedControl: View {
  
  enum SelectedContent: Int, Equatable, CaseIterable, Identifiable {
    case assets
    case nfts
    
    var displayText: String {
      switch self {
      case .assets: return Strings.Wallet.assetsTitle
      case .nfts: return Strings.Wallet.nftsTitle
      }
    }
    
    var id: Int { rawValue }
  }
  
  @Binding var selected: SelectedContent
  @State private var viewSize: CGSize = .zero
  @State private var location: CGPoint = .zero
  @GestureState private var isDragGestureActive: Bool = false
  
  var body: some View {
    GeometryReader { geometryProxy in
      Capsule()
        .fill(Color(braveSystemName: .containerHighlight))
        .osAvailabilityModifiers {
          if #unavailable(iOS 16) {
            $0.overlay {
              // TapGesture does not give a location,
              // SpatialTapGesture is iOS 16+.
              HStack {
                Color.clear
                  .contentShape(Rectangle())
                  .onTapGesture {
                    select(.assets)
                  }
                Color.clear
                  .contentShape(Rectangle())
                  .onTapGesture {
                    select(.nfts)
                  }
              }
            }
          } else {
            $0
          }
        }
        .overlay {
          Capsule()
            .fill(Color(braveSystemName: .containerBackground))
            .padding(4)
            .frame(width: geometryProxy.size.width / 2)
            .position(location)
        }
        .overlay {
          HStack {
            Spacer()
            Text(SelectedContent.assets.displayText)
              .font(.subheadline.weight(.semibold))
              .foregroundColor(Color(braveSystemName: selected == .assets ? .textPrimary : .textSecondary))
              .allowsHitTesting(false)
            Spacer()
            Spacer()
            Text(SelectedContent.nfts.displayText)
              .font(.subheadline.weight(.semibold))
              .foregroundColor(Color(braveSystemName: selected == .nfts ? .textPrimary : .textSecondary))
              .allowsHitTesting(false)
            Spacer()
          }
        }
        .readSize { size in
          if location == .zero {
            let newX = selected == .assets ? geometryProxy.size.width / 4 : geometryProxy.size.width / 4 * 3
            location = CGPoint(
              x: newX,
              y: geometryProxy.size.height / 2
            )
          }
          viewSize = size
        }
    }
    .frame(height: 40)
    .gesture(dragGesture)
    .onChange(of: isDragGestureActive) { isDragGestureActive in
      if !isDragGestureActive { // cancellation of gesture, ex while scrolling
        var newX = location.x
        if newX < viewSize.width / 2 {
          select(.assets)
        } else {
          select(.nfts)
        }
      }
    }
    .osAvailabilityModifiers {
      if #available(iOS 16, *) {
        $0.simultaneousGesture(tapGesture)
      } else {
        $0
      }
    }
    .accessibilityRepresentation {
      Picker(selection: $selected) {
        ForEach(SelectedContent.allCases) { content in
          Text(content.displayText).tag(content)
        }
      } label: {
        EmptyView()
      }
      .pickerStyle(.segmented)
    }
  }
  
  private var dragGesture: some Gesture {
    DragGesture()
      .updating($isDragGestureActive) { value, state, transaction in
        state = true
      }
      .onChanged { value in
        var newX = value.location.x
        if newX < viewSize.width / 4 {
          newX = viewSize.width / 4
        } else if newX > (viewSize.width / 4 * 3) {
          newX = (viewSize.width / 4 * 3)
        }
        location = CGPoint(
          x: newX,
          y: location.y
        )
      }
      .onEnded { value in
        if value.predictedEndLocation.x <= viewSize.width / 2 {
          select(.assets)
        } else {
          select(.nfts)
        }
      }
  }
  
  @available(iOS 16, *)
  private var tapGesture: some Gesture {
    SpatialTapGesture()
      .onEnded { value in
        if value.location.x < viewSize.width / 2 {
          select(.assets)
        } else {
          select(.nfts)
        }
      }
  }
  
  private func select(_ selectedContent: SelectedContent) {
    selected = selectedContent
    withAnimation(.spring()) {
      var newX = viewSize.width / 4
      if selectedContent == .nfts {
        newX *= 3
      }
      location = CGPoint(
        x: newX,
        y: viewSize.height / 2
      )
    }
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
