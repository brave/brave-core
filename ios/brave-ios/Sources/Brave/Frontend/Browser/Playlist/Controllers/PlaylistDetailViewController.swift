// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import Shared
import Data
import UIKit
import Playlist

class PlaylistDetailViewController: UIViewController, UIGestureRecognizerDelegate {

  private var playerView: VideoView?
  private let isPrivateBrowsing: Bool
  weak var delegate: PlaylistViewControllerDelegate?
  
  init(isPrivateBrowsing: Bool) {
    self.isPrivateBrowsing = isPrivateBrowsing
    super.init(nibName: nil, bundle: nil)
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()

    setup()
    layoutBarButtons()
    addGestureRecognizers()
  }

  // MARK: Private

  private func setup() {
    view.backgroundColor = .black

    navigationController?.do {
      let appearance = UINavigationBarAppearance()
      appearance.configureWithTransparentBackground()
      appearance.titleTextAttributes = [.foregroundColor: UIColor.white]
      appearance.backgroundColor = .braveBackground

      $0.navigationBar.standardAppearance = appearance
      $0.navigationBar.barTintColor = UIColor.braveBackground
      $0.navigationBar.tintColor = .white
    }
  }

  private func layoutBarButtons() {
    let exitBarButton = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(onExit(_:)))
    let sideListBarButton = UIBarButtonItem(image: UIImage(named: "playlist_split_navigation", in: .module, compatibleWith: nil)!, style: .done, target: self, action: #selector(onDisplayModeChange))

    navigationItem.rightBarButtonItem =
      PlayListSide(rawValue: Preferences.Playlist.listViewSide.value) == .left ? exitBarButton : sideListBarButton
    navigationItem.leftBarButtonItem =
      PlayListSide(rawValue: Preferences.Playlist.listViewSide.value) == .left ? sideListBarButton : exitBarButton
  }

  private func addGestureRecognizers() {
    let slideToRevealGesture = UISwipeGestureRecognizer(target: self, action: #selector(handleGesture))
    slideToRevealGesture.direction = PlayListSide(rawValue: Preferences.Playlist.listViewSide.value) == .left ? .right : .left

    view.addGestureRecognizer(slideToRevealGesture)
  }

  private func updateSplitViewDisplayMode(to displayMode: UISplitViewController.DisplayMode) {
    UIView.animate(withDuration: 0.2) {
      self.splitViewController?.preferredDisplayMode = displayMode
    }
  }

  // MARK: Actions

  func onSidePanelStateChanged() {
    onDisplayModeChange()
  }

  func onFullscreen() {
    navigationController?.setNavigationBarHidden(true, animated: true)

    if navigationController?.isNavigationBarHidden == true {
      splitViewController?.preferredDisplayMode = .secondaryOnly
    }
  }

  func onExitFullscreen() {
    navigationController?.setNavigationBarHidden(false, animated: true)

    if navigationController?.isNavigationBarHidden == true {
      splitViewController?.preferredDisplayMode = .oneOverSecondary
    }
  }

  @objc
  private func onExit(_ button: UIBarButtonItem) {
    dismiss(animated: true, completion: nil)
  }

  @objc
  func handleGesture(gesture: UISwipeGestureRecognizer) {
    guard gesture.direction == .right,
      let playerView = playerView,
      !playerView.controlsView.trackBar.frame.contains(gesture.location(in: view))
    else {
      return
    }

    onDisplayModeChange()
  }

  @objc
  private func onDisplayModeChange() {
    updateSplitViewDisplayMode(
      to: splitViewController?.displayMode == .oneOverSecondary ? .secondaryOnly : .oneOverSecondary)
  }

  public func setVideoPlayer(_ videoPlayer: VideoView?) {
    if playerView?.superview == view {
      playerView?.removeFromSuperview()
    }

    playerView = videoPlayer
  }

  public func updateLayoutForMode(_ mode: UIUserInterfaceIdiom) {
    guard let playerView = playerView else { return }

    if mode == .pad {
      view.addSubview(playerView)
      playerView.snp.makeConstraints {
        $0.bottom.leading.trailing.equalTo(view)
        $0.top.equalTo(view.safeAreaLayoutGuide)
      }
    } else {
      if playerView.superview == view {
        playerView.removeFromSuperview()
      }
    }
  }
}

// MARK: - Error Handling

extension PlaylistDetailViewController {
  func displayExpiredResourceError(item: PlaylistInfo?) {
    if let item = item {
      let alert = UIAlertController(
        title: Strings.PlayList.expiredAlertTitle,
        message: Strings.PlayList.expiredAlertDescription, preferredStyle: .alert)
      alert.addAction(
        UIAlertAction(
          title: Strings.PlayList.reopenButtonTitle, style: .default,
          handler: { _ in

            if let url = URL(string: item.pageSrc) {
              self.dismiss(animated: true, completion: nil)
              
              self.delegate?.openURLInNewTab(
                url,
                isPrivate: self.isPrivateBrowsing,
                isPrivileged: false)
            }
          }))
      alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
      self.present(alert, animated: true, completion: nil)
    } else {
      let alert = UIAlertController(
        title: Strings.PlayList.expiredAlertTitle,
        message: Strings.PlayList.expiredAlertDescription, preferredStyle: .alert)
      alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
      self.present(alert, animated: true, completion: nil)
    }
  }

  func displayLoadingResourceError() {
    let alert = UIAlertController(
      title: Strings.PlayList.sorryAlertTitle, message: Strings.PlayList.loadResourcesErrorAlertDescription, preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: Strings.PlayList.okayButtonTitle, style: .default, handler: nil))

    self.present(alert, animated: true, completion: nil)
  }
}
