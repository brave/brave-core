/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared
import Data

enum DeviceType {
    case mobile
    case computer
}

class SyncAddDeviceViewController: SyncViewController {
    var doneHandler: (() -> Void)?

    lazy var stackView: UIStackView = {
        let stack = UIStackView()
        stack.axis = .vertical
        stack.distribution = .equalSpacing
        stack.spacing = 4
        return stack
    }()
    
    lazy var codewordsView: UILabel = {
        let label = UILabel()
        label.font = UIFont.systemFont(ofSize: 18.0, weight: UIFont.Weight.medium)
        label.lineBreakMode = NSLineBreakMode.byWordWrapping
        label.numberOfLines = 0
        return label
    }()
    
    lazy var copyPasteButton: UIButton = {
        let button = UIButton()
        button.setTitle(Strings.CopyToClipboard, for: .normal)
        button.addTarget(self, action: #selector(SEL_copy), for: .touchUpInside)
        button.setTitleColor(BraveUX.BraveOrange, for: .normal)
        button.contentEdgeInsets = UIEdgeInsets(top: 8, left: 8, bottom: 8, right: 8)
        button.isHidden = true
        return button
    }()
    var controlContainerView: UIView!
    var containerView: UIView!
    var qrCodeView: SyncQRCodeView?
    var modeControl: UISegmentedControl!
    var titleLabel: UILabel!
    var descriptionLabel: UILabel!
    var doneButton: RoundInterfaceButton!
    var enterWordsButton: RoundInterfaceButton!
    var pageTitle: String = Strings.Sync
    var deviceType: DeviceType = .mobile
    var didCopy = false {
        didSet {
            if didCopy {
                copyPasteButton.setTitle(Strings.CopiedToClipboard, for: .normal)
            } else {
                copyPasteButton.setTitle(Strings.CopyToClipboard, for: .normal)
            }
        }
    }

    private var clipboardClearTimer: Timer?
    
    // Pass in doneHandler here
    convenience init(title: String, type: DeviceType) {
        self.init()
        pageTitle = title
        deviceType = type
    }
    
    override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
        super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = deviceType == .computer ? Strings.SyncAddComputerTitle : Strings.SyncAddTabletOrPhoneTitle

        view.addSubview(stackView)
        stackView.snp.makeConstraints { make in
            make.top.equalTo(self.view.safeArea.top)
            make.left.right.equalTo(self.view)
            make.bottom.equalTo(self.view.safeArea.bottom).inset(24)
        }
        
        controlContainerView = UIView()
        controlContainerView.translatesAutoresizingMaskIntoConstraints = false

        containerView = UIView()
        containerView.translatesAutoresizingMaskIntoConstraints = false
        containerView.layer.cornerRadius = 8
        containerView.layer.masksToBounds = true

        guard let syncSeed = Sync.shared.syncSeedArray else {
            showInitializationError()
            return
        }
        
        let syncCrypto = SyncCrypto()

        let qrSyncSeed = syncCrypto.joinBytes(fromCombinedBytes: syncSeed)
        if qrSyncSeed.isEmpty {
            showInitializationError()
            return
        }
        
        let words = syncCrypto.passphrase(fromBytes: syncSeed)
        
        switch words {
        case .success(let passphrase):
            qrCodeView = SyncQRCodeView(data: qrSyncSeed)
            containerView.addSubview(qrCodeView!)
            qrCodeView?.snp.makeConstraints { make in
                make.top.bottom.equalTo(0).inset(22)
                make.centerX.equalTo(self.containerView)
                make.size.equalTo(BarcodeSize)
            }
            
            self.codewordsView.text = passphrase.joined(separator: " ")
        case .failure:
            showInitializationError()
            return
        }
        
        self.setupVisuals()
    }
    
    private func showInitializationError() {
        present(SyncAlerts.initializationError, animated: true) {
            Sync.shared.leaveSyncGroup()
        }
    }
    
