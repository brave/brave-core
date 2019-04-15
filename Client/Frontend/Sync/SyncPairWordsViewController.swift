/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared
import Data

private let log = Logger.browserLogger

class SyncPairWordsViewController: SyncViewController {
    
    var syncHandler: (([Int]?) -> Void)?
    var scrollView: UIScrollView!
    var containerView: UIView!
    var codewordsView: SyncCodewordsView!
    
    lazy var wordCountLabel: UILabel = {
        let label = UILabel()
        label.font = UIFont.systemFont(ofSize: 13, weight: UIFont.Weight.regular)
        label.textColor = BraveUX.GreyE
        label.text = String(format: Strings.WordCount, 0)
        return label
    }()
    
    lazy var copyPasteButton: UIButton = {
        let button = UIButton()
        button.setImage(UIImage(named: "copy_paste"), for: .normal)
        button.addTarget(self, action: #selector(SEL_paste), for: .touchUpInside)
        return button
    }()
    
    let useCameraButton = UIButton().then {
        $0.setTitle(Strings.SyncSwitchBackToCameraButton, for: .normal)
        $0.addTarget(self, action: #selector(useCameraButtonTapped), for: .touchDown)
        $0.setTitleColor(BraveUX.GreyH, for: .normal)
        $0.titleLabel?.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.regular)
    }
    
    var loadingView: UIView!
    let loadingSpinner = UIActivityIndicatorView(style: .whiteLarge)
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.SyncAddDeviceWords
        
        scrollView = UIScrollView()
        scrollView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(scrollView)
        
        containerView = UIView()
        containerView.translatesAutoresizingMaskIntoConstraints = false
        containerView.backgroundColor = UIColor.white
        containerView.layer.shadowColor = UIColor(rgb: 0xC8C7CC).cgColor
        containerView.layer.shadowRadius = 0
        containerView.layer.shadowOpacity = 1.0
        containerView.layer.shadowOffset = CGSize(width: 0, height: 0.5)
        scrollView.addSubview(containerView)
        
        codewordsView = SyncCodewordsView(data: [])
        codewordsView.wordCountChangeCallback = { (count) in
            self.wordCountLabel.text = String(format: Strings.WordCount, count)
        }
        containerView.addSubview(codewordsView)
        containerView.addSubview(wordCountLabel)
        containerView.addSubview(copyPasteButton)
        
        loadingSpinner.startAnimating()
        
        loadingView = UIView()
        loadingView.backgroundColor = UIColor(white: 0.5, alpha: 0.5)
        loadingView.isHidden = true
        loadingView.addSubview(loadingSpinner)
        
        view.addSubview(loadingView)
        view.addSubview(useCameraButton)
        
        navigationItem.rightBarButtonItem = UIBarButtonItem(title: Strings.Confirm, style: .done, target: self, action: #selector(SEL_done))
        
        edgesForExtendedLayout = UIRectEdge()
        
        scrollView.snp.makeConstraints { (make) in
            make.edges.equalTo(self.view)
        }
        
        containerView.snp.makeConstraints { (make) in
            // Making these edges based off of the scrollview removes selectability on codewords.
            //  This currently works for all layouts and enables interaction, so using `view` instead.
            make.top.equalTo(self.view)
            make.left.equalTo(self.view)
            make.right.equalTo(self.view)
            make.height.equalTo(295)
            make.width.equalTo(self.view)
        }
        
        codewordsView.snp.makeConstraints { (make) in
            make.edges.equalTo(self.containerView).inset(UIEdgeInsets(top: 0, left: 0, bottom: 45, right: 0))
        }
        
        wordCountLabel.snp.makeConstraints { (make) in
            make.top.equalTo(codewordsView.snp.bottom)
            make.left.equalTo(codewordsView).inset(24)
        }
        
        copyPasteButton.snp.makeConstraints { (make) in
            make.size.equalTo(45)
            make.right.equalTo(containerView).inset(15)
            make.bottom.equalTo(containerView).inset(15)
        }
        
        loadingView.snp.makeConstraints { (make) in
            make.edges.equalTo(loadingView.superview!)
        }
        
        loadingSpinner.snp.makeConstraints { (make) in
            make.center.equalTo(loadingView)
        }
        
        useCameraButton.snp.makeConstraints { make in
            make.top.equalTo(containerView.snp.bottom).offset(16)
            make.left.equalTo(self.view)
            make.right.equalTo(self.view)
            make.centerX.equalTo(self.view)
        }
    }
    
    @objc func useCameraButtonTapped() {
        self.navigationController?.popViewController(animated: true)
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        codewordsView.becomeFirstResponder()
    }
    
    @objc func SEL_paste() {
        if let contents = UIPasteboard.general.string, !contents.isEmpty {
            // remove linebreaks and whitespace, split into codewords.
            codewordsView.setCodewords(data: contents.separatedBy(" "))

            UIPasteboard.general.clear()
        }
    }
    
    @objc func SEL_done() {
        doIfConnected {
            checkCodes()
        }
    }
    
    private func checkCodes() {
        log.debug("check codes")
        
        func alert(title: String? = nil, message: String? = nil) {
            if Sync.shared.isInSyncGroup {
                // No alert
                return
            }
            let title = title ?? Strings.UnableToConnectTitle
            let message = message ?? Strings.UnableToConnectDescription
            let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
            self.present(alert, animated: true, completion: nil)
        }
        
        let codes = self.codewordsView.codeWords()

        // Maybe temporary validation, sync server has issues without this validation
        if codes.count < Sync.SeedByteLength / 2 {
            alert(title: Strings.NotEnoughWordsTitle, message: Strings.NotEnoughWordsDescription)
            return
        }
        
        self.view.endEditing(true)
        enableNavigationPrevention()
        
        // forced timeout
        DispatchQueue.main.asyncAfter(deadline: DispatchTime.now() + Double(Int64(25.0) * Int64(NSEC_PER_SEC)) / Double(NSEC_PER_SEC), execute: {
            self.disableNavigationPrevention()
            alert()
        })
        
        let result = SyncCrypto().bytes(fromPassphrase: codes)
        switch result {
        case .success(let bytes):
            syncHandler?(bytes)
        case .failure(let error):
            alert(message: "\(error)")
            disableNavigationPrevention()
        }
    }
}

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
