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
  private let shredSiteDataNow: () -> Void
  @State private var showConfirmation = false

  init(url: URL, isPersistent: Bool, shredSiteDataNow: @escaping () -> Void) {
    self.settings = DomainSettings(url: url, isPersistent: isPersistent)
    self.shredSiteDataNow = shredSiteDataNow
  }

  var body: some View {
    Form {
      Section {
        Picker(selection: $settings.shredLevel) {
          ForEach(SiteShredLevel.allCases) { level in
            Text(level.localizedTitle)
              .foregroundColor(Color(.secondaryBraveLabel))
              .tag(level)
          }
        } label: {
          LabelView(
            title: Strings.Shields.autoShred,
            subtitle: nil
          )
        }

        Button(
          action: {
            showConfirmation = true
          },
          label: {
            HStack(alignment: .center) {
              Text(Strings.Shields.shredSiteDataNow)
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
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
    .navigationTitle(Strings.Shields.shredSiteData)
    .toolbar(.visible)
  }

  private var confirmationAlert: Alert {
    Alert(
      title: Text(Strings.Shields.shredSiteDataConfirmationTitle),
      message: Text(
        Strings.Shields.shredSiteDataConfirmationMessage
      ),
      primaryButton: .destructive(
        Text(Strings.Shields.shredDataButtonTitle),
        action: {
          shredSiteDataNow()
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
      return Strings.Shields.shredOnSiteTabsClosed
    case .appExit:
      return Strings.Shields.shredOnAppClose
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
    shredSiteDataNow: @escaping () -> Void
  ) {
    super.init(
      rootView: ShredSettingsView(
        url: url,
        isPersistent: isPersistent,
        shredSiteDataNow: shredSiteDataNow
      )
    )
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    navigationItem.title = Strings.Shields.shredSiteData
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    setPreferredContentSize()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    setPreferredContentSize()
    view.setNeedsLayout()
  }

  /// Will set the preferred size
  @objc private func setPreferredContentSize() {
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

  @MainActor required dynamic init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