    func setupVisuals() {
        modeControl = UISegmentedControl(items: [Strings.QRCode, Strings.CodeWords])
        modeControl.translatesAutoresizingMaskIntoConstraints = false
        modeControl.tintColor = BraveUX.BraveOrange
        modeControl.selectedSegmentIndex = 0
        modeControl.addTarget(self, action: #selector(SEL_changeMode), for: .valueChanged)
        modeControl.isHidden = deviceType == .computer
        controlContainerView.addSubview(modeControl)
        stackView.addArrangedSubview(controlContainerView)
        
        let titleDescriptionStackView = UIStackView()
        titleDescriptionStackView.axis = .vertical
        titleDescriptionStackView.spacing = 2
        titleDescriptionStackView.alignment = .center
        
        titleLabel = UILabel()
        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        titleLabel.font = UIFont.systemFont(ofSize: 20, weight: UIFont.Weight.semibold)
        titleDescriptionStackView.addArrangedSubview(titleLabel)

        descriptionLabel = UILabel()
        descriptionLabel.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.regular)
        descriptionLabel.numberOfLines = 0
        descriptionLabel.lineBreakMode = .byTruncatingTail
        descriptionLabel.textAlignment = .center
        descriptionLabel.adjustsFontSizeToFitWidth = true
        descriptionLabel.minimumScaleFactor = 0.5
        titleDescriptionStackView.addArrangedSubview(descriptionLabel)

        let textStackView = UIStackView(arrangedSubviews: [UIView.spacer(.horizontal, amount: 32),
                                                           titleDescriptionStackView,
                                                           UIView.spacer(.horizontal, amount: 32)])
        textStackView.setContentCompressionResistancePriority(UILayoutPriority(rawValue: 100), for: .vertical)

        stackView.addArrangedSubview(textStackView)
        
        codewordsView.isHidden = true
        containerView.addSubview(codewordsView)
        
        stackView.addArrangedSubview(containerView)
        
        let copyPasteStackView = UIStackView()
        copyPasteStackView.axis = .vertical
        copyPasteStackView.spacing = 1
        copyPasteStackView.addArrangedSubview(copyPasteButton)
        stackView.addArrangedSubview(copyPasteStackView)

        let doneEnterWordsStackView = UIStackView()
        doneEnterWordsStackView.axis = .vertical
        doneEnterWordsStackView.spacing = 4

        doneButton = RoundInterfaceButton(type: .roundedRect)
        doneButton.translatesAutoresizingMaskIntoConstraints = false
        doneButton.setTitle(Strings.Done, for: .normal)
        doneButton.titleLabel?.font = UIFont.systemFont(ofSize: 17, weight: UIFont.Weight.bold)
        doneButton.appearanceTextColor = .white
        doneButton.backgroundColor = BraveUX.BraveOrange
        doneButton.addTarget(self, action: #selector(SEL_done), for: .touchUpInside)

        doneEnterWordsStackView.addArrangedSubview(doneButton)

        enterWordsButton = RoundInterfaceButton(type: .roundedRect)
        enterWordsButton.translatesAutoresizingMaskIntoConstraints = false
        enterWordsButton.setTitle(Strings.ShowCodeWords, for: .normal)
        enterWordsButton.titleLabel?.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.semibold)
        enterWordsButton.setTitleColor(BraveUX.GreyH, for: .normal)
        enterWordsButton.addTarget(self, action: #selector(SEL_showCodewords), for: .touchUpInside)

        let buttonsStackView = UIStackView(arrangedSubviews: [UIView.spacer(.horizontal, amount: 16),
                                                              doneEnterWordsStackView,
                                                              UIView.spacer(.horizontal, amount: 16)])
        buttonsStackView.setContentCompressionResistancePriority(UILayoutPriority(rawValue: 1000), for: .vertical)

        stackView.addArrangedSubview(buttonsStackView)
        
        controlContainerView.snp.makeConstraints { (make) in
            make.top.equalTo(0)
            make.left.right.equalTo(self.view)
            make.height.greaterThanOrEqualTo(44)
        }

        modeControl.snp.makeConstraints { (make) in
            make.top.equalTo(0).offset(10)
            make.left.right.equalTo(self.controlContainerView).inset(8)
        }
        
        containerView.snp.makeConstraints { (make) in
            make.left.right.equalTo(self.view).inset(22)
        }

        codewordsView.snp.makeConstraints { (make) in
            make.top.bottom.equalTo(self.containerView).inset(8)
            make.left.right.equalTo(self.containerView).inset(22)
        }

        doneButton.snp.makeConstraints { (make) in
            make.height.equalTo(40)
        }

        enterWordsButton.snp.makeConstraints { (make) in
            make.height.equalTo(20)
        }

        if deviceType == .computer {
            SEL_showCodewords()
        }
        
        updateLabels()
    }
    
    func updateLabels() {
        let isFirstIndex = modeControl.selectedSegmentIndex == 0
        
        titleLabel.text = isFirstIndex ? Strings.SyncAddDeviceScan : Strings.SyncAddDeviceWords
        
        if isFirstIndex {
            descriptionLabel.text = Strings.SyncAddDeviceScanDescription
        } else {
            // The button name should be the same as in codewords instructions.
            let buttonName = Strings.ScanSyncCode
            let addDeviceWords = String(format: Strings.SyncAddDeviceWordsDescription, buttonName)
            let fontSize = descriptionLabel.font.pointSize
            
            // For codewords instructions copy, we want to bold the button name which needs to be tapped.
            descriptionLabel.attributedText =
                addDeviceWords.makePartiallyBoldAttributedString(stringToBold: buttonName, boldTextSize: fontSize)
        }
    }
    
    @objc func SEL_showCodewords() {
        modeControl.selectedSegmentIndex = 1
        enterWordsButton.isHidden = true
        SEL_changeMode()
    }
    
    @objc func SEL_copy() {
        UIPasteboard.general.string = self.codewordsView.text
        didCopy = true

        clipboardClearTimer?.invalidate()
        clipboardClearTimer = UIPasteboard.general.clear(after: 30)
    }
    
    @objc func SEL_changeMode() {
        let isFirstIndex = modeControl.selectedSegmentIndex == 0
        
        qrCodeView?.isHidden = !isFirstIndex
        codewordsView.isHidden = isFirstIndex
        copyPasteButton.isHidden = isFirstIndex

        updateLabels()
    }
    
    @objc func SEL_done() {
        doneHandler?()
    }
}

