// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import DesignSystem
import Favicon
import Strings
import SwiftUI

public struct ShieldsPanelView: View {
  /// Called with the height required to display the panel's contents without scrolling
  public var onContentHeightChanged: ((CGFloat) -> Void)?

  private let url: URL
  private let displayHost: String
  private let isShredEnabled: Bool
  @ObservedObject private var viewModel: ShieldsPanelViewModel
  private var action: (ShieldsPanelAction) -> Void

  @ScaledMetric private var faviconCircleSize = 32

  public init(
    url: URL,
    viewModel: ShieldsPanelViewModel,
    isShredEnabled: Bool,
    action: @escaping (ShieldsPanelAction) -> Void
  ) {
    self.url = url
    self.viewModel = viewModel
    self.isShredEnabled = isShredEnabled
    self.action = action
    self.displayHost =
      "\u{200E}\(URLFormatter.formatURLOrigin(forDisplayOmitSchemePathAndTrivialSubdomains: url.strippingBlobURLAuth.absoluteString))"
  }

  @AppStorage("advancedShieldsExpanded") private var isAdvancedControlsExpanded: Bool = false

  public var body: some View {
    NavigationStack {
      Form {
        Toggle(isOn: $viewModel.shieldsEnabled) {
          HStack {
            FaviconImage(
              url: url,
              isPrivateBrowsing: viewModel.isPrivateBrowsing
            )
            .padding(8)
            .frame(width: faviconCircleSize, height: faviconCircleSize, alignment: .center)
            .background(Color(braveSystemName: .pageBackground), in: .circle)
            .shadow(radius: 1.5, x: 0, y: 1)
            VStack(alignment: .leading) {
              URLElidedText(text: displayHost)
                .font(.title2)
                .fontWeight(.semibold)
                .foregroundStyle(viewModel.shieldsEnabled ? .primary : .secondary)
              Group {
                Text(
                  LocalizedStringKey(
                    viewModel.shieldsEnabled
                      ? Strings.Shields.shieldsUpForSite : Strings.Shields.shieldsDownForSite
                  )
                )
              }
              .font(.footnote)
              .foregroundStyle(.secondary)
            }
          }
        }
        .tint(Color(braveSystemName: .primary40))
        .listRowBackground(Color.clear)
        .onChange(of: viewModel.shieldsEnabled) { _, _ in
          action(.changedShieldSettings)
        }

        if viewModel.shieldsEnabled {
          shieldsUpView
        } else {
          shieldsDownView
        }
      }
      .listSectionSpacing(.compact)
      .contentMargins(.top, 16, for: .scrollContent)
      // `isAdvancedControlsExpanded` is backed by UserDefaults, so it's change
      // lands outside of a `withAnimation` transaction. Need to animate off
      // value instead.
      .animation(.default, value: isAdvancedControlsExpanded)
      // A `Form` has no usable ideal size (it always fills its container),
      // so pull height from underlying ScrollView instead.
      .onScrollGeometryChange(for: CGFloat.self) { geometry in
        geometry.contentSize.height
      } action: { _, height in
        onContentHeightChanged?(height)
      }
    }
  }

