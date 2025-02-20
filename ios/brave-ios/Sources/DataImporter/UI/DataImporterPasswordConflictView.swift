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

  var model: DataImportModel

  var body: some View {
    VStack {
      Button {
        dismiss()
      } label: {
        Label {
          Text(Strings.close)
        } icon: {
          Image(braveSystemName: "leo.close")
            .foregroundColor(Color(braveSystemName: .iconDefault))
            .padding(8)
        }
        .labelStyle(.iconOnly)
      }
      .background(Color(braveSystemName: .materialSeparator), in: Circle())
      .frame(maxWidth: .infinity, alignment: .trailing)

      VStack {
        Image("failure_import_logo", bundle: .module)
          .padding(.bottom, 32.0)

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
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)

      Spacer()

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

      Button(
        action: {
          dismiss()
        },
        label: {
          Text(Strings.CancelString)
            .font(.headline)
            .padding(.horizontal)
            .padding(.vertical, 12.0)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
            .frame(maxWidth: .infinity)
        }
      )
      .buttonStyle(.plain)
    }
  }
}
