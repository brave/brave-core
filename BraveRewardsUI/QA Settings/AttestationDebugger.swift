// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveRewards
import MobileCoreServices

class QAAttestationDebugViewController: UIViewController, UIPickerViewDelegate, UIPickerViewDataSource, UITextViewDelegate {
  
  init(paymentId: String) {
    self.paymentId = paymentId
    
    super.init(nibName: nil, bundle: nil)
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  // MARK: - Views
  private let pickerView = UIPickerView().then {
    $0.isHidden = true
    $0.backgroundColor = .lightGray
  }
  
  private let requestLabel = UILabel().then {
    $0.text = "Request: "
    $0.textColor = .blue
    $0.font = UIFont.boldSystemFont(ofSize: 14.0)
  }
  
  private let responseLabel = UILabel().then {
    $0.text = "Response: "
    $0.textColor = .blue
    $0.font = UIFont.boldSystemFont(ofSize: 14.0)
  }
  
  private let payloadLabel = UILabel().then {
    $0.text = "Device Token: "
    $0.textColor = .blue
    $0.font = UIFont.boldSystemFont(ofSize: 14.0)
  }
  
  private let requestView = UITextView().then {
    $0.isEditable = false
    $0.backgroundColor = .white
    $0.layer.borderColor = UIColor.gray.cgColor
    $0.layer.borderWidth = 1.0
    $0.layer.cornerRadius = 5.0
  }
  
  private let responseView = UITextView().then {
    $0.isEditable = false
    $0.backgroundColor = .white
    $0.layer.borderColor = UIColor.gray.cgColor
    $0.layer.borderWidth = 1.0
    $0.layer.cornerRadius = 5.0
  }
  
  private let payloadView = UITextView().then {
    $0.isEditable = false
    $0.backgroundColor = .white
    $0.layer.borderColor = UIColor.gray.cgColor
    $0.layer.borderWidth = 1.0
    $0.layer.cornerRadius = 5.0
  }
  
  private let changeButton = UIButton().then {
    $0.setTitle("Choose Option", for: .normal)
    $0.backgroundColor = .lightGray
    $0.layer.cornerRadius = 12.0
  }
  
  private let executeButton = UIButton().then {
    $0.setTitle("Execute", for: .normal)
    $0.backgroundColor = .lightGray
    $0.layer.cornerRadius = 12.0
  }
  
  private let loadingView = UIActivityIndicatorView(style: .whiteLarge).then {
    $0.tintColor = .lightGray
    $0.hidesWhenStopped = true
    $0.isHidden = true
  }
  
  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.layoutMargins = UIEdgeInsets(top: 15.0, left: 15.0, bottom: 15.0, right: 15.0)
    $0.isLayoutMarginsRelativeArrangement = true
    $0.spacing = 8.0
  }
  
  private let buttonStackView = UIStackView().then {
    $0.axis = .horizontal
    $0.distribution = .fillEqually
    $0.spacing = 15.0
  }
  
  // MARK: - Variables
  
  private let deviceCheckClient = DeviceCheckClient(environment: BraveLedger.environment)
  private var isEnrolled: Bool = false
  private let paymentId: String
  private var attestationBlob: AttestationBlob?
  private var token: String?
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    title = "Device Check Debugger"
    navigationItem.backBarButtonItem = UIBarButtonItem(title: "Back", style: .plain, target: nil, action: nil)
    navigationItem.rightBarButtonItem = UIBarButtonItem(title: "Copy All", style: .plain, target: self, action: #selector(onCopyAll(_:)))
    
    view.backgroundColor = UIColor(hex: 0xF6F6F6)
    view.addSubview(stackView)
    view.addSubview(pickerView)
    view.addSubview(loadingView)
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(view.safeAreaLayoutGuide)
    }
    
    // Subviews
    stackView.addArrangedSubview(payloadLabel)
    stackView.addArrangedSubview(payloadView)
    
