// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import BraveStrings
import BraveUI
import Foundation
import Preferences
import Shared
import SwiftUI
import UIKit

class StatsSectionProvider: NSObject, NTPSectionProvider {
  private let isPrivateBrowsing: Bool
  var openPrivacyHubPressed: () -> Void
  var hidePrivacyHubPressed: () -> Void

  init(
    isPrivateBrowsing: Bool,
    openPrivacyHubPressed: @escaping () -> Void,
    hidePrivacyHubPressed: @escaping () -> Void
  ) {
    self.isPrivateBrowsing = isPrivateBrowsing
    self.openPrivacyHubPressed = openPrivacyHubPressed
    self.hidePrivacyHubPressed = hidePrivacyHubPressed
  }

  func collectionView(
    _ collectionView: UICollectionView,
    numberOfItemsInSection section: Int
  ) -> Int {
    return Preferences.NewTabPage.showNewTabPrivacyHub.value ? 1 : 0
  }

  func registerCells(to collectionView: UICollectionView) {
    collectionView.register(StatsNTPWidgetCell.self)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    cellForItemAt indexPath: IndexPath
  ) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(for: indexPath) as StatsNTPWidgetCell
    cell.contentConfiguration = UIHostingConfiguration {
      StatsNTPWidget(
        isPrivateBrowsing: isPrivateBrowsing
      ) { [unowned self] in
        openPrivacyHubPressed()
      } hidePrivacyHubPressed: { [unowned self] in
        hidePrivacyHubPressed()
      }
      .frame(maxWidth: 640)
      .fixedSize(horizontal: false, vertical: true)
    }
    .margins(.all, 0)
    return cell
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    sizeForItemAt indexPath: IndexPath
  ) -> CGSize {
    var size = fittingSizeForCollectionView(collectionView, section: indexPath.section)
    size.height = 110
    return size
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    insetForSectionAt section: Int
  ) -> UIEdgeInsets {
    let insets = horizontalInsets(for: collectionView, maxWidth: 640, minimumInset: 16)
    return UIEdgeInsets(top: 8, left: insets.left, bottom: 8, right: insets.right)
  }
}

class StatsNTPWidgetCell: UICollectionViewCell, CollectionViewReusable {
  override func preferredLayoutAttributesFitting(
    _ layoutAttributes: UICollectionViewLayoutAttributes
  ) -> UICollectionViewLayoutAttributes {
    let attributes = layoutAttributes.copy() as! UICollectionViewLayoutAttributes
    attributes.size.height =
      systemLayoutSizeFitting(
        layoutAttributes.size,
        withHorizontalFittingPriority: .required,
        verticalFittingPriority: .fittingSizeLevel
      ).height
    return attributes
  }
}

struct StatsNTPWidget: View {
  var isPrivateBrowsing: Bool

  var openPrivacyHubPressed: () -> Void
  var hidePrivacyHubPressed: () -> Void

  private let stats = BraveGlobalShieldStats.shared

  private struct StatLabeledContentStyle: LabeledContentStyle {
    func makeBody(configuration: Configuration) -> some View {
      VStack {
        configuration.content
          .font(.title2)
        configuration.label
          .font(.caption)
          .foregroundStyle(.white)
      }
      .frame(maxWidth: .infinity)
      .multilineTextAlignment(.center)
    }
  }

  private struct StatsButtonStyle: ButtonStyle {
    @Environment(\.isEnabled) private var isEnabled

    func makeBody(configuration: Configuration) -> some View {
      configuration.label
        .osAvailabilityModifiers { content in
          if #available(iOS 26.0, *) {
            content
              .glassEffect(.regular.interactive(isEnabled), in: .rect(cornerRadius: 16))
          } else {
            content
              .background(.thinMaterial, in: .rect(cornerRadius: 16))
              .animation(.spring(response: 0.3, dampingFraction: 0.8)) { content in
                content.scaleEffect(configuration.isPressed ? 0.95 : 1)
              }
          }
        }
    }
  }

  private var adblockCount: Int {
    stats.adblock + stats.trackingProtection
  }

  var body: some View {
    Button {
      openPrivacyHubPressed()
    } label: {
      VStack(spacing: 8) {
        Label("Privacy Hub", braveSystemImage: "leo.shield.done-filled")
          .foregroundStyle(.white)
          .font(.footnote.weight(.semibold))
          .frame(maxWidth: .infinity, alignment: .leading)
        HStack {
          LabeledContent {
            Text(adblockCount.kFormattedNumber)
              .foregroundStyle(Color(braveSystemName: .primitiveOrange70))
          } label: {
            Text(Strings.Shields.shieldsAdAndTrackerStats.capitalized)
          }
          LabeledContent {
            Text(stats.dataSaved)
              .foregroundStyle(Color(braveSystemName: .primitiveBlurple70))
          } label: {
            Text(Strings.Shields.dataSavedStat)
          }
          LabeledContent {
            Text(stats.timeSaved)
              .foregroundStyle(.white)
          } label: {
            Text(Strings.Shields.shieldsTimeStats)
          }
        }
        .labeledContentStyle(StatLabeledContentStyle())
      }
      .padding()
      .frame(maxWidth: .infinity)
      .contentShape(.rect)
    }
    .buttonStyle(StatsButtonStyle())
    .colorScheme(.dark)
    .contextMenu {
      Button {
        hidePrivacyHubPressed()
      } label: {
        Label(Strings.PrivacyHub.hidePrivacyHubWidgetActionTitle, braveSystemImage: "leo.eye.off")
      }
    }
    .disabled(isPrivateBrowsing)
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.xLarge)
  }
}
