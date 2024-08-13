// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Data
import Shared
import UIKit
import os.log

class SyncPairWordsViewController: SyncViewController {

  private let scrollView = UIScrollView().then {
    $0.translatesAutoresizingMaskIntoConstraints = false
  }
  private let containerView = UIView().then {
    $0.translatesAutoresizingMaskIntoConstraints = false
    $0.backgroundColor = .braveBackground
    $0.layer.shadowColor = UIColor.braveSeparator.cgColor
    $0.layer.shadowRadius = 0
    $0.layer.shadowOpacity = 1.0
    $0.layer.shadowOffset = CGSize(width: 0, height: 0.5)
  }

  private lazy var wordCountLabel: UILabel = {
    let label = UILabel()
    label.font = UIFont.systemFont(ofSize: 13, weight: UIFont.Weight.regular)
    label.textColor = .braveLabel
    label.text = String(format: Strings.Sync.wordCount, 0)
    return label
  }()

  private lazy var copyPasteButton: UIButton = {
    let button = UIButton()
    button.setImage(
      UIImage(named: "copy_paste", in: .module, compatibleWith: nil)?.template,
      for: .normal
    )
    button.addTarget(self, action: #selector(pasteButtonPressed), for: .touchUpInside)
    button.tintColor = .braveLabel
    return button
  }()

  private lazy var useCameraButton = UIButton().then {
    $0.setTitle(Strings.Sync.switchBackToCameraButton, for: .normal)
    $0.addTarget(self, action: #selector(useCameraButtonTapped), for: .touchDown)
    $0.setTitleColor(.braveLabel, for: .normal)
    $0.titleLabel?.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.regular)
  }

  private let loadingView = UIView().then {
    $0.backgroundColor = UIColor(white: 0.5, alpha: 0.5)
    $0.isHidden = true
  }
  private let loadingSpinner = UIActivityIndicatorView(style: .large)

  private var codewordsView = SyncCodewordsView(data: [])
  private let syncAPI: BraveSyncAPI
  weak var delegate: SyncPairControllerDelegate?

  // MARK: Lifecycle

  init(syncAPI: BraveSyncAPI) {
    self.syncAPI = syncAPI
    super.init()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    containerView.layer.shadowColor = UIColor.braveSeparator.cgColor
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.Sync.addDeviceWordsTitle

    doLayout()

    codewordsView.wordCountChangeCallback = { [weak self] count in
      self?.wordCountLabel.text = String(format: Strings.Sync.wordCount, count)
    }

    loadingSpinner.startAnimating()

    navigationItem.rightBarButtonItem = UIBarButtonItem(
      title: Strings.confirm,
      style: .done,
      target: self,
      action: #selector(doneButtonPressed)
    )

    edgesForExtendedLayout = UIRectEdge()
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    codewordsView.becomeFirstResponder()
  }

  private func doLayout() {
    view.addSubview(scrollView)

    scrollView.addSubview(containerView)

    containerView.addSubview(codewordsView)
    containerView.addSubview(wordCountLabel)
    containerView.addSubview(copyPasteButton)

    loadingView.addSubview(loadingSpinner)

    view.addSubview(loadingView)
    view.addSubview(useCameraButton)

    scrollView.snp.makeConstraints {
      $0.edges.equalTo(view)
    }

    containerView.snp.makeConstraints {
      // Making these edges based off of the scrollview removes selectability on codewords.
      //  This currently works for all layouts and enables interaction, so using `view` instead.
      $0.top.equalTo(view)
      $0.left.equalTo(view)
      $0.right.equalTo(view)
      $0.height.equalTo(295)
      $0.width.equalTo(view)
    }

    codewordsView.snp.makeConstraints {
      $0.edges.equalTo(containerView).inset(UIEdgeInsets(top: 0, left: 0, bottom: 45, right: 0))
    }

    wordCountLabel.snp.makeConstraints {
      $0.top.equalTo(codewordsView.snp.bottom)
      $0.left.equalTo(codewordsView).inset(24)
    }

    copyPasteButton.snp.makeConstraints {
      $0.size.equalTo(45)
      $0.right.equalTo(containerView).inset(15)
      $0.bottom.equalTo(containerView).inset(15)
    }

    loadingView.snp.makeConstraints {
      $0.edges.equalTo(loadingView.superview!)
    }

    loadingSpinner.snp.makeConstraints {
      $0.center.equalTo(loadingView)
    }

    useCameraButton.snp.makeConstraints {
      $0.top.equalTo(containerView.snp.bottom).offset(16)
      $0.left.equalTo(view)
      $0.right.equalTo(view)
      $0.centerX.equalTo(view)
    }
  }

  // MARK: Actions

  @objc func useCameraButtonTapped() {
    navigationController?.popViewController(animated: true)
  }

  @objc func pasteButtonPressed() {
    if let contents = UIPasteboard.general.string, !contents.isEmpty {
      // remove linebreaks and whitespace, split into codewords.
      codewordsView.setCodewords(data: contents.separatedBy(" "))

      UIPasteboard.general.clearPasteboard()
    }
  }

  @objc func doneButtonPressed() {
    doIfConnected {
      self.checkCodes()
    }
  }

  // MARK: Internal

  /// Check Codes is performing code word validation from core side to determine
  /// the sync code words entered are valid and not expired
  /// This is pre validation before  sync chain not deleted confirmation
  private func checkCodes() {
    let codes = self.codewordsView.codeWords().joined(separator: " ")
    let syncCodeValidation = syncAPI.getWordsValidationResult(codes)
    if syncCodeValidation == .wrongWordsNumber {
      showAlert(
        title: Strings.Sync.notEnoughWordsTitle,
        message: Strings.Sync.notEnoughWordsDescription
      )
      return
    }

    view.endEditing(true)
    enableNavigationPrevention()

    timeoutSyncSetup()

    if syncCodeValidation == .valid {
      let words = syncAPI.getWordsFromTimeLimitedWords(codes)
      delegate?.syncOnWordsEntered(self, codeWords: words, isCodeScanned: false)
    } else {
      showAlert(message: syncCodeValidation.errorDescription)
      disableNavigationPrevention()
    }
  }

  /// Time out and disable loading for sync setup
  private func timeoutSyncSetup() {
    // Time out after 25 sec
    DispatchQueue.main.asyncAfter(deadline: .now() + 25.0) {
      self.disableNavigationPrevention()
      self.showAlert()
    }
  }

  private func showAlert(title: String? = nil, message: String? = nil) {
    if syncAPI.isInSyncGroup {
      return
    }

    let title = title ?? Strings.Sync.unableToConnectTitle
    let message = message ?? Strings.Sync.unableToConnectDescription

    let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
    present(alert, animated: true, completion: nil)
  }
}

// MARK: NavigationPrevention

extension SyncPairWordsViewController: NavigationPrevention {
  func enableNavigationPrevention() {
    loadingView.isHidden = false
    navigationItem.rightBarButtonItem?.isEnabled = false
    navigationItem.hidesBackButton = true
  }

  func disableNavigationPrevention() {
    loadingView.isHidden = true
    navigationItem.rightBarButtonItem?.isEnabled = true
    navigationItem.hidesBackButton = false

  }
}