    stackView.addArrangedSubview(requestLabel)
    stackView.addArrangedSubview(requestView)
    
    stackView.addArrangedSubview(responseLabel)
    stackView.addArrangedSubview(responseView)
    stackView.addArrangedSubview(buttonStackView)
    
    stackView.setCustomSpacing(15.0, after: requestView)
    stackView.setCustomSpacing(15.0, after: responseView)
    stackView.setCustomSpacing(15.0, after: payloadView)
    
    buttonStackView.addArrangedSubview(changeButton)
    buttonStackView.addArrangedSubview(executeButton)
    
    requestView.snp.makeConstraints {
      $0.height.equalTo(responseView.snp.height)
    }
    
    responseView.snp.makeConstraints {
      $0.height.equalTo(payloadView.snp.height)
    }
    
    payloadView.snp.makeConstraints {
      $0.height.equalTo(requestView.snp.height)
    }
    
    pickerView.snp.makeConstraints {
      $0.left.right.bottom.equalTo(view.safeAreaLayoutGuide)
    }
    
    buttonStackView.snp.makeConstraints {
      $0.height.equalTo(45.0)
    }
    
    loadingView.snp.makeConstraints {
      $0.center.equalTo(view.center)
    }
    
    // Logic
    pickerView.delegate = self
    pickerView.dataSource = self
    requestView.delegate = self
    responseView.delegate = self
    payloadView.delegate = self
    changeButton.addTarget(self, action: #selector(onChooseButton(_:)), for: .touchUpInside)
    executeButton.addTarget(self, action: #selector(onExecuteButton(_:)), for: .touchUpInside)
    
    view.isUserInteractionEnabled = false
    loadingView.isHidden = false
    loadingView.startAnimating()
    isEnrolled = DeviceCheckClient.isDeviceEnrolled()
    pickerView.reloadAllComponents()
    view.isUserInteractionEnabled = true
    loadingView.stopAnimating()
    
    resetViews()
  }
  
  // MARK: - Actions
  
  @objc
  private func onCopyAll(_ button: UIBarButtonItem) {
    let attributedText = NSMutableAttributedString()
      
    if !payloadView.text.isEmpty {
      attributedText.append(NSAttributedString(string: "Device Token:\n", attributes: [.foregroundColor: UIColor.blue, .font: UIFont.systemFont(ofSize: 14.0)]))
      attributedText.append(NSAttributedString(string: payloadView.text + "\n\n", attributes: [.font: UIFont.systemFont(ofSize: 10.0)]))
    }
    
    if !requestView.text.isEmpty {
      attributedText.append(NSAttributedString(string: "Request:\n", attributes: [.foregroundColor: UIColor.blue, .font: UIFont.systemFont(ofSize: 14.0)]))
      attributedText.append(NSAttributedString(string: requestView.text + "\n\n", attributes: [.font: UIFont.systemFont(ofSize: 10.0)]))
    }
    
    if !responseView.text.isEmpty {
      attributedText.append(NSAttributedString(string: "Response:\n", attributes: [.foregroundColor: UIColor.blue, .font: UIFont.systemFont(ofSize: 14.0)]))
      attributedText.append(NSAttributedString(string: responseView.text + "\n\n", attributes: [.font: UIFont.systemFont(ofSize: 10.0)]))
    }
    
    let pasteboard = UIPasteboard.general
    pasteboard.set(attributedString: attributedText) { error in
      if error != nil {
        pasteboard.string = """
        Device Token:
        \(payloadView.text ?? "{}")
        
        Request:
        \(requestView.text ?? "{}")
        
        Response:
        \(responseView.text ?? "{}")
        """
      }
    }
    
    resetViews()
  }
  
  @objc
  private func onChooseButton(_ button: UIButton) {
    pickerView.isHidden = false
  }
  
  @objc
  private func onExecuteButton(_ button: UIButton) {
    let row = pickerView.selectedRow(inComponent: 0)
    
    if isEnrolled {
      switch row {
      case 0:
        resetViews()
        setLoading(true)
        self.generateToken({ [weak self] _ in
          self?.setLoading(false)
        })
        
      case 1:
        resetViews()
        
        setLoading(true)
        self.getAttestation({ [weak self] blob in
          guard let self = self else { return }
          
          self.attestationBlob = blob
          self.setLoading(false)
        })
        
      case 2:
        resetViews()
        
        guard let attestationBlob = self.attestationBlob else {
          self.requestView.text = "INVALID REQUEST - Get Attestation First!"
          return
        }
        
        self.setAttestation(attestationBlob, { [weak self] in
          self?.setLoading(false)
        })
        
      default: fatalError()
      }
    } else {
      switch row {
      case 0:
        resetViews()
        setLoading(true)
        self.generateToken({ [weak self] _ in
          self?.setLoading(false)
        })
        
      case 1:
        resetViews()
        setLoading(true)
        self.registerDevice({ [weak self] in
          self?.setLoading(false)
        })
        
      default: fatalError()
      }
    }
  }
  
  // MARK: - Picker View Delegate
  
  func numberOfComponents(in pickerView: UIPickerView) -> Int {
    return 1
  }
  
  func pickerView(_ pickerView: UIPickerView, numberOfRowsInComponent component: Int) -> Int {
    return isEnrolled ? 3 : 2
  }
  
  func pickerView(_ pickerView: UIPickerView, titleForRow row: Int, forComponent component: Int) -> String? {
    if isEnrolled {
      switch row {
        case 0: return "Generate Token"
        case 1: return "Get Attestation"
        case 2: return "Set Attestation"
        default: return "N/A"
      }
    }
    
    switch row {
      case 0: return "Generate Token"
      case 1: return "Enroll Device"
      default: return "N/A"
    }
  }
  
  func pickerView(_ pickerView: UIPickerView, didSelectRow row: Int, inComponent component: Int) {
    pickerView.isHidden = true
  }
  
  // MARK: - TextViewDelegate
  override func canPerformAction(_ action: Selector, withSender sender: Any?) -> Bool {
    if action == #selector(copy(_:)) {
      return true
    }
    
    if action == #selector(selectAll(_:)) {
      return true
    }
    