  @ViewBuilder private var shieldsUpView: some View {
    Section {
      HStack {
        Text("\(viewModel.stats.total)")
          .font(.title2)
          .fontWeight(.semibold)
        Text(Strings.Shields.trackersAdsAndMoreBlocked)
          .font(.footnote)
      }
    }

    Section {
      if viewModel.advancedControlsEnabled {
        DisclosureGroup(isExpanded: $isAdvancedControlsExpanded) {
          advancedShieldsSection
        } label: {
          Text(Strings.Shields.advancedControls)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .disclosureGroupStyle(ShieldsPanelDisclosureStyle())
      }
    } footer: {
      Text(
        LocalizedStringKey(
          String.localizedStringWithFormat(
            Strings.Shields.siteSeemsBroken,
            URL.Brave.privacyFeatures.absoluteString
          )
        )
      )
      .foregroundStyle(Color.secondary)
      .font(.caption)
      .listRowBackground(Color.clear)
    }
  }

  @ViewBuilder private var advancedShieldsSection: some View {
    Picker(selection: $viewModel.blockAdsAndTrackingLevel) {
      ForEach(ShieldLevel.allCases) { level in
        Text(level.localizedTitle)
          .tag(level)
      }
    } label: {
      Text(Strings.Shields.trackersAndAdsBlocking)
    }
    .onChange(of: viewModel.blockAdsAndTrackingLevel) { _, _ in
      action(.changedShieldSettings)
    }

    Toggle(Strings.Shields.blockScripts, isOn: $viewModel.blockScripts)
      .tint(Color(braveSystemName: .primary40))
      .onChange(of: viewModel.blockScripts) { _, _ in
        action(.changedShieldSettings)
      }

    Toggle(Strings.Shields.fingerprintingProtection, isOn: $viewModel.fingerprintProtection)
      .tint(Color(braveSystemName: .primary40))
      .onChange(of: viewModel.fingerprintProtection) { _, _ in
        action(.changedShieldSettings)
      }

    if isShredEnabled {
      NavigationLink {
        ShredSiteSettingsView(viewModel: viewModel) {
          action(.shredSiteData)
        }
      } label: {
        HStack {
          Text(Strings.Shields.shredSiteData)
            .multilineTextAlignment(.leading)
          Spacer()
          Text(viewModel.autoShredLevel.localizedTitle)
            .multilineTextAlignment(.trailing)
            .foregroundStyle(.secondary)
        }
      }
    }

    Button {
      action(.navigate(.globalShields, dismiss: true))
    } label: {
      Label(Strings.Shields.shieldsGlobalSettingsButtonTitle, braveSystemImage: "leo.launch")
        .labelStyle(RightIconLabelStyle())
    }
    .buttonStyle(.plain)
  }

  @ViewBuilder private var shieldsDownView: some View {
    Section {
      HStack {
        Text(Strings.Shields.siteNotWorkingCorrectly)
        Button {
          action(.navigate(.reportBrokenSite, dismiss: true))
        } label: {
          Text(Strings.Shields.reportBrokenSiteButtonTitle)
        }
        .buttonStyle(.filled)
      }
    }
  }
}

struct ShieldsPanelDisclosureStyle: DisclosureGroupStyle {
  func makeBody(configuration: Configuration) -> some View {
    Button {
      configuration.isExpanded.toggle()
    } label: {
      HStack {
        configuration.label
        Image(systemName: "chevron.down")
          .font(.body)
          .rotationEffect(.degrees(configuration.isExpanded ? -180 : 0))
      }
      .contentShape(Rectangle())
    }
    .buttonStyle(.plain)

    if configuration.isExpanded {
      configuration.content
        .transition(
          .asymmetric(
            insertion: .move(edge: .top).combined(with: .opacity),
            removal: .identity
          )
        )
    }
  }
}

private struct RightIconLabelStyle: LabelStyle {
  func makeBody(configuration: Configuration) -> some View {
    HStack {
      configuration.title
      Spacer()
      configuration.icon
    }
  }
}

public class ShieldsPanelViewController: UIHostingController<ShieldsPanelView> {
  public init(
    url: URL,
    viewModel: ShieldsPanelViewModel,
    isShredEnabled: Bool = true,
    action: @escaping (ShieldsPanelAction) -> Void
  ) {
    super.init(
      rootView: ShieldsPanelView(
        url: url,
        viewModel: viewModel,
        isShredEnabled: isShredEnabled,
        action: action
      )
    )
    rootView.onContentHeightChanged = { [weak self] height in
      self?.updateContentHeight(height)
    }
  }

  @MainActor required dynamic init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  /// The height the panel's contents need, as measured by SwiftUI. Zero until the first measurement
  /// lands.
  private var contentHeight: CGFloat = 0

  private var sheetController: UISheetPresentationController? {
    sheetPresentationController
      ?? popoverPresentationController?.adaptiveSheetPresentationController
  }

  private func updateContentHeight(_ height: CGFloat) {
    // Ignore rpunding differences during a resize can't invalidate the detents
    guard height > 0, abs(height - contentHeight) > 1 else { return }
    let isFirstMeasurement = contentHeight == 0
    contentHeight = height
    preferredContentSize = CGSize(width: 375, height: height)
    guard let sheetController else { return }
    if isFirstMeasurement {
      sheetController.invalidateDetents()
    } else {
      sheetController.animateChanges {
        sheetController.invalidateDetents()
      }
    }
  }

  public override func viewIsAppearing(_ animated: Bool) {
    super.viewIsAppearing(animated)

    if let sheetController {
      sheetController.detents = [
        .custom(identifier: .fitsContent) { [weak self] context in
          guard let height = self?.contentHeight, height > 0 else {
            return context.maximumDetentValue
          }
          return min(context.maximumDetentValue, height)
        },
        .large(),
      ]
      sheetController.prefersGrabberVisible = true
      sheetController.prefersEdgeAttachedInCompactHeight = true
    }
  }
}

extension UISheetPresentationController.Detent.Identifier {
  fileprivate static let fitsContent: Self = .init("fitsContent")
}
