/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SnapKit

extension WalletViewController {
  
  class View: UIView {
    
    var notificationView: WalletNotificationView? {
      willSet {
        notificationView?.removeFromSuperview()
      }
      didSet {
        guard let notificationView = notificationView else { return }
        addSubview(notificationView)
        notificationView.snp.makeConstraints {
          $0.top.leading.trailing.equalTo(self)
          $0.height.equalTo(headerView).offset(1.0)
        }
      }
    }
    
    func setNotificationView(_ notificationView: WalletNotificationView?, animated: Bool) {
      switch (self.notificationView, notificationView) {
      case (.some(let oldNotificationView), .none):
        // removing, animate out
        UIView.animate(withDuration: 0.15, animations: {
          oldNotificationView.alpha = 0.0
        }) { _ in
          self.notificationView = nil
        }
      case (.none, .some(let newNotificationView)):
        // adding, animate in
        self.notificationView = newNotificationView
        newNotificationView.alpha = 0.0
        UIView.animate(withDuration: 0.15) {
          newNotificationView.alpha = 1.0
        }
      case (.some(let oldNotificationView), .some(let newNotificationView)):
        // replacing, alpha replace
        UIView.animate(withDuration: 0.1, animations: {
          oldNotificationView.subviews.forEach { view in
            view.alpha = 0.0
          }
        }) { _ in
          self.notificationView = newNotificationView
          newNotificationView.subviews.forEach { view in
            view.alpha = 0.0
          }
          UIView.animate(withDuration: 0.1, animations: {
            newNotificationView.subviews.forEach { view in
              view.alpha = 1.0
            }
          })
        }
        break
      case (.none, .none):
        // nothing
        break
      }
    }
    
    let headerView = WalletHeaderView()
    
    var contentView: (UIView & WalletContentView)? {
      willSet {
        if newValue === contentView { return }
        contentView?.removeFromSuperview()
      }
      didSet {
        if oldValue === contentView { return }
        setupContentView()
      }
    }
    
    var rewardsSummaryView: RewardsSummaryView?
    let summaryLayoutGuide = UILayoutGuide()
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      backgroundColor = .white
      clipsToBounds = true
      
      addLayoutGuide(summaryLayoutGuide)
      addSubview(headerView)
      
      headerView.snp.makeConstraints {
        $0.top.leading.trailing.equalTo(self)
      }
      summaryLayoutGuide.snp.makeConstraints {
        $0.top.equalTo(self.headerView.snp.bottom).offset(20.0)
        $0.leading.trailing.equalTo(self)
        $0.bottom.equalTo(self)
      }
      
      setupContentView()
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
    
    // MARK: -
    
    private func setupContentView() {
      guard let contentView = contentView else { return }
      
      insertSubview(contentView, belowSubview: headerView)
      
      contentView.snp.makeConstraints {
        if let _ = contentView.innerScrollView {
          $0.top.equalTo(self)
        } else {
          $0.top.equalTo(self.headerView.snp.bottom)
        }
        $0.leading.trailing.equalTo(self)
      }
      
      if let rewardsSummaryView = rewardsSummaryView, contentView.displaysRewardsSummaryButton {
        // Hide this by default, it'll be shown when the user expands it
        rewardsSummaryView.monthYearLabel.isHidden = true
        
        insertSubview(rewardsSummaryView, belowSubview: headerView)
        contentView.snp.makeConstraints {
          $0.bottom.equalTo(rewardsSummaryView.snp.top)
        }
        rewardsSummaryView.snp.makeConstraints {
          $0.leading.trailing.equalTo(self)
          $0.height.equalTo(self.summaryLayoutGuide)
        }
        rewardsSummaryView.rewardsSummaryButton.snp.makeConstraints {
          $0.bottom.equalTo(self)
        }
      } else {
        rewardsSummaryView?.removeFromSuperview()
        contentView.snp.makeConstraints {
          $0.bottom.equalTo(self)
        }
      }
    }
  }
}
