// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Shared
import BraveShared

extension PrivacyReportsView {
  struct BlockedLabel: View {
    var body: some View {
      Text(Strings.PrivacyHub.blockedLabel.uppercased())
        .foregroundColor(Color("label_red_foreground", bundle: .module))
        .padding(.horizontal, 8)
        .padding(.vertical, 2)
        .background(Color("label_red_background", bundle: .module))
        .clipShape(RoundedRectangle(cornerRadius: 12, style: .continuous))
        .fixedSize(horizontal: false, vertical: true)
    }
  }

  struct BlockedByVPNLabel: View {
    @Environment(\.sizeCategory) private var sizeCategory

    var body: some View {
      Text(Strings.PrivacyHub.vpnLabel.uppercased())
        .foregroundColor(Color("label_violet_foreground", bundle: .module))
        .padding(.horizontal, 4)
        .padding(.vertical, 2)
        .background(Color("label_violet_background", bundle: .module))
        .clipShape(
          RoundedRectangle(
            cornerRadius: sizeCategory.isAccessibilityCategory ? 8.0 : 24.0,
            style: .continuous)
        )
        .fixedSize(horizontal: false, vertical: true)
    }
  }

  struct BlockedByShieldsLabel: View {
    @Environment(\.sizeCategory) private var sizeCategory

    var body: some View {
      Text(Strings.PrivacyHub.shieldsLabel.uppercased())
        .foregroundColor(Color("label_orange_foreground", bundle: .module))
        .padding(.horizontal, 4)
        .padding(.vertical, 2)
        .background(Color("label_orange_background", bundle: .module))
        .clipShape(
          RoundedRectangle(
            cornerRadius: sizeCategory.isAccessibilityCategory ? 8.0 : 24.0,
            style: .continuous)
        )
        .fixedSize(horizontal: false, vertical: true)
    }
  }
}

#if DEBUG
struct PrivacyReportLabels_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      PrivacyReportsView.BlockedLabel()
      PrivacyReportsView.BlockedByVPNLabel()
      PrivacyReportsView.BlockedByShieldsLabel()

      Group {
        PrivacyReportsView.BlockedLabel()
        PrivacyReportsView.BlockedByVPNLabel()
        PrivacyReportsView.BlockedByShieldsLabel()
      }
      .preferredColorScheme(.dark)
    }
    .previewLayout(.sizeThatFits)
    .padding()

  }
}
#endif
