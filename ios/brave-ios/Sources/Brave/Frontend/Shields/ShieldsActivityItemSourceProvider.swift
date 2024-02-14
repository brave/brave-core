// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import LinkPresentation
import BraveShared
import BraveUI
import Shared

// MARK: - ActivityTypeValue

public enum ActivityTypeValue: String, CaseIterable {
  case whatsapp = "net.whatsapp.WhatsApp.ShareExtension"
  case slack = "com.tinyspeck.chatlyio.share"
  case gmail = "com.google.Gmail.ShareExtension"
  case instagram = "com.burbn.instagram.shareextension"
}

// MARK: - ShieldsActivityItemSourceProvider

final class ShieldsActivityItemSourceProvider {

  static let shared = ShieldsActivityItemSourceProvider()

  func setupGlobalShieldsActivityController(isPrivateBrowsing: Bool) -> UIActivityViewController {
    let backgroundImage = UIImage(named: "share-activity-background", in: .module, compatibleWith: nil)!

    let statsView = UIView(frame: CGRect(size: backgroundImage.size)).then {
      let backgroundImageView = UIImageView(image: backgroundImage)
      let statsInfoView = BraveShieldStatsView()
      statsInfoView.isPrivateBrowsing = isPrivateBrowsing

      $0.addSubview(backgroundImageView)
      $0.addSubview(statsInfoView)

      backgroundImageView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }
      statsInfoView.snp.makeConstraints {
        $0.centerX.equalToSuperview()
        $0.centerY.equalToSuperview().offset(15)
        $0.height.equalToSuperview().multipliedBy(0.40)
        $0.width.equalToSuperview().multipliedBy(0.70)
      }
    }

    let contentView = UIView(frame: CGRect(width: statsView.frame.width, height: statsView.frame.height + 85)).then {
      $0.backgroundColor = #colorLiteral(red: 0.2980392157, green: 0.3294117647, blue: 0.8235294118, alpha: 1)
      $0.layer.borderWidth = 1
    }

    contentView.addSubview(statsView)
    statsView.frame = CGRect(
      origin: .zero,
      size: CGSize(width: statsView.frame.width, height: statsView.frame.height))

    let snapshotImage = statsView.snapshot
    let snapshotImageWithText =
      contentView.snapshot.textToImage(
        drawText: Strings.ShieldEducation.shareDescriptionTitle,
        atPoint: CGPoint(x: 0, y: statsView.frame.height + 20)) ?? snapshotImage

    let activityViewController = UIActivityViewController(
      activityItems: [
        ImageActivityItemSource(
          image: snapshotImage,
          imageWithText: snapshotImageWithText),
        OptionalTextActivityItemSource(text: Strings.ShieldEducation.shareDescriptionTitle),
      ],
      applicationActivities: nil)

    activityViewController.excludedActivityTypes = [.openInIBooks, .saveToCameraRoll, .assignToContact]

    return activityViewController
  }

}

// MARK: - OptionalTextActivityItemSource

class OptionalTextActivityItemSource: NSObject, UIActivityItemSource {

  let text: String

  weak var viewController: UIViewController?

  init(text: String) {
    self.text = text

    super.init()
  }

  func activityViewControllerPlaceholderItem(_ activityViewController: UIActivityViewController) -> Any {
    return text
  }

  func activityViewController(_ activityViewController: UIActivityViewController, itemForActivityType activityType: UIActivity.ActivityType?) -> Any? {
    let activityValueType = ActivityTypeValue.allCases.first(where: { $0.rawValue == activityType?.rawValue })

    return activityValueType == nil ? text : nil
  }
}

// MARK: - ImageActivityItemSource

class ImageActivityItemSource: NSObject, UIActivityItemSource {
  let image: UIImage
  let imageWithText: UIImage

  init(image: UIImage, imageWithText: UIImage) {
    self.image = image
    self.imageWithText = imageWithText

    super.init()
  }

  func activityViewControllerPlaceholderItem(_ activityViewController: UIActivityViewController) -> Any {
    return image
  }

  func activityViewController(_ activityViewController: UIActivityViewController, itemForActivityType activityType: UIActivity.ActivityType?) -> Any? {
    let activityValueType = ActivityTypeValue.allCases.first(where: { $0.rawValue == activityType?.rawValue })

    return activityValueType == nil ? image : imageWithText
  }

  func activityViewControllerLinkMetadata(_ activityViewController: UIActivityViewController) -> LPLinkMetadata? {
    let imageProvider = NSItemProvider(object: image)

    let metadata = LPLinkMetadata()
    metadata.imageProvider = imageProvider
    metadata.title = Strings.ShieldEducation.shareDescriptionTitle
    return metadata
  }
}
