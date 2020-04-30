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
    
    private let barcodeSize: CGFloat = 200.0

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
        button.setTitle(Strings.copyToClipboard, for: .normal)
        button.addTarget(self, action: #selector(SEL_copy), for: .touchUpInside)
        button.setTitleColor(BraveUX.braveOrange, for: .normal)
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
    var pageTitle: String = Strings.sync
    var deviceType: DeviceType = .mobile
    var didCopy = false {
        didSet {
            if didCopy {
                copyPasteButton.setTitle(Strings.copiedToClipboard, for: .normal)
            } else {
                copyPasteButton.setTitle(Strings.copyToClipboard, for: .normal)
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
        
        title = deviceType == .computer ? Strings.syncAddComputerTitle : Strings.syncAddTabletOrPhoneTitle

        view.addSubview(stackView)
        stackView.snp.makeConstraints { make in
            make.top.equalTo(self.view.safeArea.top).inset(10)
            make.left.right.equalTo(self.view).inset(16)
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
                make.size.equalTo(barcodeSize)
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
    
    private func setupVisuals() {
        modeControl = UISegmentedControl(items: [Strings.QRCode, Strings.codeWords])
        modeControl.translatesAutoresizingMaskIntoConstraints = false
        modeControl.selectedSegmentIndex = 0
        modeControl.addTarget(self, action: #selector(SEL_changeMode), for: .valueChanged)
        modeControl.isHidden = deviceType == .computer
        modeControl.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
        
        if #available(iOS 13.0, *) {
            modeControl.selectedSegmentTintColor = BraveUX.braveOrange
        } else {
            modeControl.tintColor = BraveUX.braveOrange
        }
        stackView.addArrangedSubview(modeControl)
        
        let titleDescriptionStackView = UIStackView()
        titleDescriptionStackView.axis = .vertical
        titleDescriptionStackView.spacing = 2
        titleDescriptionStackView.alignment = .center
        
        titleLabel = UILabel()
        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        titleLabel.font = UIFont.systemFont(ofSize: 20, weight: UIFont.Weight.semibold)
        titleLabel.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
        titleDescriptionStackView.addArrangedSubview(titleLabel)

        descriptionLabel = UILabel()
        descriptionLabel.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.regular)
        descriptionLabel.numberOfLines = 0
        descriptionLabel.lineBreakMode = .byTruncatingTail
        descriptionLabel.textAlignment = .center
        descriptionLabel.adjustsFontSizeToFitWidth = true
        descriptionLabel.minimumScaleFactor = 0.5
        descriptionLabel.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
        titleDescriptionStackView.addArrangedSubview(descriptionLabel)

        stackView.addArrangedSubview(titleDescriptionStackView)
        
        codewordsView.isHidden = true
        containerView.addSubview(codewordsView)
        stackView.addArrangedSubview(containerView)
        
        let doneEnterWordsStackView = UIStackView()
        doneEnterWordsStackView.axis = .vertical
        doneEnterWordsStackView.spacing = 4
        doneEnterWordsStackView.distribution = .fillEqually
        
        doneEnterWordsStackView.addArrangedSubview(copyPasteButton)

        doneButton = RoundInterfaceButton(type: .roundedRect)
        doneButton.translatesAutoresizingMaskIntoConstraints = false
        doneButton.setTitle(Strings.done, for: .normal)
        doneButton.titleLabel?.font = UIFont.systemFont(ofSize: 17, weight: UIFont.Weight.bold)
        doneButton.appearanceTextColor = .white
        doneButton.backgroundColor = BraveUX.braveOrange
        doneButton.addTarget(self, action: #selector(SEL_done), for: .touchUpInside)

        doneEnterWordsStackView.addArrangedSubview(doneButton)

        enterWordsButton = RoundInterfaceButton(type: .roundedRect)
        enterWordsButton.translatesAutoresizingMaskIntoConstraints = false
        enterWordsButton.setTitle(Strings.showCodeWords, for: .normal)
        enterWordsButton.titleLabel?.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.semibold)
        enterWordsButton.setTitleColor(BraveUX.greyH, for: .normal)
        enterWordsButton.addTarget(self, action: #selector(SEL_showCodewords), for: .touchUpInside)

        doneEnterWordsStackView.setContentCompressionResistancePriority(.required, for: .vertical)

        stackView.addArrangedSubview(doneEnterWordsStackView)

        codewordsView.snp.makeConstraints {
            $0.edges.equalToSuperview()
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
    }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        updateLabels()
    }
    
    private func updateLabels() {
        let isFirstIndex = modeControl.selectedSegmentIndex == 0
        
        titleLabel.text = isFirstIndex ? Strings.syncAddDeviceScan : Strings.syncAddDeviceWords
        
        if isFirstIndex {
            let description = Strings.syncAddDeviceScanDescription
            let attributedDescription = NSMutableAttributedString(string: description)
            
            if let lastSentenceRange = lastSentenceRange(text: description) {
                attributedDescription.addAttribute(.foregroundColor, value: BraveUX.red, range: lastSentenceRange)
            }
            
            descriptionLabel.attributedText = attributedDescription
        } else {
            // The button name should be the same as in codewords instructions.
            let buttonName = Strings.scanSyncCode
            let addDeviceWords = String(format: Strings.syncAddDeviceWordsDescription, buttonName)
            let description = NSMutableAttributedString(string: addDeviceWords)
            let fontSize = descriptionLabel.font.pointSize
            
            let boldRange = (addDeviceWords as NSString).range(of: buttonName)
            description.addAttribute(.font, value: UIFont.boldSystemFont(ofSize: fontSize), range: boldRange)
            
            if let lastSentenceRange = lastSentenceRange(text: addDeviceWords) {
                description.addAttribute(.foregroundColor, value: BraveUX.red, range: lastSentenceRange)
            }
            
            descriptionLabel.attributedText = description
        }
    }
    
    private func lastSentenceRange(text: String) -> NSRange? {
        guard let lastSentence = text.split(separator: "\n").last else { return nil }
        return (text as NSString).range(of: String(lastSentence))
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

