/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore

@available(iOS 14.0, *)
struct DateRangeView: View {
  @Binding var selectedRange: BraveWallet.AssetPriceTimeframe
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  
  private var accessibilityRows: [[BraveWallet.AssetPriceTimeframe]] {
    [[.live, .oneDay, .oneWeek],
     [.oneMonth, .threeMonths, .oneYear, .all]]
  }
  
  @ViewBuilder private func button(for range: BraveWallet.AssetPriceTimeframe) -> some View {
    Button(action: { selectedRange = range }) {
      range.label
        .accessibility(label: Text(verbatim: range.accessibilityLabel))
    }
    .buttonStyle(OptionButtonStyle(isSelected: range == selectedRange))
  }
  
  var body: some View {
    Group {
      if sizeCategory.isAccessibilityCategory && horizontalSizeClass == .compact {
        // Split into 2 rows
        VStack {
          ForEach(accessibilityRows, id: \.self) { row in
            HStack(spacing: 0) {
              ForEach(row, id: \.rawValue) { range in
                button(for: range)
                if range != row.last {
                  Spacer()
                }
              }
            }
          }
        }
      } else {
        HStack(spacing: horizontalSizeClass == .regular ? 6 : 0) {
          ForEach(BraveWallet.AssetPriceTimeframe.allCases, id: \.rawValue) { range in
            button(for: range)
            if range != BraveWallet.AssetPriceTimeframe.allCases.last && horizontalSizeClass == .compact {
              Spacer()
            }
          }
        }
      }
    }
    .font(Font.caption.bold())
//    .frame(maxWidth: .infinity)
  }
}

extension BraveWallet.AssetPriceTimeframe: CaseIterable {
  public static var allCases: [BraveWallet.AssetPriceTimeframe] {
    [.live, .oneDay, .oneWeek, .oneMonth, .threeMonths, .oneYear, .all]
  }
  
  var accessibilityLabel: String {
    // NSLocalizedString
    switch self {
    case .live: return "Live"
    case .oneDay: return "1 Day"
    case .oneWeek: return "1 Week"
    case .oneMonth: return "1 Month"
    case .threeMonths: return "3 Months"
    case .oneYear: return "1 Year"
    case .all: return "All"
    @unknown default: return ""
    }
  }
  
  private var displayString: String {
    // NSLocalizedString
    switch self {
    case .live: return "LIVE"
    case .oneDay: return "1D"
    case .oneWeek: return "1W"
    case .oneMonth: return "1M"
    case .threeMonths: return "3M"
    case .oneYear: return "1Y"
    case .all: return "ALL"
    @unknown default: return ""
    }
  }
  
  @ViewBuilder fileprivate var label: some View {
    switch self {
    case .live:
      HStack(spacing: 4) {
        Circle().frame(width: 6, height: 6)
        Text(verbatim: displayString)
      }
    default:
      Text(verbatim: displayString)
    }
  }
}

private struct OptionButtonStyle: ButtonStyle {
  var isSelected: Bool
  
  @Environment(\.colorScheme) private var colorScheme
  
  private var backgroundShape: some View {
    RoundedRectangle(cornerRadius: 6, style: .continuous)
      .fill(Color(.secondaryBraveLabel))
  }
  
  func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .foregroundColor(.white)
      .colorMultiply(isSelected ? Color(.braveBackground) : Color(.secondaryBraveLabel)) // To animate text color
      .padding(.horizontal, 6)
      .padding(.vertical, 4)
      .background(
        backgroundShape
          .opacity(configuration.isPressed ? 0.1 : 0.0)
      )
      .background(
        backgroundShape
          .opacity(isSelected ? 1.0 : 0.0)
      )
      .animation(.linear(duration: 0.1), value: configuration.isPressed || isSelected)
      .accessibility(addTraits: isSelected ? .isSelected : [])
  }
}
