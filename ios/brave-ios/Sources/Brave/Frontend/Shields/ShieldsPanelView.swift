// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveShields
import BraveUI
import Data
import DesignSystem
import Favicon
import SnapKit
import Strings
import SwiftUI

struct ShieldsPanelView: View {
  enum Action {
    enum NavigationTarget {
      case shareStats
      case reportBrokenSite
      case globalShields
    }
    case navigate(NavigationTarget, dismiss: Bool)
    case changedShieldSettings
    case shredSiteData
  }

  private let url: URL
  private weak var tab: Tab?
  private let displayHost: String
  @AppStorage("advancedShieldsExpanded") private var advancedShieldsExpanded = false
  @ObservedObject private var viewModel: ShieldsSettingsViewModel
  private var actionCallback: (Action) -> Void

  @MainActor init(url: URL, tab: Tab, domain: Domain, callback: @escaping (Action) -> Void) {
    self.url = url
    self.tab = tab
    self.viewModel = ShieldsSettingsViewModel(tab: tab, domain: domain)
    self.actionCallback = callback
    self.displayHost =
      "\u{200E}\(URLFormatter.formatURLOrigin(forDisplayOmitSchemePathAndTrivialSubdomains: url.absoluteString))"
  }

  private var shieldsEnabledAccessibiltyLabel: String {
    viewModel.shieldsEnabled
      ? Strings.Shields.statusValueUp : Strings.Shields.statusValueDown
  }

  var body: some View {
    ScrollView {
      VStack(spacing: 16) {
        headerView

        if viewModel.shieldsEnabled {
          shieldsReportView
          Text(Strings.Shields.siteBroken)
            .font(.caption)
            .foregroundStyle(Color(.secondaryBraveLabel))
            .multilineTextAlignment(.leading)
            .padding(.horizontal)
          DisclosureGroup(isExpanded: $advancedShieldsExpanded) {
            advancedShieldsSection
          } label: {
            Text(Strings.Shields.advancedControls)
              .foregroundStyle(Color(.braveLabel))
              .frame(maxWidth: .infinity, alignment: .leading)
          }.disclosureGroupStyle(ShieldsPanelDisclosureStyle())
        } else {
          shieldsOffFooterView
        }
      }
      .padding(.top)
    }
    .background(Color(.braveBackground))
    .frame(idealWidth: 360, alignment: .center)
    .toolbar(.hidden)
  }

  @ViewBuilder @MainActor private var headerView: some View {
    VStack(alignment: .center, spacing: 8) {
      HStack(alignment: .center, spacing: 8) {
        FaviconImage(
          url: url.absoluteString,
          isPrivateBrowsing: viewModel.isPrivateBrowsing
        )
        URLElidedText(text: displayHost)
          .font(.title2)
          .foregroundStyle(Color(.bravePrimary))
      }
      .frame(minWidth: .zero, alignment: .center)

      ShieldsSwitchView(isEnabled: $viewModel.shieldsEnabled)
        .frame(
          width: ShieldsSwitch.size.width,
          height: ShieldsSwitch.size.height,
          alignment: .center
        ).padding(.top, 8)
        .onChange(of: viewModel.shieldsEnabled) { newValue in
          actionCallback(.changedShieldSettings)
        }

      Group {
        Text(verbatim: "\(Strings.Shields.statusTitle) ")
          + Text(shieldsEnabledAccessibiltyLabel.uppercased()).bold()
      }
      .font(.footnote)
      .foregroundStyle(Color(.secondaryBraveLabel))
      .padding(.bottom, 8)
    }.padding(.horizontal)
  }

