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
          Text(Strings.Shields.shredNever)
            .foregroundColor(Color(.secondaryBraveLabel))
            .tag(nil as SiteShredLevel?)

          ForEach(SiteShredLevel.allCases) { level in
            Text(level.localizedTitle)
              .foregroundColor(Color(.secondaryBraveLabel))
              .tag(level as SiteShredLevel?)
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

  @Published var shredLevel: SiteShredLevel? {
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
