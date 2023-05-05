// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveUI
import Lottie
import NetworkExtension
import GuardianConnect

public class BraveVPNPickerViewController: UIViewController {

  private var overlayView: UIView?
  let tableView: UITableView = .init(frame: .zero, style: .insetGrouped)

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  var isLoading: Bool = false {
    didSet {
      overlayView?.removeFromSuperview()

      navigationItem.hidesBackButton = isLoading

      // Prevent dismissing the modal by swipe when the VPN is being configured
      navigationController?.isModalInPresentation = isLoading

      if !isLoading { return }

      let overlay = UIView().then {
        $0.backgroundColor = UIColor.black.withAlphaComponent(0.5)
        let activityIndicator = UIActivityIndicatorView().then { indicator in
          indicator.startAnimating()
          indicator.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        }

        $0.addSubview(activityIndicator)
      }

      view.addSubview(overlay)
      overlay.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }

      overlayView = overlay
    }
  }

  public init() {
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) { fatalError() }

  public override func viewDidLoad() {
    tableView.register(VPNRegionCell.self)
    
    NotificationCenter.default.addObserver(self, selector: #selector(vpnConfigChanged(notification:)),
                                           name: .NEVPNStatusDidChange, object: nil)
    
    view.addSubview(tableView)
    tableView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  public override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    tableView.reloadData()
  }
  
  @objc func vpnConfigChanged(notification: NSNotification) { }

  func showSuccessAlert(text: String) {
    let animation = AnimationView(name: "vpncheckmark", bundle: .module).then {
      $0.bounds = CGRect(x: 0, y: 0, width: 300, height: 200)
      $0.contentMode = .scaleAspectFill
      $0.play()
    }
    
    let popup = AlertPopupView(imageView: animation,
                               title: text, message: "",
                               titleWeight: .semibold, titleSize: 18,
                               dismissHandler: { true })
    
    popup.showWithType(showType: .flyUp, autoDismissTime: 1.5)
  }
  
  func showErrorAlert(title: String, message: String?) {
    DispatchQueue.main.async {
      let alert = AlertController(title: Strings.VPN.regionPickerErrorTitle,
                                  message: Strings.VPN.regionPickerErrorMessage,
                                  preferredStyle: .alert)
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
