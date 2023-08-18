// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import Strings
import DesignSystem
import BraveShared
import Growth
import BraveUI

public struct CookieNotificationBlockingConsentView: View {
  public static let contentHeight = 400.0
  public static let contentWidth = 344.0
  private static let textPadding = 24.0
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  public var body: some View {
    ScrollView {
      VStack(alignment: .center, spacing: 48) {
        VStack(alignment: .center, spacing: Self.textPadding) {
          Image("cookie-consent-icon", bundle: .module)
            .resizable()
            .frame(width: 48, height: 48)
          
          Text(Strings.blockCookieConsentNoticesPopupTitle).font(.title)
          Text(Strings.blockCookieConsentNoticesPopupDescription).font(.body)
        }
        .padding(Self.textPadding)
        .foregroundColor(Color(UIColor.braveLabel))
        .multilineTextAlignment(.center)
        .fixedSize(horizontal: false, vertical: true)
        
        Button(Strings.blockCookieConsentNoticesDismissButtonTitle,
          action: {
            presentationMode.dismiss()
          }
        )
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .multilineTextAlignment(.center)
      }
    }
    .frame(width: Self.contentWidth, height: Self.contentHeight)
    .background(
      Image("cookie-consent-background", bundle: .module),
      alignment: .bottomLeading
    )
    .background(Color(UIColor.braveBackground))
    .onAppear {
      recordCookieListPromptP3A(answer: .seen)
    }
  }
  
  private enum P3AAnswer: Int, CaseIterable {
    case seen = 1
  }
  
  private func recordCookieListPromptP3A(answer: P3AAnswer) {
    // Q68 If you have viewed the cookie consent block prompt, how did you react?
    UmaHistogramEnumeration("Brave.Shields.CookieListPrompt", sample: answer)
  }
}

#if DEBUG
struct CookieNotificationBlockingConsentView_Previews: PreviewProvider {
  static var previews: some View {
    CookieNotificationBlockingConsentView()
  }
}
#endif

public class CookieNotificationBlockingConsentViewController: UIHostingController<CookieNotificationBlockingConsentView>, PopoverContentComponent {
  public init() {
    super.init(rootView: CookieNotificationBlockingConsentView())
    
    self.preferredContentSize = CGSize(
      width: CookieNotificationBlockingConsentView.contentWidth,
      height: CookieNotificationBlockingConsentView.contentHeight
    )
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  public override func viewDidLoad() {
    super.viewDidLoad()
    view.backgroundColor = UIColor.braveBackground
  }
}
