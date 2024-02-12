// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import DesignSystem

/// The button which displays a more icon (`•••`) and a set of badges
class MenuButton: ToolbarButton {
  /// A badge that can be added to the menu icon
  struct Badge: Hashable {
    var gradientView: () -> BraveGradientView
    var icon: UIImage?

    func hash(into hasher: inout Hasher) {
      hasher.combine(icon)
    }

    static func == (lhs: Self, rhs: Self) -> Bool {
      return lhs.icon == rhs.icon
    }

    static let playlist: Self = .init(
      gradientView: { .gradient02 },
      icon: UIImage(named: "playlist-menu-badge", in: .module, compatibleWith: nil)?.withTintColor(.white)
    )
  }

  private(set) var badges: [Badge: UIView] = [:]

  func setBadges(_ badges: [Badge]) {
    badges.forEach { [self] in
      addBadge($0, animated: false)
    }
  }

  func addBadge(_ badge: Badge, animated: Bool) {
    if badges[badge] != nil {
      // Badge already exists
      return
    }

    guard let imageView = imageView else { return }

    let view = BadgeView(badge: badge)
    badges[badge] = view
    addSubview(view)
    if badges.count > 1 {
      // TODO: All badges become background only
    } else {
      view.snp.makeConstraints {
        $0.centerY.equalTo(imageView.snp.top)
        $0.centerX.equalTo(imageView.snp.trailing)
        $0.size.equalTo(13)
      }
      if animated {
        view.transform = CGAffineTransform(scaleX: 0.0001, y: 0.0001)
        UIViewPropertyAnimator(duration: 0.4, dampingRatio: 0.6) {
          view.transform = .identity
        }
        .startAnimation()
      }
    }
  }

  func removeBadge(_ badge: Badge, animated: Bool) {
    let view = badges[badge]
    badges[badge] = nil
    if let view = view {
      if animated {
        let animator = UIViewPropertyAnimator(duration: 0.3, dampingRatio: 1.0) {
          view.transform = CGAffineTransform(scaleX: 0.0001, y: 0.0001)
        }
        animator.addCompletion { _ in
          view.removeFromSuperview()
        }
        animator.startAnimation()
      } else {
        view.removeFromSuperview()
      }
    }
  }

  private class BadgeView: UIView {
    let badge: Badge
    var contentView: UIView?

    init(badge: Badge) {
      self.badge = badge
      super.init(frame: .zero)
      clipsToBounds = true

      snp.makeConstraints {
        $0.width.greaterThanOrEqualTo(snp.height)
      }

      let backgroundView = badge.gradientView()
      addSubview(backgroundView)
      backgroundView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }

      if let image = badge.icon {
        let imageView = UIImageView(image: image)
        imageView.contentMode = .center
        addSubview(imageView)
        imageView.snp.makeConstraints {
          $0.edges.equalToSuperview().inset(1)
        }
        contentView = imageView
      }
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
    override func layoutSubviews() {
      super.layoutSubviews()
      layer.cornerRadius = bounds.height / 2.0
    }
  }
}
