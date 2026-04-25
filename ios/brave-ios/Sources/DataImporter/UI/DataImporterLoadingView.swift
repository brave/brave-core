// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import SwiftUI

struct DataImporterLoadingView: View {
  var body: some View {
    VStack(spacing: 32.0) {
      ProgressView()
        .progressViewStyle(BraveProgressStyleCircular(thickness: 5, speed: 3.0))
        .backgroundStyle(Color(braveSystemName: .blurple20))
        .foregroundStyle(Color(braveSystemName: .iconInteractive))
        .frame(width: 40, height: 40)

      VStack {
        Text(Strings.DataImporter.loadingTitle)
          .font(.title.weight(.semibold))
          .foregroundColor(Color(braveSystemName: .textPrimary))

        Text(Strings.DataImporter.loadingMessage)
          .font(.footnote)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
      }
      .multilineTextAlignment(.center)
      .frame(maxWidth: .infinity)
      .fixedSize(horizontal: false, vertical: true)
    }
    .padding(.horizontal, 16.0)
  }
}

#if DEBUG
#Preview {
  DataImporterLoadingView()
}
#endif
