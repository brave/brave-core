// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import Strings
import BraveUI
import DesignSystem
import SnapKit

struct SubmitReportSuccessView: View {
  var body: some View {
    VStack(alignment: .center, spacing: 16) {
      Image(braveSystemName: "leo.check.circle-outline")
        .font(.system(size: 48))
        .foregroundStyle(Color(braveSystemName: .systemfeedbackSuccessIcon))
      Text(Strings.Shields.siteReportedTitle)
        .font(.title)
      Text(Strings.Shields.siteReportedBody)
    }
    .padding(32)
    .multilineTextAlignment(.center)
    .foregroundStyle(Color(braveSystemName: .textTertiary))
  }
}

#if swift(>=5.9)
#Preview {
  SubmitReportSuccessView()
}
#endif
