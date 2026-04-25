// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import GuardianConnect
import Lottie
import NetworkExtension
import Shared
import UIKit

public class BraveVPNPickerViewController: UIViewController {

  let tableView: UITableView = .init(frame: .zero, style: .insetGrouped)

  private lazy var overlayView = UIView().then {
    $0.backgroundColor = UIColor.black.withAlphaComponent(0.5)

    let activityIndicator = UIActivityIndicatorView().then {
      $0.style = .large
      $0.color = .white
      $0.autoresizingMask = [.flexibleWidth, .flexibleHeight]
      $0.startAnimating()
    }

    $0.addSubview(activityIndicator)
  }

  var isLoading: Bool = false {
    didSet {
      navigationItem.hidesBackButton = isLoading

      // Prevent dismissing the modal by swipe when the VPN is being configured
      navigationController?.isModalInPresentation = isLoading

      if isLoading {
        view.addSubview(overlayView)
        overlayView.snp.makeConstraints {
          $0.edges.equalToSuperview()
        }
      } else {
        if overlayView.isDescendant(of: view) {
          overlayView.removeFromSuperview()
        }
      }
    }
  }

  public init() {
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) { fatalError() }

  public override func viewDidLoad() {
    tableView.register(VPNRegionCell.self)

    NotificationCenter.default.addObserver(
      self,
      selector: #selector(vpnConfigChanged(notification:)),
      name: .NEVPNStatusDidChange,
      object: nil
    )

    view.addSubview(tableView)
    tableView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  public override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    tableView.reloadData()
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  @objc func vpnConfigChanged(notification: NSNotification) {}

  func showSuccessAlert(text: String) {
    let animation = LottieAnimationView(name: "vpncheckmark", bundle: .module).then {
      $0.bounds = CGRect(x: 0, y: 0, width: 300, height: 200)
      $0.contentMode = .scaleAspectFill
      $0.play()
    }

    let popup = AlertPopupView(
      imageView: animation,
      title: text,
      message: "",
      titleWeight: .semibold,
      titleSize: 18,
      dismissHandler: { true }
    )

    popup.showWithType(showType: .flyUp, autoDismissTime: 1.5)
  }

  func showErrorAlert(title: String, message: String?) {
    DispatchQueue.main.async {
      let alert = AlertController(
        title: Strings.VPN.regionPickerErrorTitle,
        message: Strings.VPN.regionPickerErrorMessage,
        preferredStyle: .alert
      )
      let okAction = UIAlertAction(title: Strings.OKString, style: .default) { _ in
        self.dismiss(animated: true)
      }
      alert.addAction(okAction)

      self.present(alert, animated: true)
    }
  }
}

class VPNRegionCell: UITableViewCell, TableViewReusable {
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .value1, reuseIdentifier: reuseIdentifier)
  }
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
