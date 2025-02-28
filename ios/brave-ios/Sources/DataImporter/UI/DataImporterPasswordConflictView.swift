// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import DesignSystem
import SwiftUI

struct DataImporterPasswordConflictView: View {
  @Environment(\.dismiss)
  private var dismiss

  @State
  var sheetHeight: CGFloat = 0.0

  var model: DataImportModel

  var body: some View {
    ScrollView {
      VStack {
        Image("failure_import_logo", bundle: .module)
          .padding(.top, 24.0)
          .padding(.bottom, 16.0)

        VStack {
          Text(Strings.DataImporter.importStatePasswordConflictTitle)
            .font(.headline)
            .foregroundColor(Color(braveSystemName: .textPrimary))

          if case .failedToImportPasswordsDueToConflict(let results) = model.importError {
            Text(
              String(
                format: Strings.DataImporter.importStatePasswordConflictMessage,
                results.displayedEntries.count
              )
            )
            .font(.footnote)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
          }
        }
        .multilineTextAlignment(.center)
        .frame(maxWidth: .infinity)
        .fixedSize(horizontal: false, vertical: true)
        .padding(.horizontal, 24.0)
        .padding(.bottom, 16.0)

        Button(
          action: {
            Task {
              await model.keepPasswords(option: .keepBravePasswords)
            }
          },
          label: {
            Text(
              Strings.DataImporter
                .importStatePasswordConflictKeepExistingPasswordsTitle
            )
            .font(.headline)
            .padding(.horizontal)
            .padding(.vertical, 12.0)
            .foregroundStyle(Color(braveSystemName: .schemesOnPrimary))
            .frame(maxWidth: .infinity)
            .background(
              Color(braveSystemName: .buttonBackground),
              in: RoundedRectangle(cornerRadius: 12.0, style: .continuous)
            )
          }
        )
        .buttonStyle(.plain)

        Button(
          action: {
            Task {
              await model.keepPasswords(option: .keepSafariPasswords)
            }
          },
          label: {
            Text(
              Strings.DataImporter
                .importStatePasswordConflictKeepImportedPasswordsTitle
            )
            .font(.headline)
            .padding(.horizontal)
            .padding(.vertical, 12.0)
            .foregroundStyle(Color(braveSystemName: .textInteractive))
            .frame(maxWidth: .infinity)
          }
        )
        .buttonStyle(.plain)
      }
      .padding(16.0)
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .onGeometryChange(
        for: CGSize.self,
        of: { $0.size },
        action: {
          sheetHeight = $0.height
        }
      )
    }
    .presentationDetents([.height(sheetHeight)])
    .presentationDragIndicator(.visible)
    .osAvailabilityModifiers({
      if #available(iOS 16.4, *) {
        $0.presentationBackground(.thickMaterial)
          .presentationCornerRadius(15.0)
          .presentationCompactAdaptation(.sheet)
      } else {
        $0.background(.thickMaterial)
      }
    })
  }
}
