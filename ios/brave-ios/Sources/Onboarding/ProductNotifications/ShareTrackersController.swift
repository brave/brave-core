// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared  // Strings (should be moved to this module)
import BraveUI
import Foundation
import Shared

// MARK: TrackingType

public enum TrackingType: Equatable {
  case trackerCountShare(count: Int, description: String)
  case videoAdBlock
  case domainSpecificDataSaved(dataSaved: String)

  var title: String {
    switch self {
    case .trackerCountShare(let count, _):
      return String(format: Strings.ShieldEducation.trackerCountShareTitle, count)
    case .videoAdBlock:
      return Strings.ShieldEducation.videoAdBlockTitle
    case .domainSpecificDataSaved:
      return Strings.ShieldEducation.domainSpecificDataSavedTitle
    }
  }

  var subTitle: String {
    switch self {
    case .trackerCountShare(_, let description):
      return description
    case .videoAdBlock:
      return Strings.ShieldEducation.videoAdBlockSubtitle
    case .domainSpecificDataSaved(let dataSaved):
      return String(format: Strings.ShieldEducation.domainSpecificDataSavedSubtitle, dataSaved)
    }
  }
}

// MARK: - ShareTrackersController

public class ShareTrackersController: UIViewController, PopoverContentComponent {

  // MARK: Action

  public enum Action {
    case shareTheNewsTapped
    case dontShowAgainTapped
  }

  // MARK: Properties

  private let trackingType: TrackingType

  private let shareTrackersView: ShareTrackersView

  public var actionHandler: ((Action) -> Void)?

  // MARK: Lifecycle

  public init(trackingType: TrackingType) {
    self.trackingType = trackingType
    shareTrackersView = ShareTrackersView(trackingType: trackingType)

    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  // MARK: Internal

  public override func viewDidLoad() {
    super.viewDidLoad()

    view.backgroundColor = .braveBackground

    shareTrackersView.actionHandler = { [weak self] action in
      guard let self = self else { return }

      switch action {
      case .didShareTheNewsTapped:
        self.actionHandler?(.shareTheNewsTapped)
      case .didDontShowAgainTapped:
        self.actionHandler?(.dontShowAgainTapped)
      }
    }

    doLayout()
  }

  private func doLayout() {
    view.addSubview(shareTrackersView)

    view.snp.makeConstraints {
      $0.width.equalTo(264)
      $0.height.equalTo(shareTrackersView)
    }

    shareTrackersView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }
}

// MARK: - ShareTrackersView

private class ShareTrackersView: UIView {

  // MARK: UX

  struct UX {
    static let contentMargins: UIEdgeInsets = UIEdgeInsets(equalInset: 32)
    static let actionButtonInsets: UIEdgeInsets = UIEdgeInsets(
      top: 10,
      left: 0,
      bottom: 10,
      right: 0
    )
  }

  // MARK: Action

  enum Action {
    case didShareTheNewsTapped
    case didDontShowAgainTapped
  }

  // MARK: Properties

  private let trackingType: TrackingType

  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 20
  }

  private lazy var titleLabel = UILabel().then {
    $0.backgroundColor = .clear
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.numberOfLines = 0
    $0.textColor = .bravePrimary
  }

  private let subtitleLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 16)
    $0.numberOfLines = 0
    $0.textColor = .bravePrimary
  }

  private lazy var actionButton: InsetButton = {
    let actionButton = InsetButton().then {
      $0.addTarget(self, action: #selector(tappedActionButton), for: .touchUpInside)

      $0.backgroundColor = .braveDarkerBlurple
      $0.contentEdgeInsets = UX.actionButtonInsets
      $0.layer.cornerRadius = 20
      $0.layer.cornerCurve = .continuous
      $0.clipsToBounds = true
      $0.titleLabel?.adjustsFontSizeToFitWidth = true
      $0.titleLabel?.allowsDefaultTighteningForTruncation = true
      $0.setTitleColor(.white, for: .normal)
      $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    }

    return actionButton
  }()

  var actionHandler: ((Action) -> Void)?

  // MARK: Lifecycle

  init(trackingType: TrackingType) {
    self.trackingType = trackingType

    super.init(frame: .zero)

    doLayout()
    setContent()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  private func doLayout() {
    addSubview(stackView)

    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(UX.contentMargins)
    }

    stackView.addStackViewItems(
      .view(
        UIStackView().then {
          $0.alignment = .center
          $0.spacing = 10
          $0.addStackViewItems(
            .view(
              UIStackView().then {
                $0.axis = .vertical
                $0.spacing = 8
                $0.addStackViewItems(
                  .view(titleLabel),
                  .view(subtitleLabel)
                )
                $0.setContentHuggingPriority(.required, for: .vertical)
              }
            )
          )
        }
      )
    )

    switch trackingType {
    case .trackerCountShare, .domainSpecificDataSaved:
      stackView.addArrangedSubview(actionButton)
    default:
      return
    }
  }

  private func setContent() {
    titleLabel.attributedText = {
      let imageAttachment = NSTextAttachment().then {
        switch trackingType {
        case .domainSpecificDataSaved:
          $0.image = UIImage(named: "data-saved-domain", in: .module, compatibleWith: nil)!
        default:
          $0.image = UIImage(named: "share-bubble-shield", in: .module, compatibleWith: nil)!
        }
      }

      let string = NSMutableAttributedString(attachment: imageAttachment)

      string.append(
        NSMutableAttributedString(
          string: trackingType.title,
          attributes: [.font: UIFont.systemFont(ofSize: 20.0)]
        )
      )
      return string.withLineSpacing(2)
    }()

    subtitleLabel.attributedText = NSAttributedString(string: trackingType.subTitle)
      .withLineSpacing(2)

    switch trackingType {
    case .trackerCountShare:
      actionButton.setTitle(Strings.ShieldEducation.shareTheNewsTitle, for: .normal)
      actionButton.addTrailingImageIcon(image: UIImage(sharedNamed: "shields-share")!)
    case .domainSpecificDataSaved:
      actionButton.setTitle(Strings.ShieldEducation.dontShowThisTitle, for: .normal)
    default:
      return
    }
  }

  // MARK: Action
  @objc func tappedActionButton() {
    switch trackingType {
    case .trackerCountShare:
      actionHandler?(.didShareTheNewsTapped)
    case .domainSpecificDataSaved:
      actionHandler?(.didDontShowAgainTapped)
    default:
      return
    }
  }
}
