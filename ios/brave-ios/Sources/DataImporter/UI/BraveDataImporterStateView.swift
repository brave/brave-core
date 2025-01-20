//
//  BraveDataImporterStateView.swift
//  Brave
//
//  Created by Brandon T on 2025-01-30.
//

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
    case .passwordConflict: return "password_conflict_logo"
    }
  }

  private var title: String {
    switch kind {
    case .failure: return "Import Failed"
    case .success: return "Successful!"
    case .passwordConflict: return "Password conflicts"
    }
  }

  private var subtitle: String {
    switch kind {
    case .failure:
      return
        "Something went wrong while importing your data.\nPlease try again.\nIf the problem persists, contact support."
    case .success:
      return
        "Your data has been successfully imported.\nYou're all set to start browsing with Brave."
    case .passwordConflict: return "X Password conflicts were found.\nChoose what to import."
    }
  }

  private var primaryButtonTitle: String {
    switch kind {
    case .failure: return "Try Again"
    case .success: return "Sync with Brave"
    case .passwordConflict: return "Keep passwords from Brave"
    }
  }

  private var secondaryButtonTitle: String {
    switch kind {
    case .failure: return ""
    case .success: return "Continue"
    case .passwordConflict: return "Use passwords from Safari"
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
            Text("Cancel")
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
