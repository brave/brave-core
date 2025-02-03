// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import SwiftUI

struct BraveDataImporterLoadingView: View {
  var body: some View {
    VStack(alignment: .center) {
      ProgressView()
        .progressViewStyle(
          BraveProgressStyleCircular(thickness: 5, speed: 3.0)
        )
        .backgroundStyle(Color(braveSystemName: .blurple20))
        .foregroundStyle(Color(braveSystemName: .iconInteractive))
        .frame(width: 40, height: 40)
        .padding(.vertical, 32.0)

      Text(Strings.DataImporter.loadingTitle)
        .font(.body.weight(.semibold))
        .multilineTextAlignment(.center)
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .frame(maxWidth: .infinity, alignment: .center)
        .fixedSize(horizontal: false, vertical: true)
        .padding(.horizontal, 24.0)

      Text(Strings.DataImporter.loadingMessage)
        .font(.footnote)
        .multilineTextAlignment(.center)
        .foregroundStyle(Color(braveSystemName: .textSecondary))
        .frame(maxWidth: .infinity, alignment: .center)
        .fixedSize(horizontal: false, vertical: true)
        .padding(.horizontal, 24.0)
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)
    .padding()
  }
}

#Preview {
  BraveDataImporterLoadingView()
}
