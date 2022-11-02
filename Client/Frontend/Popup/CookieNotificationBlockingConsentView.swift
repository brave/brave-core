// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Strings
import DesignSystem
import BraveShared

struct CookieNotificationBlockingConsentView: View {
  public static let contentHeight = 480.0
  public static let contentWidth = 344.0
  private static let gifHeight = 328.0
  private static let bottomSectionHeight = contentHeight - gifHeight
  private static let textPadding = 16.0
  
  private let animation = Animation.easeOut(duration: 0.5).delay(0)
  private let transition = AnyTransition.scale(scale: 1.1).combined(with: .opacity)
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  @State private var showAnimation = false
  var onDismiss: (() -> Void)?
  
  private var yesButton: some View {
    Button(Strings.yesBlockCookieConsentNotices) {
      withAnimation(animation) {
        self.showAnimation = true
      }

      if !FilterListResourceDownloader.shared.enableFilterList(for: FilterList.cookieConsentNoticesComponentID, isEnabled: true) {
        assertionFailure("This filter list should exist or this UI is completely useless")
      }
      
      Task { @MainActor in
        try await Task.sleep(seconds: 3.5)
        self.dismiss()
      }
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
    .multilineTextAlignment(.center)
    .transition(transition)
  }
  
  private var noButton: some View {
    Button(Strings.noThanks) {
      self.dismiss()
    }
    .font(Font.body.weight(.semibold))
    .foregroundColor(.accentColor)
    .multilineTextAlignment(.center)
    .transition(transition)
  }
  
  var body: some View {
    ScrollView {
      VStack {
        VStack {
          if !showAnimation {
            VStack(spacing: Self.textPadding) {
              Text(Strings.blockCookieConsentNoticesPopupTitle).font(.title)
              Text(Strings.blockCookieConsentNoticesPopupDescription).font(.body)
            }
            .transition(transition)
            .padding(Self.textPadding)
            .padding(.top, 80)
            .foregroundColor(Color(UIColor.braveLabel))
            .multilineTextAlignment(.center)
            .fixedSize(horizontal: false, vertical: true)
          }
        }
        .frame(width: Self.contentWidth)
        .frame(minHeight: Self.gifHeight)
        .background(
          GIFImage(asset: "cookie-consent-animation", animate: showAnimation)
            .frame(width: Self.contentWidth, height: Self.gifHeight, alignment: .top),
          alignment: .top
        )
        
        VStack(spacing: Self.textPadding) {
          if !showAnimation {
            yesButton
            noButton
          }
        }
        .padding(Self.textPadding)
      }
    }
    .frame(width: Self.contentWidth, height: Self.contentHeight)
    .background(
      Image("cookie-consent-background", bundle: .module),
      alignment: .bottomLeading
    )
    .background(Color(UIColor.braveBackground))
  }
  
  private func dismiss() {
    // Dismiss on presentation mode does not work on iOS 14
    // when using the UIHostingController is parent view.
    // As a workaround a completion handler is used instead.
    if #available(iOS 15, *) {
      presentationMode.dismiss()
    } else {
      onDismiss?()
    }
  }
}

#if DEBUG
struct CookieNotificationBlockingConsentView_Previews: PreviewProvider {
  static var previews: some View {
    CookieNotificationBlockingConsentView()
  }
}
#endif
