// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Data
import Preferences
import Shared
import SwiftUI
import WebKit

private struct ZoomView: View {
  @ScaledMetric private var buttonWidth = 44.0

  var isPrivateBrowsing: Bool
  var minValue: Double
  var maxValue: Double
  @Binding var value: Double

  var onDecrement: () -> Void
  var onReset: () -> Void
  var onIncrement: () -> Void

  var body: some View {
    HStack(spacing: 0.0) {
      Button(action: onDecrement) {
        Image(systemName: "minus")
          .font(.system(.footnote).weight(.medium))
          .foregroundColor(value == minValue ? .accentColor : Color(UIColor.braveLabel))
          .frame(width: buttonWidth)
      }
      .disabled(value == minValue)
      .hoverEffect()

      Divider()

      // 999 is the largest width of number as a percentage that a 3 digit number can be.
      // IE: Zoom level of 300% would be smaller physically than 999% in character widths
      Text(NSNumber(value: 0.999), formatter: PageZoomView.percentFormatter)
        .font(.system(.footnote).weight(.medium))
        .fixedSize(horizontal: true, vertical: false)
        .hidden()
        .overlay(resetZoomButton)
        .padding(.horizontal)

      Divider()

      Button(action: onIncrement) {
        Image(systemName: "plus")
          .font(.system(.footnote).weight(.medium))
          .foregroundColor(value == maxValue ? .accentColor : Color(UIColor.braveLabel))
          .frame(width: buttonWidth)
      }
      .disabled(value == maxValue)
      .hoverEffect()
    }
    .padding(.vertical, 8.0)
    .fixedSize(horizontal: false, vertical: true)
    .background(Color(UIColor.secondaryBraveBackground))
    .clipShape(RoundedRectangle(cornerRadius: 10.0, style: .continuous))
  }

  private var resetZoomButton: some View {
    Button(action: onReset) {
      Text(NSNumber(value: value), formatter: PageZoomView.percentFormatter)
        .font(.system(.footnote).weight(.medium))
        .foregroundColor(
          (value == (isPrivateBrowsing ? 1.0 : Preferences.General.defaultPageZoomLevel.value))
            ? .accentColor : Color(UIColor.braveLabel)
        )
        .padding()
        .contentShape(Rectangle())
    }
    .fixedSize(horizontal: true, vertical: false)
    .disabled(value == (isPrivateBrowsing ? 1.0 : Preferences.General.defaultPageZoomLevel.value))
  }
}

struct PageZoomView: View {
  @Environment(\.managedObjectContext) private var context

  @ObservedObject private var zoomHandler: PageZoomHandler
  private let isPrivateBrowsing: Bool
  @State private var minValue = 0.5
  @State private var maxValue = 3.0

  public static let percentFormatter = NumberFormatter().then {
    $0.numberStyle = .percent
    $0.minimumIntegerDigits = 1
    $0.maximumIntegerDigits = 3
    $0.maximumFractionDigits = 0
    $0.minimumFractionDigits = 0
  }

  public static let notificationName = Notification.Name(rawValue: "com.brave.pagezoom-change")

  var dismiss: (() -> Void)?

  init(zoomHandler: PageZoomHandler) {
    self.zoomHandler = zoomHandler
    self.isPrivateBrowsing = zoomHandler.isPrivateBrowsing

  }

  var body: some View {
    VStack(spacing: 0.0) {
      Divider()
      HStack(spacing: 0.0) {
        Text(Strings.PageZoom.zoomViewText)
          .font(.system(.subheadline))
          .fixedSize(horizontal: false, vertical: true)
          .frame(maxWidth: .infinity, alignment: .leading)
        ZoomView(
          isPrivateBrowsing: isPrivateBrowsing,
          minValue: minValue,
          maxValue: maxValue,
          value: $zoomHandler.currentValue,
          onDecrement: decrement,
          onReset: reset,
          onIncrement: increment
        )
        .frame(maxWidth: .infinity)
        Button {
          dismiss?()
        } label: {
          Image(systemName: "xmark")
            .foregroundColor(Color(UIColor.braveBlurpleTint))
            .font(.system(.footnote).weight(.medium))
        }
        .frame(maxWidth: .infinity, alignment: .trailing)
      }
      .padding(.horizontal)
      .padding(.vertical, 6.0)
      .hoverEffect()
    }
    .background(Color(UIColor.braveBackground))
    .ignoresSafeArea()
  }

  private func increment() {
    zoomHandler.changeZoomLevel(.increment)
  }

  private func reset() {
    zoomHandler.reset()
  }

  private func decrement() {
    zoomHandler.changeZoomLevel(.decrement)
  }
}

#if DEBUG
struct PageZoomView_Previews: PreviewProvider {
  static var previews: some View {
    PageZoomView(zoomHandler: PageZoomHandler(tab: nil, isPrivateBrowsing: false))
      .previewLayout(PreviewLayout.sizeThatFits)
  }
}
#endif
