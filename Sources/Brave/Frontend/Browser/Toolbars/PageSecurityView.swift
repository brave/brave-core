// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveCore
import BraveUI
import Shared

/// Displays warnings about the pages security
///
/// Currently this is only shown when the page security requires a visible warning on the URL bar
struct PageSecurityView: View {
  var displayURL: String
  var secureState: TabSecureContentState
  var hasCertificate: Bool
  var presentCertificateViewer: () -> Void
  
  @Environment(\.pixelLength) private var pixelLength
  
  private var warningTitle: String {
    switch secureState {
    case .secure, .localhost:
      return ""
    case .unknown, .invalidCert, .missingSSL:
      return Strings.PageSecurityView.pageNotSecureTitle
    case .mixedContent:
      return Strings.PageSecurityView.pageNotFullySecureTitle
    }
  }
  
  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      VStack(alignment: .leading, spacing: 16) {
        Text(displayURL)
          .font(.headline)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
        HStack(alignment: .firstTextBaseline) {
          Image(braveSystemName: "leo.warning.triangle-filled")
            .foregroundColor(Color(braveSystemName: .systemfeedbackErrorIcon))
          VStack(alignment: .leading, spacing: 4) {
            Text(warningTitle)
              .foregroundColor(Color(braveSystemName: .systemfeedbackErrorText))
            Text(Strings.PageSecurityView.pageNotSecureDetailedWarning)
              .foregroundColor(Color(braveSystemName: .textTertiary))
              .font(.footnote)
          }
        }
      }
      .multilineTextAlignment(.leading)
      .font(.subheadline)
      .padding()
      if hasCertificate {
        Color(braveSystemName: .dividerSubtle)
          .frame(height: pixelLength)
        Button {
          presentCertificateViewer()
        } label: {
          HStack(alignment: .firstTextBaseline) {
            Label(Strings.PageSecurityView.viewCertificateButtonTitle, braveSystemImage: "leo.lock.plain")
            Spacer()
            Image(braveSystemName: "leo.carat.right")
              .imageScale(.large)
          }
          .font(.subheadline)
          .foregroundColor(Color(braveSystemName: .textInteractive))
          .padding()
        }
      }
    }
    .background(Color(.braveBackground))
    .frame(maxWidth: BraveUX.baseDimensionValue, alignment: .leading)
#if DEBUG
    .onAppear {
      assert(secureState.shouldDisplayWarning, 
             "Currently only supports displaying insecure warnings")
    }
#endif
  }
}

extension PageSecurityView: PopoverContentComponent {
  var popoverBackgroundColor: UIColor {
    UIColor.braveBackground
  }
}

#if swift(>=5.9)
#if DEBUG
#Preview {
  PageSecurityView(
    displayURL: "http.badssl.com",
    secureState: .missingSSL,
    hasCertificate: false,
    presentCertificateViewer: { }
  )
  .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
  .shadow(radius: 10, x: 0, y: 1)
  .padding()
}
#endif
#endif