    if action == #selector(cut(_:)) {
      return false
    }
    
    if action == #selector(paste(_:)) {
      return false
    }
    return super.canPerformAction(action, withSender: sender)
  }
  
  override func selectAll(_ sender: Any?) {
    [payloadView, requestView, responseView].forEach({
      if $0.isFirstResponder {
        $0.selectAll(sender)
      }
    })
  }
  
  override func copy(_ sender: Any?) {
    let pasteBoard = UIPasteboard.general
    
    [payloadView, requestView, responseView].forEach({
      if $0.isFirstResponder {
        pasteBoard.string = $0.text
        $0.selectedTextRange = nil
        
        $0.resignFirstResponder()
      }
    })
  }
  
  // MARK: - Other
  
  private func resetViews() {
    requestView.text = ""
    responseView.text = ""
    payloadView.text = isEnrolled ? "Already Enrolled" : ""
  }
  
  private func setLoading(_ loading: Bool) {
    if loading {
      loadingView.startAnimating()
    } else {
      
      loadingView.stopAnimating()
    }
    
    view.isUserInteractionEnabled = !loading
    loadingView.isHidden = !loading
  }
  
  private func generateToken(_ completion: @escaping (Error?) -> Void) {
    deviceCheckClient.generateToken({ [weak self] token, error in
      guard let self = self else { return }
      
      DispatchQueue.main.async {
        if let error = error {
          self.payloadView.text = "ERROR: \(error)"
          completion(error)
          return
        }
        
        self.payloadView.text = "\(token)"
        self.token = token
        completion(nil)
      }
    })
  }
  
  private func registerDevice(_ completion: @escaping () -> Void) {
    generateToken { [weak self] error in
      guard let self = self else { return }
      
      if let error = error {
        self.requestView.text = "Enrollment ERROR: \(error)"
        return
      }
      
      guard let token = self.token else {
        self.requestView.text = "Enrollment ERROR: Failed to Generate Device Check Token"
        return
      }
      
      self.deviceCheckClient.generateEnrollment(paymentId: self.paymentId, token: token) { [weak self] registration, error in
        guard let self = self else { return }
        
        if let error = error {
          self.requestView.text = "Enrollment ERROR: \(error)"
          completion()
          return
        }
        
        guard let registration = registration else {
          self.requestView.text = "Enrollment ERROR: Failed to generate registration"
          completion()
          return
        }
        
        self.requestView.text = self.modelToString(registration)
        
        self.deviceCheckClient.registerDevice(enrollment: registration) { error in
          if let error = error {
            self.responseView.text = "Enrollment ERROR: \(error)"
            completion()
            return
          }
          
          self.responseView.text = "SUCCESSFULLY ENROLLED"
          completion()
        }
      }
    }
  }
  
  private func getAttestation(_ completion: @escaping (AttestationBlob?) -> Void) {
    deviceCheckClient.generateAttestation(paymentId: paymentId) { [weak self] attestation, error in
      guard let self = self else { return }
      
      if let error = error {
        self.requestView.text = "GetAttestation ERROR: \(error)"
        completion(nil)
        return
      }
      
      guard let attestation = attestation else {
        self.requestView.text = "GetAttestation ERROR: Failed to generate attestation"
        completion(nil)
        return
      }
      
      self.requestView.text = self.modelToString(attestation)
      
      self.deviceCheckClient.getAttestation(attestation: attestation) { attestationBlob, error in
        if let error = error {
          self.responseView.text = "GetAttestation ERROR: \(error)"
          completion(nil)
          return
        }
        
        guard let attestationBlob = attestationBlob else {
          self.responseView.text = "GetAttestation ERROR: No Attestation Blob"
          completion(nil)
          return
        }
        
        self.responseView.text = self.modelToString(attestationBlob)
        completion(attestationBlob)
      }
    }
  }
  
  private func setAttestation(_ blob: AttestationBlob, _ completion: @escaping () -> Void) {
    deviceCheckClient.generateAttestationVerification(nonce: blob.nonce) { [weak self] verification, error in
      guard let self = self else { return }
      
      if let error = error {
        self.requestView.text = "SetAttestation ERROR: \(error)"
        completion()
        return
      }
      
      guard let verification = verification else {
        self.requestView.text = "SetAttestation ERROR: Failed to generate verification"
        completion()
        return
      }
      
      self.requestView.text = self.modelToString(verification)
      
      self.deviceCheckClient.setAttestation(nonce: blob.nonce, verification: verification) { error in
        if let error = error {
          self.responseView.text = "SetAttestation ERROR: \(error)"
          completion()
          return
        }
        
        self.responseView.text = "SUCCESS"
        completion()
      }
    }
  }
  
  private func modelToString<T>(_ model: T) -> String? where T: Encodable {
    let encoder = JSONEncoder()
    encoder.outputFormatting = [.sortedKeys, .prettyPrinted]
    
    let data = (try? encoder.encode(model)) ?? Data()
    return String(data: data, encoding: .utf8)
  }
}

private extension UIPasteboard {
  func set(attributedString: NSAttributedString, _ completion: (Error?) -> Void) {
    do {
      let range = NSRange(location: 0, length: attributedString.length)
      let attributes = [NSAttributedString.DocumentAttributeKey.documentType: NSAttributedString.DocumentType.rtf]
      let rtf = try attributedString.data(from: range, documentAttributes: attributes)
      self.setData(rtf, forPasteboardType: kUTTypeRTF as String)
      completion(nil)
    } catch {
      completion(error)
    }
  }
}
