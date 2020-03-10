/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards
import BraveShared

public class RewardsPanelController: PopoverNavigationController {
  
  public enum InitialPage {
    case `default`
    case settings
  }

  public static let batLogoImage = UIImage(frameworkResourceNamed: "bat-small")
  
  private let state: RewardsState
  
  public init(_ rewards: BraveRewards, tabId: UInt64, url: URL, faviconURL: URL?, pageHTML: String? = nil, delegate: RewardsUIDelegate, dataSource: RewardsDataSource, initialPage: InitialPage = .default) {
    state = RewardsState(ledger: rewards.ledger, ads: rewards.ads, tabId: tabId, url: url, faviconURL: faviconURL, delegate: delegate, dataSource: dataSource)
    
    super.init()
    
    if !rewards.ledger.isWalletCreated {
      viewControllers = [CreateWalletViewController(state: state)]
    } else {
      var vcs: [UIViewController] = [WalletViewController(state: state)]
      if initialPage == .settings {
        vcs.append(SettingsViewController(state: state))
      }
      viewControllers = vcs
    }
  }
  
  private var errorOverlayView: UIView?
  
  public override func viewDidLoad() {
    super.viewDidLoad()
    
    navigationBar.appearanceBarTintColor = navigationBar.barTintColor
    navigationBar.tintColor = Colors.blurple400
    navigationBar.titleTextAttributes = [.foregroundColor: UIColor.black]
    
    toolbar.appearanceBarTintColor = toolbar.barTintColor
    toolbar.tintColor = Colors.blurple400
    
    if #available(iOS 13.0, *) {
      overrideUserInterfaceStyle = .light
    }
    
    if state.ledger.dataMigrationFailed && !Preferences.Rewards.seenDataMigrationFailureError.value {
      let errorView = LedgerInitializationFailedView(
        failureMessage: Strings.ledgerDatabaseMigrationFailedBody,
        dismissed: { [weak self] in
          guard let self = self else { return }
          Preferences.Rewards.seenDataMigrationFailureError.value = true
          UIView.animate(withDuration: 0.25, animations: {
            self.errorOverlayView?.alpha = 0.0
          }, completion: { _ in
            self.errorOverlayView?.removeFromSuperview()
            self.errorOverlayView = nil
          })
        }
      )
      view.addSubview(errorView)
      errorView.snp.makeConstraints {
        $0.edges.equalTo(self.view)
      }
      errorOverlayView = errorView
    }
  }
}
