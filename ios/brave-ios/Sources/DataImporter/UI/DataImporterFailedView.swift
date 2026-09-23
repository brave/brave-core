// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import DesignSystem
import SwiftUI

struct DataImporterFailedView: View {
  var primaryAction: () -> Void

  var body: some View {
    VStack {
      VStack {
        Image("failure_import_logo", bundle: .module)
          .padding(.bottom, 32.0)

        VStack {
          Text(Strings.DataImporter.importStateFailureTitle)
            .font(.title.weight(.semibold))
            .foregroundColor(Color(braveSystemName: .textPrimary))

          Text(Strings.DataImporter.importStateFailureMessage)
            .font(.subheadline)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
        .multilineTextAlignment(.center)
        .frame(maxWidth: .infinity)
        .fixedSize(horizontal: false, vertical: true)
        .padding(.horizontal, 24.0)
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)

      Spacer()

      Button(
        action: {
          primaryAction()
        },
        label: {
          Text(Strings.DataImporter.importStateTryAgainTitle)
            .frame(maxWidth: .infinity)
        }
      )
      .buttonStyle(.filled)
    }
  }
}
