// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Foundation
import Shared
import SwiftUI

/// Displays warnings about the pages security
///
/// Currently this is only shown when the page security requires a visible warning on the URL bar
struct PageSecurityView: View {
  var displayURL: String
  var secureState: TabSecureContentState
  var hasCertificate: Bool
  var presentCertificateViewer: () -> Void

  @Environment(\.pixelLength) private var pixelLength

  @ViewBuilder private var warningIcon: some View {
    switch secureState {
    case .secure, .localhost:
      EmptyView()
    case .unknown:
      Image(braveSystemName: "leo.info.filled")
        .foregroundColor(Color(braveSystemName: .systemfeedbackWarningIcon))
    case .invalidCert, .missingSSL, .mixedContent:
      Image(braveSystemName: "leo.warning.triangle-filled")
        .foregroundColor(Color(braveSystemName: .systemfeedbackErrorIcon))
    }
  }

  @ViewBuilder private var warningTitle: some View {
    switch secureState {
    case .secure, .localhost:
      EmptyView()
    case .unknown:
      Text(Strings.PageSecurityView.pageUnknownStatusTitle)
        .foregroundColor(Color(braveSystemName: .systemfeedbackWarningText))
    case .invalidCert, .missingSSL:
      Text(Strings.PageSecurityView.pageNotSecureTitle)
        .foregroundColor(Color(braveSystemName: .systemfeedbackErrorText))
    case .mixedContent:
      Text(Strings.PageSecurityView.pageNotFullySecureTitle)
        .foregroundColor(Color(braveSystemName: .systemfeedbackErrorText))
    }
  }

  private var warningBody: String {
    switch secureState {
    case .secure, .localhost:
      return ""
    case .unknown:
      return Strings.PageSecurityView.pageUnknownStatusWarning
    case .invalidCert, .missingSSL, .mixedContent:
      return Strings.PageSecurityView.pageNotSecureDetailedWarning
    }
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      VStack(alignment: .leading, spacing: 16) {
        URLElidedText(text: displayURL)
          .font(.headline)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .frame(maxWidth: .infinity, alignment: .leading)
          .fixedSize(horizontal: false, vertical: true)

        HStack(alignment: .firstTextBaseline) {
          warningIcon
          VStack(alignment: .leading, spacing: 4) {
            warningTitle
            Text(warningBody)
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
            Label(
              Strings.PageSecurityView.viewCertificateButtonTitle,
              braveSystemImage: "leo.lock.plain"
            )
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
      assert(
        secureState.shouldDisplayWarning,
        "Currently only supports displaying insecure warnings"
      )
    }
    #endif
  }
}

extension PageSecurityView: PopoverContentComponent {
  var popoverBackgroundColor: UIColor {
    UIColor.braveBackground
  }
}

#if DEBUG
#Preview {
  PageSecurityView(
    displayURL: "http.badssl.com",
    secureState: .missingSSL,
    hasCertificate: false,
    presentCertificateViewer: {}
  )
  .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
  .shadow(radius: 10, x: 0, y: 1)
  .padding()
}
#endif