  @ViewBuilder private var shieldsReportView: some View {
    HStack(spacing: 2) {
      HStack {
        Text(verbatim: "\(viewModel.stats.total)")
          .frame(minWidth: 30, alignment: .center)
          .foregroundStyle(Color(.bravePrimary))
          .font(.title)
          .padding(0)
        Text(Strings.Shields.blockedCountLabel)
          .foregroundStyle(Color(.braveLabel))
          .font(.caption)
          .lineLimit(4)
          .padding(0)
          .frame(maxWidth: .infinity, alignment: .leading)
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .center)
      .background(Color(.secondaryBraveBackground).cornerRadius(8))

      NavigationLink {
        AboutBraveShieldsView()
      } label: {
        HStack {
          Image(braveSystemName: "leo.help.outline")
            .font(.title2)
        }
        .padding()
        .contentShape(RoundedRectangle(cornerRadius: 8))
      }
      .foregroundStyle(Color(.bravePrimary))
      .frame(maxHeight: .infinity, alignment: .center)
      .background(Color(.secondaryBraveBackground).cornerRadius(8))

      Button {
        actionCallback(.navigate(.shareStats, dismiss: false))
      } label: {
        HStack {
          Image(braveSystemName: "leo.share")
            .font(.title2)
        }
        .padding()
        .contentShape(RoundedRectangle(cornerRadius: 8))
      }
      .foregroundStyle(Color(.bravePrimary))
      .buttonStyle(.plain)
      .frame(maxHeight: .infinity, alignment: .center)
      .background(Color(.secondaryBraveBackground).cornerRadius(8))
    }
    .padding(.horizontal)
  }

  @ViewBuilder private var shieldsOffFooterView: some View {
    VStack(alignment: .center, spacing: 16) {
      Text(Strings.Shields.shieldsDownDisclaimer)
        .font(.caption)
        .foregroundStyle(Color(.secondaryBraveLabel))
        .multilineTextAlignment(.leading)
      Button {
        actionCallback(.navigate(.reportBrokenSite, dismiss: true))
      } label: {
        Text(Strings.Shields.reportABrokenSite)
          .foregroundStyle(Color(.braveLabel))
      }
      .buttonStyle(BraveOutlineButtonStyle(size: .normal))
      .frame(maxWidth: .infinity, alignment: .center)
    }
    .padding(.horizontal)
    .padding(.bottom)
  }

  @ViewBuilder private var advancedShieldsSection: some View {
    VStack(alignment: .leading, spacing: 0) {
      shieldSettingsSectionView
      globalSettingsSectionView
    }
    .padding(0)
  }

  @ViewBuilder private var shieldSettingsSectionView: some View {
    ShieldSettingSectionHeader(
      title: displayHost
    )
    ShieldSettingRow {
      HStack {
        Text(Strings.Shields.trackersAndAdsBlocking)
          .foregroundStyle(Color(.bravePrimary))
          .frame(maxWidth: .infinity, alignment: .leading)

        Picker(selection: $viewModel.blockAdsAndTrackingLevel) {
          ForEach(ShieldLevel.allCases) { level in
            Text(level.localizedTitle).tag(level)
          }
        } label: {
          // The label will not show outside of a form or list
          Text(Strings.Shields.trackersAndAdsBlocking)
        }
        .tint(Color(.secondaryBraveLabel))
        .buttonStyle(.plain)
        .padding(.horizontal, -10)
        .onChange(of: viewModel.blockAdsAndTrackingLevel) { newValue in
          actionCallback(.changedShieldSettings)
        }
      }
    }
    ShieldSettingRow {
      ToggleView(
        title: Strings.blockScripts,
        subtitle: nil,
        toggle: $viewModel.blockScripts
      ) { _ in
        actionCallback(.changedShieldSettings)
      }
    }
    ShieldSettingRow {
      ToggleView(
        title: Strings.fingerprintingProtection,
        subtitle: nil,
        toggle: $viewModel.fingerprintProtection
      ) { _ in
        actionCallback(.changedShieldSettings)
      }
    }
    if FeatureList.kBraveShredFeature.enabled {
      ShieldSettingRow {
        NavigationLink {
          ShredSettingsView(
            url: url,
            isPersistent: !viewModel.isPrivateBrowsing
          ) {
            actionCallback(.shredSiteData)
          }
        } label: {
          ShieldSettingsNavigationWrapper {
            Text(Strings.Shields.shredSiteData)
              .frame(maxWidth: .infinity, alignment: .leading)
              .multilineTextAlignment(.leading)
          }
        }
        .foregroundStyle(Color(.bravePrimary))
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.vertical, 4)
      }
    }
    if FeatureList.kBraveIOSDebugAdblock.enabled, let contentBlocker = tab?.contentBlocker {
      ShieldSettingRow {
        NavigationLink {
          AdblockBlockedRequestsView(
            url: url.baseDomain ?? url.absoluteDisplayString,
            contentBlockerHelper: contentBlocker
          )
        } label: {
          ShieldSettingsNavigationWrapper {
            Text(Strings.Shields.blockedRequestsTitle)
              .frame(maxWidth: .infinity, alignment: .leading)
              .multilineTextAlignment(.leading)
          }
        }
        .foregroundStyle(Color(.bravePrimary))
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.vertical, 4)
      }
    }
  }

  @ViewBuilder private var globalSettingsSectionView: some View {
    ShieldSettingSectionHeader(title: Strings.Shields.globalControls)
    ShieldSettingRow {
      Button {
        actionCallback(.navigate(.globalShields, dismiss: true))
      } label: {
        ShieldSettingsNavigationWrapper {
          Label(
            Strings.Shields.globalChangeButton,
            braveSystemImage: "leo.globe.block"
          )
          .frame(maxWidth: .infinity, alignment: .leading)
          .multilineTextAlignment(.leading)
          .foregroundStyle(Color(.bravePrimary))
          .labelStyle(.titleAndIcon)
        }
      }
      .buttonStyle(.plain)
      .frame(maxWidth: .infinity, alignment: .leading)
    }
  }
}

