// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import BraveUI
import Data
import Preferences
import Strings
import SwiftUI

struct ShredSettingsView: View {
  @ObservedObject private var settings: DomainSettings
  private let shredSitesDataNow: () -> Void
  @State private var showConfirmation = false

  init(url: URL, isPersistent: Bool, shredSitesDataNow: @escaping () -> Void) {
    self.settings = DomainSettings(url: url, isPersistent: isPersistent)
    self.shredSitesDataNow = shredSitesDataNow
  }

  var body: some View {
    List {
      Section {
        Picker(selection: $settings.shredLevel) {
          ForEach(SiteShredLevel.allCases) { level in
            Text(level.localizedTitle)
              .foregroundColor(Color(.secondaryBraveLabel))
              .tag(level)
          }
        } label: {
          LabelView(
            title: Strings.Shields.automaticallyShred,
            subtitle: nil
          )
        }

        Button(
          action: {
            showConfirmation = true
          },
          label: {
            HStack(alignment: .center) {
              Text(Strings.Shields.shredSitesDataNow)
                .frame(maxWidth: .infinity, alignment: .leading)
              Image(braveSystemName: "leo.shred.data")
            }
          }
        ).alert(
          isPresented: $showConfirmation,
          content: {
            confirmationAlert
          }
        )
      } header: {
        Text(settings.url.displayURL?.baseDomain ?? settings.url.absoluteString)
      }.listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
  }

  private var confirmationAlert: Alert {
    Alert(
      title: Text(Strings.Shields.shredThisSitesDataConfirmationTitle),
      message: Text(Strings.Shields.shredThisSitesDataConfirmationMessage),
      primaryButton: .destructive(
        Text(Strings.Shields.shredDataButtonTitle),
        action: {
          shredSitesDataNow()
        }
      ),
      secondaryButton: .cancel(Text(Strings.cancelButtonTitle))
    )
  }
}

#Preview {
  ShredSettingsView(
    url: URL(string: "https://brave.com")!,
    isPersistent: false
  ) {
    // Do nothing
  }
}

extension SiteShredLevel: Identifiable {
  public var id: String {
    return rawValue
  }

  var localizedTitle: String {
    switch self {
    case .never:
      return Strings.Shields.shredNever
    case .whenSiteClosed:
      return Strings.Shields.shredWhenSiteClosed
    case .appExit:
      return Strings.Shields.shredOnAppExitOnly
    }
  }
}

@MainActor class DomainSettings: ObservableObject {
  let domain: Domain
  let url: URL

  @Published var shredLevel: SiteShredLevel {
    didSet {
      domain.shredLevel = self.shredLevel
    }
  }

  init(url: URL, isPersistent: Bool) {
    let domain = Domain.getOrCreate(forUrl: url, persistent: isPersistent)
    self.url = url
    self.domain = domain
    self.shredLevel = domain.shredLevel
  }
}

/// A wrapper controller for the ShredSettingsView which will set the correct size based on trait collection changes
class ShredSettingsHostingController: UIHostingController<ShredSettingsView> {
  init(
    url: URL,
    isPersistent: Bool,
    presentingView: UIView,
    shredSitesDataNow: @escaping () -> Void
  ) {
    super.init(
      rootView: ShredSettingsView(
        url: url,
        isPersistent: isPersistent,
        shredSitesDataNow: shredSitesDataNow
      )
    )

    modalPresentationStyle = .popover

    if let popover = popoverPresentationController {
      popover.sourceView = presentingView
      popover.sourceRect = presentingView.bounds

      let sheet = popover.adaptiveSheetPresentationController
      sheet.largestUndimmedDetentIdentifier = nil
      sheet.prefersEdgeAttachedInCompactHeight = false
      sheet.widthFollowsPreferredContentSizeWhenEdgeAttached = true
      sheet.detents = [
        .custom(resolver: { context in
          return Self.makePreferredHeight(
            for: context.containerTraitCollection
          )
        }),
        .large(),
      ]
      sheet.prefersGrabberVisible = true
    }
  }

  override func viewIsAppearing(_ animated: Bool) {
    super.viewIsAppearing(animated)
    setPreferredContentChange()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    setPreferredContentChange()
    view.setNeedsLayout()
  }

  /// Will set the preferred size
  @objc private func setPreferredContentChange() {
    self.preferredContentSize = makePreferredSize(for: traitCollection)
  }

  /// Get a preferred width based on the `UITraitCollection`
  private func makePreferredSize(for traitCollection: UITraitCollection) -> CGSize {
    switch traitCollection.verticalSizeClass {
    case .regular:
      switch traitCollection.preferredContentSizeCategory {
      case .small, .extraSmall, .medium, .large:
        return CGSize(width: view.frame.width, height: 150)
      case .extraLarge, .extraExtraLarge, .extraExtraExtraLarge:
        return CGSize(width: view.frame.width, height: 175)
      case .accessibilityMedium:
        return CGSize(width: 400, height: 225)
      case .accessibilityLarge:
        return CGSize(width: 400, height: 275)
      case .accessibilityExtraLarge:
        return CGSize(width: 500, height: 325)
      case .accessibilityExtraExtraLarge:
        return CGSize(width: 600, height: 300)
      case .accessibilityExtraExtraExtraLarge:
        return CGSize(width: 600, height: 400)
      default:
        return view.frame.size
      }
    case .compact, .unspecified:
      return view.frame.size
    @unknown default:
      return view.frame.size
    }
  }

  /// Get a preferred height based on the `UITraitCollection`
  private static func makePreferredHeight(for traitCollection: UITraitCollection) -> CGFloat {
    switch traitCollection.preferredContentSizeCategory {
    case .small, .extraSmall, .medium, .large:
      return 150
    case .extraLarge, .extraExtraLarge, .extraExtraExtraLarge:
      return 175
    case .accessibilityMedium:
      return 250
    case .accessibilityLarge:
      return 300
    case .accessibilityExtraLarge:
      return 400
    case .accessibilityExtraExtraLarge:
      return 500
    default:
      return 600
    }
  }

  @MainActor required dynamic init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
