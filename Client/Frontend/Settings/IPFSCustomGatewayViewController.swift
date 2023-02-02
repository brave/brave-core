// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Static
import Shared
import WebKit
import BraveCore
import SnapKit
import Fuzi
import Storage
import Data
import os.log

class IPFSCustomGatewayViewController: UIViewController {
  
  private static let ipfsTestPath = "ipfs"
  private static let ipfsTestCID = "bafkqae2xmvwgg33nmuqhi3zajfiemuzahiwss"
  private static let ipfsTestContent = "Welcome to IPFS :-)"
  
  // MARK: AddButtonType
  
  private enum AddButtonType {
    case enabled
    case disabled
    case loading
  }
  
  // MARK: Properties
  
  private var ipfsAPI: IpfsAPI
  
  private var urlText: String?
  
  private lazy var spinnerView = UIActivityIndicatorView(style: .medium).then {
    $0.hidesWhenStopped = true
  }
  
  private var tableView = UITableView(frame: .zero, style: .grouped)
  
  // MARK: Lifecycle
  
  init(ipfsAPI: IpfsAPI) {
    self.ipfsAPI = ipfsAPI
    
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    title = Strings.BraveIPFS.nftGatewaySetting
    
    setup()
    doLayout()
    changeAddButton(for: .disabled)
  }
  
  // MARK: Internal
  
  private func setup() {
    tableView.do {
      $0.register(URLInputTableViewCell.self)
      $0.dataSource = self
      $0.delegate = self
    }
  }
  
  private func doLayout() {
    view.addSubview(tableView)
    
    tableView.snp.makeConstraints { make in
      make.edges.equalTo(self.view)
    }
  }
  
  private func changeAddButton(for type: AddButtonType) {
    ensureMainThread {
      switch type {
      case .enabled:
        self.navigationItem.rightBarButtonItem = UIBarButtonItem(
          title: Strings.BraveIPFS.setGatewayButtonTitle, style: .done, target: self, action: #selector(self.validateGateway))
      case .disabled:
        self.navigationItem.rightBarButtonItem = UIBarButtonItem(
          title: Strings.BraveIPFS.setGatewayButtonTitle, style: .done, target: self, action: #selector(self.validateGateway))
        self.navigationItem.rightBarButtonItem?.isEnabled = false
      case .loading:
        self.navigationItem.rightBarButtonItem = UIBarButtonItem(customView: self.spinnerView)
        self.spinnerView.startAnimating()
      }
    }
  }
  
  private func showWrongGatewayUrlAlert() {
    let alert = UIAlertController(
      title: Strings.BraveIPFS.wrongGatewayAlertTitle,
      message: Strings.BraveIPFS.wrongGatewayAlertDescription,
      preferredStyle: .alert
    )
    
    let okayOption = UIAlertAction(
      title: Strings.OKString,
      style: .default,
      handler: nil
    )
    
    alert.addAction(okayOption)
    present(alert, animated: true) {
      self.changeAddButton(for: .disabled)
    }
  }
  
  @objc func validateGateway() {
    guard let urlTextValue = urlText else {
      showWrongGatewayUrlAlert()
      return
    }
    if let url = URL(string: urlTextValue) {
      if !url.isWebPage() {
        showWrongGatewayUrlAlert()
        return
      }
      var checkUrl = url
      checkUrl.append(pathComponents: IPFSCustomGatewayViewController.ipfsTestPath, IPFSCustomGatewayViewController.ipfsTestCID)
      
      changeAddButton(for: .loading)
      view.endEditing(true)
      
      NetworkManager().downloadResource(with: checkUrl) { [weak self] response in
        guard let self = self else { return }
        switch response {
        case .success(let response):
          if (String(data: response.data, encoding: .utf8) == IPFSCustomGatewayViewController.ipfsTestContent) {
            self.setGateway(url)
          } else {
            self.showWrongGatewayUrlAlert()
          }
        case .failure(_):
          self.showWrongGatewayUrlAlert()
        }
        self.changeAddButton(for: .disabled)
      }
    } else {
      showWrongGatewayUrlAlert()
      self.changeAddButton(for: .disabled)
    }
  }
  
  @objc func setGateway(_ url: URL) {
    self.ipfsAPI.nftIpfsGateway = url
  }
  
  @objc func cancel() {
    navigationController?.popViewController(animated: true)
  }
}

fileprivate class URLInputTableViewCell: UITableViewCell, TableViewReusable {
  
  // MARK: UX
  
  struct UX {
    static let textViewHeight: CGFloat = 88
    static let textViewInset: CGFloat = 16
  }
  
  // MARK: Properties
  
  var textView = UITextView(frame: .zero)

  weak var delegate: UITextViewDelegate? {
    didSet {
      textView.delegate = delegate
    }
  }
  // MARK: Lifecycle
  
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)

    setup()
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  // MARK: Internal
  
  private func setup() {
    textView = UITextView(frame: CGRect(x: 0, y: 0, width: contentView.frame.width, height: contentView.frame.height)).then {
      $0.backgroundColor = .clear
      $0.font = UIFont.preferredFont(forTextStyle: .body)
      $0.autocapitalizationType = .none
      $0.autocorrectionType = .no
      $0.spellCheckingType = .no
      $0.keyboardType = .URL
      $0.textColor = .braveLabel
    }
    
    contentView.addSubview(textView)
    
    textView.snp.makeConstraints({ make in
      make.leading.trailing.equalToSuperview().inset(UX.textViewInset)
      make.bottom.top.equalToSuperview()
      make.height.equalTo(UX.textViewHeight)
    })
  }
}


// MARK: - UITableViewDelegate UITableViewDataSource

extension IPFSCustomGatewayViewController: UITableViewDelegate, UITableViewDataSource {
  
  func numberOfSections(in tableView: UITableView) -> Int {
    return 1
  }
  
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return 1
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(for: indexPath) as URLInputTableViewCell
    cell.do {
      $0.delegate = self
      $0.textView.text = ipfsAPI.nftIpfsGateway?.absoluteString
      $0.selectionStyle = .none
    }
    return cell
  }
  
  func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
    return Strings.BraveIPFS.nftGatewayLongDescription
  }
}

// MARK: - UITextViewDelegate

extension IPFSCustomGatewayViewController: UITextViewDelegate {
  
  func textView(_ textView: UITextView, shouldChangeTextIn range: NSRange, replacementText text: String) -> Bool {
    guard text.rangeOfCharacter(from: .newlines) == nil else {
      textView.resignFirstResponder()
      return false
    }
    
    let textLengthInRange = textView.text.count + (text.count - range.length)
    
    // The default text "https://" cant ne deleted or changed so nothing without a secure scheme can be added
    return textLengthInRange >= 8
  }
  
  func textViewDidChange(_ textView: UITextView) {
    changeAddButton(for: .disabled)
    
    // The withSecureUrlScheme is used in order to force user to use secure url scheme
    // Instead of checking paste-board with every character entry, the textView text is analyzed
    // and according to what prefix copied or entered, text is altered to start with https://
    // this logic block repeating https:// and http:// schemes
    let textEntered = textView.text.withSecureUrlScheme
    
    textView.text = textEntered
    urlText = textEntered
    
    let url = URL(string: textEntered)
    if url != nil && url!.isWebPage() {
      changeAddButton(for: .enabled)
    }
  }
  
  func textViewDidEndEditing(_ textView: UITextView) {
    urlText = textView.text
  }
}
