/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveUI
import BraveCore
import BraveStrings

class SendTabToSelfController: SendTabTransitioningController {
  
  struct UX {
    static let contentInset = 20.0
    static let preferredSizePadding = 132.0
  }
  
  // MARK: Internal
  
  let contentNavigationController: UINavigationController
  private let sendTabContentController: SendTabToSelfContentController
  
  var sendWebSiteHandler: ((SendableTabInfoDataSource) -> Void)?

  // MARK: Lifecycle
  
  init(sendTabAPI: BraveSendTabAPI, dataSource: SendableTabInfoDataSource) {
    sendTabContentController = SendTabToSelfContentController(sendTabAPI: sendTabAPI, dataSource: dataSource)
    contentNavigationController = UINavigationController(rootViewController: sendTabContentController).then {
      $0.view.layer.cornerRadius = 10.0
      $0.view.layer.cornerCurve = .continuous
      $0.view.clipsToBounds = true
    }
    
    super.init()
        
    addChild(contentNavigationController)
    contentNavigationController.didMove(toParent: self)
    
    sendTabContentController.sendWebSiteHandler = { [weak self] dataSource in
      guard let self = self else { return }
      
      self.dismiss(animated: true) {
        self.sendWebSiteHandler?(dataSource)
      }
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    contentView.addSubview(contentNavigationController.view)

    updateLayoutConstraints()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    
    updateLayoutConstraints()
  }
  
  private func updateLayoutConstraints() {
    let preferredSize = sendTabContentController.view.systemLayoutSizeFitting(
      CGSize(width: view.bounds.size.width, height: view.frame.height),
      withHorizontalFittingPriority: .required,
      verticalFittingPriority: .fittingSizeLevel
    ).with {
      $0.height += UX.preferredSizePadding
    }
    
    contentNavigationController.view.snp.makeConstraints {
      if traitCollection.horizontalSizeClass == .compact && traitCollection.verticalSizeClass == .regular {
        $0.leading.trailing.equalTo(contentView.safeAreaLayoutGuide).inset(UX.contentInset)
      } else {
        $0.width.equalToSuperview().multipliedBy(0.75)
      }
      
      $0.centerX.centerY.equalToSuperview()
      $0.height.equalTo(preferredSize.height)
    }
  }

}

class SendTabToSelfContentController: UITableViewController {
  
  struct UX {
    static let standardItemHeight = 44.0
  }

  // MARK: Internal
  
  private var dataSource: SendableTabInfoDataSource?
  private var sendTabAPI: BraveSendTabAPI?
  
  var sendWebSiteHandler: ((SendableTabInfoDataSource) -> Void)?

  // MARK: Lifecycle
  
  convenience init(sendTabAPI: BraveSendTabAPI, dataSource: SendableTabInfoDataSource) {
    self.init(style: .plain)
    
    self.dataSource = dataSource
    self.sendTabAPI = sendTabAPI
  }

  override init(style: UITableView.Style) {
    super.init(style: style)
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    navigationItem.title = Strings.OpenTabs.sendWebpageScreenTitle
    navigationItem.leftBarButtonItem =
      UIBarButtonItem(title: Strings.cancelButtonTitle, style: .plain, target: self, action: #selector(cancel))

    tableView.do {
      $0.tableHeaderView = UIView()
      $0.register(CenteredButtonCell.self)
      $0.register(TwoLineTableViewCell.self)
      $0.registerHeaderFooter(SendTabToSelfContentHeaderFooterView.self)
      $0.tableFooterView = SendTabToSelfContentHeaderFooterView().then {
        $0.titleLabel.text = Strings.OpenTabs.sendDeviceButtonTitle
        $0.titleLabel.isUserInteractionEnabled = true
        $0.titleLabel.addGestureRecognizer(UITapGestureRecognizer(
          target: self,
          action: #selector(tappedSendLabel(_:))))
      }
    }
  }
  
  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    
    guard let footerView = tableView.tableFooterView else { return }
    
    let size = footerView.systemLayoutSizeFitting(
      CGSize(width: tableView.bounds.size.width, height: UIView.layoutFittingCompressedSize.height))
    
    if footerView.frame.size.height != size.height {
      footerView.frame.size.height = size.height
      tableView.tableFooterView = footerView
    }
  }
  
  @objc func cancel() {
    dismiss(animated: true)
  }
}

// MARK: UITableViewDataSource - UITableViewDelegate

extension SendTabToSelfContentController {
  override func numberOfSections(in tableView: UITableView) -> Int {
    return 1
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    guard let dataSource = dataSource else { return 0 }

    return dataSource.numberOfDevices()
  }
  
  override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    UITableView.automaticDimension
  }
  
  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(for: indexPath) as TwoLineTableViewCell

    if let device = dataSource?.deviceInformation(for: indexPath) {
      var deviceTypeImage: UIImage?
      
      switch device.deviceType {
      case .mobile:
        deviceTypeImage = UIImage(braveSystemNamed: "brave.tablet.and.phone")
      case .PC:
        deviceTypeImage = UIImage(braveSystemNamed: "brave.laptop")
      default:
        deviceTypeImage = UIImage(braveSystemNamed: "brave.laptop.and.phone")
      }
      
      cell.do {
        $0.backgroundColor = .clear
        $0.accessoryType = indexPath.row == dataSource?.selectedIndex ? .checkmark : .none
        $0.setLines(device.fullName, detailText: device.lastUpdatedTime.formattedActivePeriodDate)
        $0.detailTextLabel?.font = .preferredFont(forTextStyle: .subheadline)
        $0.imageView?.contentMode = .scaleAspectFit
        $0.imageView?.tintColor = .braveLabel
        $0.imageView?.image = deviceTypeImage?.template
      }
    }
    
    return cell
  }
  
  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    guard let dataSource = dataSource else { return }
    
    dataSource.selectedIndex = indexPath.row
    tableView.reloadSections(IndexSet(integer: indexPath.section), with: .fade)
  }
  
  @objc private func tappedSendLabel(_ gesture: UITapGestureRecognizer) {
    guard let dataSource = dataSource, gesture.state == .ended else { return }
    
    sendWebSiteHandler?(dataSource)
  }
}

class SendTabToSelfContentHeaderFooterView: UITableViewHeaderFooterView, TableViewReusable {
  private struct UX {
    static let horizontalPadding = 15.0
    static let verticalPadding = 12.0
  }
  
  private(set) var titleLabel = UILabel().then {
    $0.numberOfLines = 0
    $0.textColor = .braveBlurpleTint
    $0.textAlignment = .center
  }

  override init(reuseIdentifier: String?) {
    super.init(reuseIdentifier: reuseIdentifier)
    addSubview(titleLabel)

    updateFont()
    updateLayoutConstraints()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    
    updateFont()
    updateLayoutConstraints()
  }
  
  private func updateFont() {
    titleLabel.font = .preferredFont(forTextStyle: .body)
  }
  
  private func updateLayoutConstraints() {
    titleLabel.snp.remakeConstraints {
      $0.left.right.greaterThanOrEqualTo(self).inset(UX.horizontalPadding)
      $0.top.bottom.greaterThanOrEqualTo(self).inset(UX.verticalPadding)
      $0.centerX.centerY.equalToSuperview()
    }
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