private struct ShieldSettingRow<Contents>: View where Contents: View {
  @ViewBuilder var contents: () -> Contents

  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      contents()
        .frame(minHeight: 32, alignment: .center)
        .padding(.horizontal)
        .padding(.vertical, 4)
      Divider()
    }
    .frame(maxWidth: .infinity, alignment: .leading)
  }
}

private struct ShieldSettingsNavigationWrapper<Contents>: View where Contents: View {
  @ViewBuilder var contents: () -> Contents

  var body: some View {
    HStack {
      contents()
      // Hack to showing the navigation chevron
      Image(systemName: "chevron.right")
        .font(.footnote)
        .fontWeight(.medium)
        .foregroundStyle(Color(.secondaryBraveLabel))
    }.contentShape(Rectangle())
  }
}

private struct ShieldSettingSectionHeader: View {
  let title: String

  var body: some View {
    VStack(alignment: .leading, spacing: 8) {
      URLElidedText(text: title)
        .font(.footnote)
        .foregroundStyle(Color(.secondaryBraveLabel))
        .textCase(.uppercase)
        .padding(.horizontal)
        .padding(.top, 8)
        .padding(.bottom, 0)
      Divider()
    }.padding(.top, 8)
  }
}

class ShieldsPanelViewController: UIHostingController<ShieldsPanelView>, PopoverContentComponent {
  private let shieldsPanelView: ShieldsPanelView

  init(url: URL, tab: Tab, domain: Domain, callback: @escaping (ShieldsPanelView.Action) -> Void) {
    let shieldsPanelView = ShieldsPanelView(
      url: url,
      tab: tab,
      domain: domain,
      callback: callback
    )
    self.shieldsPanelView = shieldsPanelView
    super.init(rootView: shieldsPanelView)
  }

  @MainActor required dynamic init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    sizingOptions = .preferredContentSize
  }
}

private struct ShieldsPanelDisclosureStyle: DisclosureGroupStyle {
  func makeBody(configuration: Configuration) -> some View {
    VStack(alignment: .leading, spacing: 0) {
      Button {
        withAnimation {
          configuration.isExpanded.toggle()
        }
      } label: {
        VStack(spacing: 0) {
          Divider()
          HStack {
            configuration.label
            Group {
              if configuration.isExpanded {
                Image(systemName: "chevron.down")
              } else {
                Image(systemName: "chevron.right")
              }
            }
            .foregroundStyle(Color(.secondaryBraveLabel))
            .font(.body)
          }
          .frame(maxWidth: .infinity, alignment: .center)
          .padding()
          .contentShape(Rectangle())
        }
      }
      .padding(0)
      .buttonStyle(.plain)
      .frame(maxWidth: .infinity, alignment: .center)
      .background(Color(.secondaryBraveBackground))
      .hoverEffect()

      if configuration.isExpanded {
        configuration.content
      }
    }
  }
}
