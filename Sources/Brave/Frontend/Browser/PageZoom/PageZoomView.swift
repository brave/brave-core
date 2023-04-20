// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import WebKit
import Data
import Shared
import Preferences

private struct ZoomView: View {
  @ScaledMetric private var buttonWidth = 44.0
  
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
        .foregroundColor((value == (PrivateBrowsingManager.shared.isPrivateBrowsing ? 1.0 : Preferences.General.defaultPageZoomLevel.value)) ? .accentColor : Color(UIColor.braveLabel))
        .padding()
        .contentShape(Rectangle())
    }
    .fixedSize(horizontal: true, vertical: false)
    .disabled(value == (PrivateBrowsingManager.shared.isPrivateBrowsing ? 1.0 : Preferences.General.defaultPageZoomLevel.value))
  }
}

struct PageZoomView: View {
  @Environment(\.managedObjectContext) private var context
  
  private var webView: WKWebView?
  @State private var minValue = 0.5
  @State private var maxValue = 3.0
  @State private var currentValue: Double
  
  public static let percentFormatter = NumberFormatter().then {
    $0.numberStyle = .percent
    $0.minimumIntegerDigits = 1
    $0.maximumIntegerDigits = 3
    $0.maximumFractionDigits = 0
    $0.minimumFractionDigits = 0
  }
  
  public static let propertyName = "viewScale"
  public static let notificationName = Notification.Name(rawValue: "com.brave.pagezoom-change")
  
  public static let steps = [0.5, 0.75, 0.85,
                             1.0, 1.15, 1.25,
                             1.50, 1.75, 2.00,
                             2.50, 3.0]
  
  var dismiss: (() -> Void)?
  
  init(webView: WKWebView?) {
    self.webView = webView
    
    // Private Browsing on Safari iOS always defaults to 100%, and isn't persistently saved.
    if PrivateBrowsingManager.shared.isPrivateBrowsing {
      _currentValue = State(initialValue: 1.0)
      return
    }
    
    // We never re-init, so
    // it is okay to initialize state here.
    if let url = webView?.url,
       let domain = Domain.getPersistedDomain(for: url) {
      
      _currentValue = State(initialValue: domain.zoom_level?.doubleValue ?? Preferences.General.defaultPageZoomLevel.value)
    } else {
      _currentValue = State(initialValue: webView?.value(forKey: PageZoomView.propertyName) as? Double ?? Preferences.General.defaultPageZoomLevel.value)
    }
  }
  
  var body: some View {
    VStack(spacing: 0.0) {
      Divider()
      HStack(spacing: 0.0) {
        Text(Strings.PageZoom.zoomViewText)
          .font(.system(.subheadline))
          .frame(maxWidth: .infinity, alignment: .leading)
        
        ZoomView(
          minValue: minValue,
          maxValue: maxValue,
          value: $currentValue,
          onDecrement: decrement,
          onReset: reset,
          onIncrement: increment)
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
    }
    .background(Color(UIColor.braveBackground))
  }
  
  private func storeChanges() {
    guard let webView = webView,
          let url = webView.url else { return }
    
    webView.setValue(currentValue, forKey: PageZoomView.propertyName)
    
    // Do NOT store the changes in the Domain
    if !PrivateBrowsingManager.shared.isPrivateBrowsing {
      let domain = Domain.getPersistedDomain(for: url)?.then {
        $0.zoom_level = currentValue == $0.zoom_level?.doubleValue ? nil : NSNumber(value: currentValue)
      }
      
      try? domain?.managedObjectContext?.save()
    }
  }
  
  private func increment() {
    guard let index = PageZoomView.steps.firstIndex(of: currentValue),
          index + 1 < PageZoomView.steps.count else { return }
    currentValue = PageZoomView.steps[index + 1]
    storeChanges()
  }
  
  private func reset() {
    currentValue = Preferences.General.defaultPageZoomLevel.value
    storeChanges()
  }
  
  private func decrement() {
    guard let index = PageZoomView.steps.firstIndex(of: currentValue),
          index - 1 >= 0 else { return }
    currentValue = PageZoomView.steps[index - 1]
    storeChanges()
  }
}

#if DEBUG
struct PageZoomView_Previews: PreviewProvider {
  static var previews: some View {
    PageZoomView(webView: nil)
      .previewLayout(PreviewLayout.sizeThatFits)
  }
}
#endif
