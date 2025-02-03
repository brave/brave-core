// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import SwiftUI

struct BraveDataImporterStateView: View {
  typealias Action = () -> Void

  enum Kind {
    case failure
    case success
    case passwordConflict
  }

  @Environment(\.dismissHandler)
  private var dismissHandler

  @State
  var kind: Kind

  var primaryAction: Action
  var secondaryAction: Action

  private var iconName: String {
    switch kind {
    case .failure: return "failure_import_logo"
    case .success: return "success_import_logo"
    case .passwordConflict: return "failure_import_logo"
    }
  }

  private var title: String {
    switch kind {
    case .failure: return Strings.DataImporter.importStateFailureTitle
    case .success: return Strings.DataImporter.importStateSuccessTitle
    case .passwordConflict: return Strings.DataImporter.importStatePasswordConflictTitle
    }
  }

  private var subtitle: String {
    switch kind {
    case .failure:
      return Strings.DataImporter.importStateFailureMessage
    case .success:
      return Strings.DataImporter.importStateSuccessMessage
    case .passwordConflict: return Strings.DataImporter.importStatePasswordConflictMessage
    }
  }

  private var primaryButtonTitle: String {
    switch kind {
    case .failure: return Strings.DataImporter.importStateTryAgainTitle
    case .success: return Strings.DataImporter.importStateSyncTitle
    case .passwordConflict:
      return Strings.DataImporter.importStatePasswordConflictKeepExistingPasswordsTitle
    }
  }

  private var secondaryButtonTitle: String {
    switch kind {
    case .failure: return ""
    case .success: return Strings.DataImporter.importStateSuccessContinueTitle
    case .passwordConflict:
      return Strings.DataImporter.importStatePasswordConflictKeepImportedPasswordsTitle
    }
  }

  var body: some View {
    VStack(alignment: .center) {
      if kind == .passwordConflict {
        HStack {
          Spacer()

          Button {
            dismissHandler?()
          } label: {
            Image(braveSystemName: "leo.close")
              .foregroundColor(Color(braveSystemName: .iconDefault))
              .padding(8)
          }
          .frame(alignment: .trailing)
          .background(Color(braveSystemName: .materialSeparator))
          .clipShape(Circle())
        }
      }

      VStack {
        Image(
          iconName,
          bundle: .module
        )
        .padding(.bottom, 32.0)

        Text(title)
          .font(.body.weight(.semibold))
          .multilineTextAlignment(.center)
          .foregroundColor(Color(braveSystemName: .textPrimary))
          .frame(maxWidth: .infinity, alignment: .center)
          .fixedSize(horizontal: false, vertical: true)
          .padding(.horizontal, 24.0)

        Text(subtitle)
          .font(.footnote)
          .multilineTextAlignment(.center)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
          .frame(maxWidth: .infinity, alignment: .center)
          .fixedSize(horizontal: false, vertical: true)
          .padding(.horizontal, 24.0)
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)

      Spacer()

      Button(
        action: {
          primaryAction()
        },
        label: {
          Text(primaryButtonTitle)
            .font(.body.weight(.semibold))
            .padding()
            .foregroundStyle(Color(braveSystemName: .schemesOnPrimary))
            .frame(maxWidth: .infinity, alignment: .center)
            .background(
              Color(braveSystemName: .buttonBackground),
              in: RoundedRectangle(cornerRadius: 12.0, style: .continuous)
            )
        }
      )
      .buttonStyle(.plain)

      if kind != .failure {
        Button(
          action: {
            secondaryAction()
          },
          label: {
            Text(secondaryButtonTitle)
              .font(.body.weight(.semibold))
              .padding()
              .foregroundStyle(Color(braveSystemName: .textInteractive))
              .frame(maxWidth: .infinity, alignment: .center)
          }
        )
        .buttonStyle(.plain)
      }

      if kind == .passwordConflict {
        Button(
          action: {
            dismissHandler?()
          },
          label: {
            Text(Strings.CancelString)
              .font(.body.weight(.semibold))
              .padding()
              .foregroundStyle(Color(braveSystemName: .textSecondary))
              .frame(maxWidth: .infinity, alignment: .center)
          }
        )
        .buttonStyle(.plain)
      }
    }
    .padding(16.0)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .background(
      .thickMaterial
    )
  }
}

#Preview {
  BraveDataImporterStateView(kind: .passwordConflict, primaryAction: {}, secondaryAction: {})
}
