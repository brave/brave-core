// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Preferences
import Shared
import SwiftUI
import UIKit

class NTPDefaultBrowserCalloutProvider: NSObject, NTPObservableSectionProvider {
  var sectionDidChange: (() -> Void)?
  private let isBackgroundNTPSI: Bool

  // MARK: Lifecycle
  init(isBackgroundNTPSI: Bool) {
    self.isBackgroundNTPSI = isBackgroundNTPSI
  }

  // MARK: UICollectionViewDelegate

  func collectionView(
    _ collectionView: UICollectionView,
    numberOfItemsInSection section: Int
  ) -> Int {
    shouldShowCallout() ? 1 : 0
  }

  func collectionView(
    _ collectionView: UICollectionView,
    cellForItemAt indexPath: IndexPath
  ) -> UICollectionViewCell {
    let cell =
      collectionView.dequeueReusableCell(for: indexPath) as DefaultBrowserCalloutNTPWidgetCell
    cell.contentConfiguration = UIHostingConfiguration {
      DefaultBrowserCalloutView(
        openSettings: { [unowned self] in
          openSettings()
        },
        dismiss: { [unowned self] in
          Preferences.General.defaultBrowserCalloutDismissed.value = true
          sectionDidChange?()
        }
      )
      .frame(maxWidth: 640)
      .fixedSize(horizontal: false, vertical: true)
    }.margins(.all, 0)
    return cell
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    sizeForItemAt indexPath: IndexPath
  ) -> CGSize {
    var size = fittingSizeForCollectionView(collectionView, section: indexPath.section)
    size.height = 60
    return size
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    insetForSectionAt section: Int
  ) -> UIEdgeInsets {
    if !shouldShowCallout() {
      return .zero
    }
    let insets = horizontalInsets(for: collectionView, maxWidth: 640, minimumInset: 16)
    return UIEdgeInsets(top: 8, left: insets.left, bottom: 0, right: insets.right)
  }

  func registerCells(to collectionView: UICollectionView) {
    collectionView.register(DefaultBrowserCalloutNTPWidgetCell.self)
  }

  func shouldShowCallout() -> Bool {
    return true
    // Never show Default Browser Notification over an NPT SI
    if isBackgroundNTPSI {
      return false
    }

    let defaultBrowserDisplayCriteria =
      !Preferences.General.defaultBrowserCalloutDismissed.value

    guard let appRetentionLaunchDate = Preferences.DAU.appRetentionLaunchDate.value else {
      return defaultBrowserDisplayCriteria
    }

    // User should not see default browser first 7 days
    // also after 14 days
    var defaultBrowserTimeConstraintCriteria = false

    let rightNow = Date()
    let first7DayPeriod = appRetentionLaunchDate.addingTimeInterval(7.days)
    let first14DayPeriod = appRetentionLaunchDate.addingTimeInterval(14.days)

    if rightNow > first7DayPeriod, rightNow < first14DayPeriod {
      defaultBrowserTimeConstraintCriteria = true
    }

    return defaultBrowserDisplayCriteria && defaultBrowserTimeConstraintCriteria
  }

  private func openSettings() {
    guard let settingsUrl = URL(string: UIApplication.openSettingsURLString) else {
      return
    }

    Preferences.General.defaultBrowserCalloutDismissed.value = true
    UIApplication.shared.open(settingsUrl)
  }
}

private class DefaultBrowserCalloutNTPWidgetCell: UICollectionViewCell, CollectionViewReusable {
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

private struct DefaultBrowserCalloutView: View {
  var openSettings: () -> Void
  var dismiss: () -> Void

  var body: some View {
    Button {
      openSettings()
    } label: {
      HStack(alignment: .top) {
        HStack {
          Image(braveSystemName: "leo.set.as-default")
            .font(.title3)
            .padding(8)
            .background(Color.black.opacity(0.3), in: .rect(cornerRadius: 8, style: .continuous))
          Text(LocalizedStringKey(Strings.setDefaultBrowserCalloutTitle))
            .font(.footnote)
        }
        .accessibilityElement()
        .foregroundStyle(.white)
        Spacer()
        Button {
          dismiss()
        } label: {
          Label(Strings.close, braveSystemImage: "leo.close")
            .foregroundStyle(.white.opacity(0.5))
            .labelStyle(.iconOnly)
        }
        .font(.callout)
      }
      .padding()
    }
    .buttonStyle(NewTabPageButtonStyle())
    .colorScheme(.dark)
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.xLarge)
  }
}

struct NewTabPageButtonStyle: ButtonStyle {
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
