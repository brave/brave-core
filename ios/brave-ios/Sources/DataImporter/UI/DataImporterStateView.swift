// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import DesignSystem
import SwiftUI

struct DataImporterStateView: View {
  typealias Action = () -> Void

  @Environment(\.dismiss)
  private var dismiss

  var kind: Kind
  var model: DataImportModel

  var primaryAction: Action
  var secondaryAction: Action?

  var body: some View {
    VStack {
      if kind == .passwordConflict {
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
      }

      VStack {
        Image(kind.iconName, bundle: .module)
          .padding(.bottom, 32.0)

        VStack {
          Text(kind.title)
            .font(.headline)
            .foregroundColor(Color(braveSystemName: .textPrimary))

          if case .failedToImportPasswordsDueToConflict(let results) = model.importError {
            Text(String(format: kind.subtitle, results.displayedEntries.count))
              .font(.footnote)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
          } else {
            Text(String(format: kind.subtitle, 0))
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
          primaryAction()
        },
        label: {
          Text(kind.primaryButtonTitle)
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

      if kind == .passwordConflict {
        Button(
          action: {
            secondaryAction?()
          },
          label: {
            Text(kind.secondaryButtonTitle)
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

  // MARK: - Kind Type

  struct Kind: Equatable {
    var iconName: String
    var title: String
    var subtitle: String
    var primaryButtonTitle: String
    var secondaryButtonTitle: String

    static let failure = Kind(
      iconName: "failure_import_logo",
      title: Strings.DataImporter.importStateFailureTitle,
      subtitle: Strings.DataImporter.importStateFailureMessage,
      primaryButtonTitle: Strings.DataImporter.importStateTryAgainTitle,
      secondaryButtonTitle: ""
    )

    static let success = Kind(
      iconName: "success_import_logo",
      title: Strings.DataImporter.importStateSuccessTitle,
      subtitle: Strings.DataImporter.importStateSuccessMessage,
      primaryButtonTitle: Strings.DataImporter.importStateSuccessContinueTitle,
      secondaryButtonTitle: ""
    )

    static let passwordConflict = Kind(
      iconName: "failure_import_logo",
      title: Strings.DataImporter.importStatePasswordConflictTitle,
      subtitle: Strings.DataImporter.importStatePasswordConflictMessage,
      primaryButtonTitle: Strings.DataImporter
        .importStatePasswordConflictKeepExistingPasswordsTitle,
      secondaryButtonTitle: Strings.DataImporter
        .importStatePasswordConflictKeepImportedPasswordsTitle
    )
  }
}

#if DEBUG
#Preview {
  DataImporterStateView(
    kind: .passwordConflict,
    model: .init(),
    primaryAction: {},
    secondaryAction: {}
  )
}
#endif
