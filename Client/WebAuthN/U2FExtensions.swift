// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Data
import Shared
import WebKit
import Lottie

private struct FIDORegisterRequest: Codable {
    var challenge: String
    var version: String
    var appId: String?
}

private struct FIDOSignRequest: Codable {
    var version: String
    var challenge: String?
    var keyHandle: String
    var appId: String?
}

private struct FIDOLowLevel: Codable {
    var type: String
    var timeoutSeconds: Int
    var requestId: Int
}

private struct FIDOLowLevelRegisterRequests: Codable {
    var registerRequests: [FIDORegisterRequest]?
}

private struct FIDOLowLevelSignRequests: Codable {
    var signRequests: [FIDOSignRequest]?
}

private enum U2FErrorCodes: Int {
    case ok = 1, other_error, bad_request, configuration_unsupported, device_ineligible, timeout
}

private let log = Logger.browserLogger
private let defaultErrorCode = U2FErrorCodes.ok.rawValue
private let authSuccess = -1

private let signRequest = "u2f_sign_request"
private let registerRequest = "u2f_register_request"

// These are messages that are logged when U2F service encounters any errors
private enum U2FErrorMessages: String {
    case Error = "Error executing U2F request"
    case ErrorRegistration = "Error executing U2F registration request"
    case ErrorAuthentication = "Error executing U2F authentication request"
}

private enum FIDO2ErrorMessages: String {
    case NotAllowedError = "NotAllowedError"
    case SecurityError = "SecurityError"
    case InvalidStateError = "InvalidStateError"
}

private enum U2FMessageType: String {
    case FIDO2Create = "fido2-create"
    case FIDO2Get = "fido2-get"
    case FIDORegister = "fido-register"
    case FIDOSign = "fido-sign"
    case FIDOLowLevel = "fido-low-level"
    case None = "none"
}

class U2FExtensions: NSObject {
    fileprivate weak var tab: Tab?
    
    // There can be multiple Registration/Authentication requests at any time
    // We use handles to keep track of different requests
    fileprivate var fido2RegHandles: [Int] = []
    fileprivate var fido2AuthHandles: [Int] = []
    fileprivate var fidoRegHandles: [Int] = []
    fileprivate var fidoSignHandles: [Int] = []
    
    fileprivate var requestId: [Int: Int] = [:]
    fileprivate var fidoRequests: [Int: Int] = [:]
    
    fileprivate var fido2RegisterRequest: [Int: WebAuthnRegisterRequest] = [:]
    fileprivate var fido2AuthRequest: [Int: WebAuthnAuthenticateRequest] = [:]
    
    fileprivate var fidoRegisterRequest: [Int: FIDORegisterRequest] = [:]
    fileprivate var fidoSignRequests: [Int: [FIDOSignRequest]] = [:]
    
    fileprivate static var observationContext = 0
    
    // Popup modals presented to the user in different session or key states
    
    /// Show when user has to touch his auth key
    fileprivate let touchKeyPopup = AlertPopupView(imageView: lottieAnimation(for: "webauth_touch_key"),
                                                   title: Strings.touchKeyMessage, message: "", titleWeight: .semibold, titleSize: 21)
    
    /// Show when user's key hasn't been inserted yet
    fileprivate let insertKeyPopup = AlertPopupView(imageView: lottieAnimation(for: "webauth_insert_key"),
                                                    title: Strings.insertKeyMessage, message: "", titleWeight: .semibold, titleSize: 21)
 
    /// Show to enter key's pin
    fileprivate let pinVerificationPopup = AlertPopupView(imageView: UIImageView(image: #imageLiteral(resourceName: "enter_pin")),
                                                          title: Strings.pinTitle, message: "",
                                                          inputType: .default, secureInput: true,
                                                          inputPlaceholder: Strings.pinPlaceholder, titleWeight: .semibold, titleSize: 21)
    
    /// Show when key's pin authentication is pending
    fileprivate let verificationPendingPopup =
        AlertPopupView(imageView: lottieAnimation(for: "webauth_verify_key"),
                       title: Strings.verificationPending, message: "", titleWeight: .semibold, titleSize: 21)
    
    private static func lottieAnimation(for bundleResource: String) -> AnimationView {
        let animationView = AnimationView(name: bundleResource).then {
            $0.frame = CGRect(x: 0, y: 0, width: 200, height: 200)
            $0.contentMode = .scaleAspectFit
            $0.loopMode = .loop
            $0.play()
        }
        
        return animationView
    }
    
    fileprivate var u2fActive = false
    fileprivate var currentMessageType = U2FMessageType.None
    fileprivate var currentHandle = -1
    fileprivate var currentTabId = ""
    
    // Using a property style approch to avoid observing twice.
    private var observeSessionStateUpdates: Bool = false {
        didSet {
            if oldValue == observeSessionStateUpdates {
                return
            }
            
            let keySession = YubiKitManager.shared.keySession as AnyObject
            
            if observeSessionStateUpdates {
                keySession.addObserver(self, forKeyPath: #keyPath(YKFKeySession.sessionState), options: [.new, .old], context: &U2FExtensions.observationContext)
            } else {
                keySession.removeObserver(self, forKeyPath: #keyPath(YKFKeySession.sessionState))
            }
        }
    }
    
    private var observeKeyStateUpdates: Bool = false {
        didSet {
            if oldValue == observeKeyStateUpdates {
                return
            }
            
            let keySession = YubiKitManager.shared.keySession as AnyObject
            
            if observeKeyStateUpdates {
                keySession.addObserver(self, forKeyPath: #keyPath(YKFKeySession.fido2Service.keyState), options: [], context: &U2FExtensions.observationContext)
                keySession.addObserver(self, forKeyPath: #keyPath(YKFKeySession.u2fService.keyState), options: [], context: &U2FExtensions.observationContext)
            } else {
                keySession.removeObserver(self, forKeyPath: #keyPath(YKFKeySession.fido2Service.keyState))
                keySession.removeObserver(self, forKeyPath: #keyPath(YKFKeySession.u2fService.keyState))
            }
        }
    }

    init(tab: Tab) {
        self.tab = tab
        defer {
            observeSessionStateUpdates = true
            observeKeyStateUpdates = true
        }
        
        super.init()
        
        let handleCancelButton: () -> PopupViewDismissType = {
            let handle = self.currentHandle
            self.cleanupPinVerificationPopup()
            switch self.currentMessageType {
            case .FIDO2Create:
                self.sendFIDO2RegistrationError(handle: handle)
            case .FIDO2Get:
                self.sendFIDO2AuthenticationError(handle: handle)
            case.FIDORegister:
                self.sendFIDORegistrationError(handle: handle, requestId: self.requestId[handle] ?? -1, errorCode: U2FErrorCodes.other_error)
            case .FIDOSign:
                self.sendFIDOAuthenticationError(handle: handle, requestId: self.requestId[handle] ?? -1, errorCode: U2FErrorCodes.other_error)
            case .FIDOLowLevel, .None:
                break
            }
            return .flyDown
        }
        
        [touchKeyPopup, insertKeyPopup, pinVerificationPopup].forEach {
            $0.addButton(title: Strings.keyCancel, tapped: handleCancelButton)
        }
        
        // Make sure the session is started
        YubiKitManager.shared.keySession.startSession()
    }
    
    deinit {
        observeSessionStateUpdates = false
        observeKeyStateUpdates = false
    }
    
    private func validateURL(string: String?) -> Bool {
        let regEx = "((https|http)://)((\\w|-)+)(([.]|[/])((\\w|-)+))+"
        let predicate = NSPredicate(format: "SELF MATCHES %@", argumentArray: [regEx])
        return predicate.evaluate(with: string)
    }
    
    // RPID can be equal or shorter as long as there is still a host.
    func validateRPID(url: String, rpId: String) -> Bool {
        guard let currentURL = URL(string: url) else {
            return false
        }
        
        guard let rpIdURL = rpId.hasPrefix("http://") || rpId.hasPrefix("https://") ?  URL(string: rpId) : URL(string: "https://" + rpId) else {
            return false
        }
        
        // rpID should be valid
        if !validateURL(string: rpIdURL.absoluteString) {
            return false
        }
        
        guard let currentURLHost = currentURL.host else {
            return false
        }
        
        guard let rpIdHost = rpIdURL.host else {
            return false
        }
        
        return currentURLHost.hasSuffix(rpIdHost)
    }

    private func getCurrentURL() -> String? {
        guard let url = self.tab?.url else {
            return nil
        }
        return url.domainURL().absoluteString
    }
    
    private func setCurrentTabId() {
        guard let tabId = tab?.id else {
            sendFIDO2RegistrationError(handle: currentHandle)
            return
        }
        currentTabId = tabId
    }
    
    override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
        // We cancel all observers in ObserveValue to avoid excessive queuing on
        // the main thread when the keys are disconnected continuosly.
        disableObservers()
        
        guard context == &U2FExtensions.observationContext else {
            super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
            return
        }
        
        switch keyPath {
        case #keyPath(YKFKeySession.sessionState):
            ensureMainThread {
                self.handleSessionStateChange()
                self.enableObservers()
            }
        case #keyPath(YKFKeySession.u2fService.keyState), #keyPath(YKFKeySession.fido2Service.keyState):
            ensureMainThread {
                self.presentInteractWithKeyModal()
                self.enableObservers()
            }
        default:
            return
        }
    }
    
    // FIDO2 Registration
    private func requestFIDO2Registration(handle: Int, request: WebAuthnRegisterRequest) {
        fido2RegHandles.append(handle)
        fido2RegisterRequest[handle] = request

        if YubiKitManager.shared.keySession.sessionState != .open {
            return
        }
        
        handleFIDO2Registration(handle: handle, request: request)
    }
    
    private func handleFIDO2Registration(handle: Int, request: WebAuthnRegisterRequest) {
        currentMessageType = U2FMessageType.FIDO2Create
        currentHandle = handle
        
        setCurrentTabId()
        
        let makeCredentialRequest = YKFKeyFIDO2MakeCredentialRequest()
        guard let url = getCurrentURL() else {
            sendFIDO2RegistrationError(handle: handle, errorName: FIDO2ErrorMessages.SecurityError.rawValue)
            return
        }
        
        let publicKey = request.publicKey
        let clientData = WebAuthnClientData(type: WebAuthnClientDataType.create.rawValue, challenge: publicKey.challenge, origin: url)
        
        do {
            let clientDataJSON = try JSONEncoder().encode(clientData)
            guard let clientDataHash = clientDataHash(data: clientDataJSON) else {
                sendFIDO2RegistrationError(handle: handle)
                return
            }
            
            makeCredentialRequest.clientDataHash = clientDataHash
            
            let rp = YKFFIDO2PublicKeyCredentialRpEntity()
            // If rpId is not present, then set rpId to effectiveDomain
            // https://www.w3.org/TR/webauthn/#GetAssn-DetermineRpId
            if let rpId = publicKey.rp.id {
                rp.rpId = rpId
            } else {
                guard let rpId = URL(string: url)?.host else {
                    sendFIDO2RegistrationError(handle: handle)
                    return
                }
                rp.rpId = rpId
            }

            guard validateRPID(url: url, rpId: rp.rpId) else {
                sendFIDO2RegistrationError(handle: handle, errorName: FIDO2ErrorMessages.SecurityError.rawValue)
                return
            }
                
            rp.rpName = publicKey.rp.name
            makeCredentialRequest.rp = rp
            
            let user = YKFFIDO2PublicKeyCredentialUserEntity()
            guard let userId = Data(base64Encoded: publicKey.user.id) else {
                sendFIDO2RegistrationError(handle: handle)
                return
            }
            user.userId = userId
            user.userName = publicKey.user.name
            makeCredentialRequest.user = user
            
            let param = YKFFIDO2PublicKeyCredentialParam()
            guard let publicKeyCred = publicKey.pubKeyCredParams.first else {
                sendFIDO2RegistrationError(handle: handle)
                return
            }
            param.alg = publicKeyCred.alg
            makeCredentialRequest.pubKeyCredParams = [param]
            
            let makeOptions = [
                YKFKeyFIDO2MakeCredentialRequestOptionRK: publicKey.authenticatorSelection?.requireResidentKey ?? false
            ]
            makeCredentialRequest.options = makeOptions
            
            let exclusionList: [YKFFIDO2PublicKeyCredentialDescriptor] = publicKey.excludeCredentials?.compactMap({
                let credentialDescriptor = YKFFIDO2PublicKeyCredentialDescriptor()
                guard let credentialIdData = Data(base64Encoded: $0.id) else {
                    return nil
                }
                
                credentialDescriptor.credentialId = credentialIdData
                credentialDescriptor.credentialType = YKFFIDO2PublicKeyCredentialType().then {
                    $0.name = "public-key"
                }
                
                return credentialDescriptor
            }) ?? []
            
            guard exclusionList.count == publicKey.excludeCredentials?.count ?? 0 else {
                sendFIDO2RegistrationError(handle: handle)
                return
            }
            
            makeCredentialRequest.excludeList = exclusionList
            
            guard let fido2Service = YubiKitManager.shared.keySession.fido2Service else {
                self.sendFIDO2RegistrationError(handle: handle)
                return
            }
            
            fido2Service.execute(makeCredentialRequest) { [weak self] response, error in
                guard let self = self else {
                    log.error(U2FErrorMessages.ErrorRegistration.rawValue)
                    return
                }
                guard error == nil else {
                    self.handleMakeCredential(handle: handle, request: request, error: error)
                    return
                }
                
                guard let response = response else {
                    self.sendFIDO2RegistrationError(handle: handle)
                    return
                }
                self.finalizeFIDO2Registration(handle: handle, response: response, clientDataJSON: clientDataJSON.base64EncodedString(), error: nil)
            }
        } catch {
            sendFIDO2RegistrationError(handle: handle, errorDescription: error.localizedDescription)
        }
    }
    
    private func finalizeFIDO2Registration(handle: Int, response: YKFKeyFIDO2MakeCredentialResponse, clientDataJSON: String, error: NSErrorPointer) {
        guard error == nil else {
            let errorDescription = error?.pointee?.localizedDescription ?? Strings.U2FRegistrationError
            sendFIDO2RegistrationError(handle: handle, errorDescription: errorDescription)
            return
        }
        
        let attestationString = response.webauthnAttestationObject.base64EncodedString()
        guard let authenticatorData = response.authenticatorData else {
            sendFIDO2RegistrationError(handle: handle)
            return
        }

        let credentialId = authenticatorData.credentialId
        guard let credentialIdString = credentialId?.base64EncodedString() else {
            sendFIDO2RegistrationError(handle: handle)
            return
        }
        cleanupFIDO2Registration(handle: handle)
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("navigator.credentials.postCreate('\(handle)', \(true), '\(credentialIdString)', '\(attestationString)', '\(clientDataJSON)', '', '')", completionHandler: { _, error in
                if error != nil {
                    let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorRegistration.rawValue
                    log.error(errorDescription)
                }
        }) }
    }
    
    private func handleMakeCredential(handle: Int, request: WebAuthnRegisterRequest, error: Error?) {
        guard let error = error else {
            log.error("Error should not be nil in handleMakeCredential")
            return
        }
        
        let makeCredentialError = error as NSError
        
        if makeCredentialError.code == YKFKeyFIDO2ErrorCode.CREDENTIAL_EXCLUDED.rawValue {
            sendFIDO2RegistrationError(handle: handle, errorName: FIDO2ErrorMessages.InvalidStateError.rawValue, errorDescription: error.localizedDescription)
            return
        }
        
        guard makeCredentialError.code == YKFKeyFIDO2ErrorCode.PIN_REQUIRED.rawValue else {
            let errorDescription = error.localizedDescription
            sendFIDO2RegistrationError(handle: handle, errorDescription: errorDescription)
            return
        }
        
        handlePinVerificationRequired { [weak self] (error) in
            guard let self = self else {
                return
            }
            self.cleanupPinVerificationPopup()
            ensureMainThread {
                self.verificationPendingPopup.dismissWithType(dismissType: .flyDown)
            }
            
            if error == true {
                self.sendFIDO2RegistrationError(handle: handle)
                return
            }
            self.handleFIDO2Registration(handle: handle, request: request)
        }
        return
    }
    
    private func sendFIDO2RegistrationError(handle: Int, errorName: String = FIDO2ErrorMessages.NotAllowedError.rawValue, errorDescription: String = Strings.U2FRegistrationError) {
        cleanupFIDO2Registration(handle: handle)
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("navigator.credentials.postCreate('\(handle)', \(true),'', '', '', '\(errorName.toBase64())', '\(errorDescription.toBase64())')", completionHandler: { _, error in
                if error != nil {
                    let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorRegistration.rawValue
                    log.error(errorDescription)
                }
        }) }
    }
    
    private func cleanupFIDO2Registration(handle: Int) {
        u2fActive = false
        guard let index = fido2RegHandles.firstIndex(of: handle) else {
            log.error(U2FErrorMessages.ErrorRegistration)
            return
        }
        fido2RegHandles.remove(at: index)
        fido2RegisterRequest.removeValue(forKey: handle)
        YubiKitManager.shared.keySession.cancelCommands()
    }
    
    // FIDO2 Authentication
    private func requestFIDO2Authentication(handle: Int, request: WebAuthnAuthenticateRequest) {
        fido2AuthHandles.append(handle)
        fido2AuthRequest[handle] = request
        
        if YubiKitManager.shared.keySession.sessionState != .open {
            return
        }
        
        handleFIDO2Authentication(handle: handle, request: request)
    }
    
    private func handleFIDO2Authentication(handle: Int, request: WebAuthnAuthenticateRequest) {
        currentMessageType = U2FMessageType.FIDO2Get
        currentHandle = handle
        
        setCurrentTabId()
        
        let getAssertionRequest = YKFKeyFIDO2GetAssertionRequest()
        guard let url = getCurrentURL() else {
            sendFIDO2AuthenticationError(handle: handle, errorName: FIDO2ErrorMessages.SecurityError.rawValue)
            return
        }
        
        let clientData = WebAuthnClientData(type: WebAuthnClientDataType.get.rawValue, challenge: request.challenge, origin: url)
        do {
            let clientDataJSON = try JSONEncoder().encode(clientData)
            
            if let rpId = request.rpID {
                getAssertionRequest.rpId = rpId
            } else {
                guard let rpId = URL(string: url)?.host else {
                    sendFIDO2AuthenticationError(handle: handle)
                    return
                }
                getAssertionRequest.rpId = rpId
            }
            
            guard validateRPID(url: url, rpId: getAssertionRequest.rpId) else {
                sendFIDO2RegistrationError(handle: handle, errorName: FIDO2ErrorMessages.SecurityError.rawValue)
                return
            }

            guard let clientDataHash = clientDataHash(data: clientDataJSON) else {
                sendFIDO2AuthenticationError(handle: handle)
                return
            }
            getAssertionRequest.clientDataHash = clientDataHash
            
            getAssertionRequest.options = [
                YKFKeyFIDO2GetAssertionRequestOptionUP: request.userPresence
            ]

            var allowList = [YKFFIDO2PublicKeyCredentialDescriptor]()
            if request.allowCredentials.count > 0 {
                for credentialId in request.allowCredentials {
                    let credentialDescriptor = YKFFIDO2PublicKeyCredentialDescriptor()
                    
                    guard let credentialIdData = Data(base64Encoded: credentialId) else {
                        sendFIDO2AuthenticationError(handle: handle)
                        return
                    }
                    
                    credentialDescriptor.credentialId = credentialIdData
                    let credType = YKFFIDO2PublicKeyCredentialType()
                    credType.name = "public-key"
                    credentialDescriptor.credentialType = credType
                    allowList.append(credentialDescriptor)
                }
                getAssertionRequest.allowList = allowList
            }
            
            guard let fido2Service = YubiKitManager.shared.keySession.fido2Service else {
                sendFIDO2AuthenticationError(handle: handle)
                return
            }
            
            fido2Service.execute(getAssertionRequest) { [weak self] response, error in
                guard let self = self else {
                    log.error(U2FErrorMessages.ErrorAuthentication.rawValue)
                    return
                }
                guard error == nil else {
                    self.handleGetAssertion(handle: handle, request: request, error: error)
                    return
                }
                
                // The reponse from the key must not be empty at this point.
                guard let response = response else {
                    self.sendFIDO2AuthenticationError(handle: handle)
                    return
                }
                self.finalizeFIDO2Authentication(handle: handle, response: response, clientDataJSON: clientDataJSON, error: nil)
            }
        } catch {
            sendFIDO2AuthenticationError(handle: handle, errorDescription: error.localizedDescription)
        }
    }
    
    private func finalizeFIDO2Authentication(handle: Int, response: YKFKeyFIDO2GetAssertionResponse, clientDataJSON: Data, error: NSErrorPointer) {
        guard error == nil else {
            let errorDescription = error?.pointee?.localizedDescription ?? Strings.U2FAuthenticationError
            sendFIDO2AuthenticationError(handle: handle, errorDescription: errorDescription)
            return
        }
        
        let authenticatorData = response.authData.base64EncodedString()
        let clientDataJSONString = clientDataJSON.base64EncodedString()
        let sig = response.signature.base64EncodedString()

       let userHandle = response.user?.userId.base64EncodedString() ?? ""
        
        guard let requestId = response.credential?.credentialId.base64EncodedString() else {
            sendFIDO2AuthenticationError(handle: handle)
            return
        }
        
        cleanupFIDO2Authentication(handle: handle)
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("navigator.credentials.postGet('\(handle)', \(true), '\(requestId)', '\(authenticatorData)', '\(clientDataJSONString)', '\(sig)', '\(userHandle)', '')", completionHandler: { _, error in
                if error != nil {
                    let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorAuthentication.rawValue
                    log.error(errorDescription)
                }
        }) }
    }
    
    private func handleGetAssertion(handle: Int, request: WebAuthnAuthenticateRequest, error: Error?) {
        guard let error = error else {
            log.error("Error should not be nil in handleGetAssertion")
            return
        }
        
        let getAssertionError = error as NSError
        
        guard getAssertionError.code == YKFKeyFIDO2ErrorCode.PIN_REQUIRED.rawValue else {
            let errorDescription = error.localizedDescription
            sendFIDO2AuthenticationError(handle: handle, errorDescription: errorDescription)
            return
        }
        
        handlePinVerificationRequired { [weak self] (error) in
            guard let self = self else {
                return
            }
            self.cleanupPinVerificationPopup()
            ensureMainThread {
                self.verificationPendingPopup.dismissWithType(dismissType: .flyDown)
            }
            if error == true {
                self.sendFIDO2AuthenticationError(handle: handle)
                return
            }
            self.handleFIDO2Authentication(handle: handle, request: request)
        }
        return
    }
    
    private func sendFIDO2AuthenticationError(handle: Int, errorName: String = FIDO2ErrorMessages.NotAllowedError.rawValue, errorDescription: String = Strings.U2FAuthenticationError) {
        cleanupFIDO2Authentication(handle: handle)
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("navigator.credentials.postGet('\(handle)', \(true), '', '', '', '', '', '\(errorName.toBase64())', '\(errorDescription.toBase64())')", completionHandler: { _, error in
                if error != nil {
                    let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorAuthentication.rawValue
                    log.error(errorDescription)
                }
        }) }
    }
    
    // This modal is presented when FIDO/FIDO2 APIs are waiting for the security key
    private func presentInsertKeyModal() {
        let currentURL = self.tab?.url?.host ?? ""
        insertKeyPopup.update(title: Strings.insertKeyMessage + currentURL)
        insertKeyPopup.showWithType(showType: .flyUp)
    }
    
    // This modal is presented when the key bootstrap is complete
    private func presentInteractWithKeyModal() {
        guard let fido2Service = YubiKitManager.shared.keySession.fido2Service else {
            return
        }
        
        guard let u2fService = YubiKitManager.shared.keySession.u2fService else {
            return
        }
        
        // The modal should be visible for the tab where the U2F API is active
        if u2fActive && tab?.id == currentTabId && (fido2Service.keyState == .touchKey || u2fService.keyState == .YKFKeyU2FServiceKeyStateTouchKey) {
            let currentURL = self.tab?.url?.host ?? ""
            touchKeyPopup.update(title: Strings.touchKeyMessage + currentURL)
            touchKeyPopup.showWithType(showType: .flyUp)
            return
        }
        touchKeyPopup.dismissWithType(dismissType: .flyDown)
    }
    
    private func handlePinVerificationRequired(completion: @escaping (Bool) -> Void) {
        ensureMainThread {
            let currentURL = self.tab?.url?.host ?? ""
            self.pinVerificationPopup.addButton(title: Strings.confirmPin, type: .primary) { [weak self] in
                self?.verificationPendingPopup.update(title: Strings.verificationPending + currentURL)
                self?.verificationPendingPopup.showWithType(showType: .flyUp)
                return self?.verifyPin(completion: completion) ?? .flyDown
            }
            self.pinVerificationPopup.showWithType(showType: .flyUp)
        }
    }
    
    private func verifyPin(completion: @escaping (Bool) -> Void) -> PopupViewDismissType {
        guard
            let pin = pinVerificationPopup.text, !pin.isEmpty,
            let fido2Service = YubiKitManager.shared.keySession.fido2Service,
            let verifyPinRequest = YKFKeyFIDO2VerifyPinRequest(pin: pin) else {
                completion(true)
                return .flyDown
            }
            
        fido2Service.execute(verifyPinRequest) { (error) in
            guard error == nil else {
                completion(true)
                return
            }
            completion(false)
            return
        }
        return .flyDown
    }
    
    // Confirm key with action is populated at run-time, we clean the state
    // when the dialog box is dismissed.
    private func cleanupPinVerificationPopup() {
        ensureMainThread {
            self.pinVerificationPopup.removeButtonAtIndex(buttonIndex: 1)
            self.pinVerificationPopup.clearTextField()
        }
    }
    
    private func disableObservers() {
        observeSessionStateUpdates = false
        observeKeyStateUpdates = false
    }
    
    private func enableObservers() {
        observeSessionStateUpdates = true
        observeKeyStateUpdates = true
    }
    
    private func cleanupFIDO2Authentication(handle: Int) {
        u2fActive = false
        guard let index = fido2AuthHandles.firstIndex(of: handle) else {
            log.error(U2FErrorMessages.ErrorRegistration)
            return
        }
        fido2AuthHandles.remove(at: index)
        fido2AuthRequest.removeValue(forKey: handle)
        YubiKitManager.shared.keySession.cancelCommands()
    }
    
    // FIDO Registration
    private func requestFIDORegistration(handle: Int, requests: [FIDORegisterRequest], id: Int) {
        fidoRegHandles.append(handle)
        
        guard let request = requests.first else {
            sendFIDORegistrationError(handle: handle, requestId: id, errorCode: U2FErrorCodes.bad_request)
            return
        }
    
        fidoRegisterRequest[handle] = request
        
        if id >= 0 {
            requestId[handle] = id
        }

        if YubiKitManager.shared.keySession.sessionState != .open {
            return
        }
        
        handleFIDORegistration(handle: handle, request: request, requestId: id)
    }
    
    private func handleFIDORegistration(handle: Int, request: FIDORegisterRequest, requestId: Int) {
        currentMessageType = U2FMessageType.FIDORegister
        currentHandle = handle
        
        setCurrentTabId()
        
        guard let registerRequest = YKFKeyU2FRegisterRequest(challenge: request.challenge, appId: request.appId ?? "") else {
            sendFIDORegistrationError(handle: handle, requestId: requestId, errorCode: U2FErrorCodes.bad_request)
            return
        }
        
        guard let u2fservice = YubiKitManager.shared.keySession.u2fService else {
            sendFIDORegistrationError(handle: handle, requestId: requestId, errorCode: U2FErrorCodes.other_error)
            return
        }
        
        u2fservice.execute(registerRequest) { [weak self] response, error in
            guard let self = self else {
                log.error(U2FErrorMessages.Error.rawValue)
                return
            }
            
            guard error == nil else {
                let errorMessage = error?.localizedDescription ?? Strings.U2FRegistrationError
                self.sendFIDORegistrationError(handle: handle, requestId: requestId, errorCode: U2FErrorCodes.other_error, errorMessage: errorMessage)
                return
            }
            guard let clientData = response?.clientData.websafeBase64String() else {
                self.sendFIDORegistrationError(handle: handle, requestId: requestId, errorCode: U2FErrorCodes.other_error)
                return
            }
            guard let registrationData = response?.registrationData.websafeBase64String() else {
                self.sendFIDORegistrationError(handle: handle, requestId: requestId, errorCode: U2FErrorCodes.other_error)
                return
            }
            
            self.cleanupFIDORegistration(handle: handle)
            if requestId >= 0 {
                ensureMainThread {
                    self.tab?.webView?.evaluateJavaScript("u2f.postLowLevelRegister(\(requestId), \(true), '\(request.version)', '\(registrationData)', '\(clientData)', \(defaultErrorCode), '')", completionHandler: { _, error in
                        if error != nil {
                            let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorRegistration.rawValue
                            log.error(errorDescription)
                        }
                }) }
                return
            }

            ensureMainThread {
                self.tab?.webView?.evaluateJavaScript("u2f.postRegister('\(handle)', \(true), '\(request.version)', '\(registrationData)', '\(clientData)', \(defaultErrorCode), '')", completionHandler: { _, error in
                    if error != nil {
                        let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorRegistration.rawValue
                        log.error(errorDescription)
                    }
            }) }
        }
    }
    
    private func sendFIDORegistrationError(handle: Int, requestId: Int, errorCode: U2FErrorCodes, errorMessage: String = Strings.U2FRegistrationError) {
        cleanupFIDORegistration(handle: handle)
        if requestId >= 0 {
            ensureMainThread {
                self.tab?.webView?.evaluateJavaScript("u2f.postLowLevelRegister(\(requestId), \(true), '', '', '', \(errorCode.rawValue), '\(errorMessage.toBase64())')", completionHandler: { _, error in
                    if error != nil {
                        let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorRegistration.rawValue
                        log.error(errorDescription)
                    }
            }) }
            return
        }
        
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("u2f.postRegister('\(handle)', \(true), '', '', '', \(errorCode.rawValue), '\(errorMessage.toBase64())')", completionHandler: { _, error in
                if error != nil {
                    let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorRegistration.rawValue
                    log.error(errorDescription)
                }
            }) }
    }
    
    private func cleanupFIDORegistration(handle: Int) {
        u2fActive = false
        guard let index = fidoRegHandles.firstIndex(of: handle) else {
            log.error(U2FErrorMessages.ErrorRegistration)
            return
        }
        fidoRegHandles.remove(at: index)
        fidoRegisterRequest.removeValue(forKey: handle)
        requestId.removeValue(forKey: handle)
        YubiKitManager.shared.keySession.cancelCommands()
    }
    
    // FIDO Authetication
    private func requestFIDOAuthentication(handle: Int, keys: [FIDOSignRequest], id: Int) {
        fidoSignHandles.append(handle)
        fidoSignRequests[handle] = keys
        fidoRequests[handle] = keys.count
        
        if id >= 0 {
            requestId[handle] = id
        }

        if YubiKitManager.shared.keySession.sessionState != .open {
            return
        }
        
        handleFIDOAuthentication(handle: handle, keys: keys, requestId: id)
    }
    
    private func handleFIDOAuthentication(handle: Int, keys: [FIDOSignRequest], requestId: Int) {
        currentMessageType = U2FMessageType.FIDOSign
        currentHandle = handle
        
        setCurrentTabId()
        
        guard let u2fservice = YubiKitManager.shared.keySession.u2fService else {
            sendFIDOAuthenticationError(handle: handle, requestId: requestId, errorCode: U2FErrorCodes.other_error)
            return
        }
        for key in keys {
            guard let signRequest = YKFKeyU2FSignRequest(challenge: key.challenge ?? "", keyHandle: key.keyHandle, appId: key.appId ?? "") else {
                sendFIDOAuthenticationError(handle: handle, requestId: requestId, errorCode: U2FErrorCodes.bad_request)
                return
            }
            
            guard var count = self.fidoRequests[handle] else {
                log.error(U2FErrorMessages.ErrorAuthentication.rawValue)
                return
            }
            
            if count == authSuccess {
                // auth was successful we don't need to execute again
                return
            }
            
            u2fservice.execute(signRequest) { [weak self] response, error in
                guard let self = self else {
                    log.error(U2FErrorMessages.ErrorAuthentication.rawValue)
                    return
                }
                
                count -= 1
                self.fidoRequests[handle] = count
                
                guard error == nil else {
                    if count == 0 {
                        let errorMessage = error?.localizedDescription ?? Strings.U2FAuthenticationError
                        self.sendFIDOAuthenticationError(handle: handle, requestId: requestId, errorCode: U2FErrorCodes.other_error, errorMessage: errorMessage)
                    }
                    return
                }
                guard let keyHandle = response?.keyHandle else {
                    self.sendFIDOAuthenticationError(handle: handle, requestId: requestId, errorCode: U2FErrorCodes.other_error)
                    return
                }
                guard let signature = response?.signature.websafeBase64String() else {
                    self.sendFIDOAuthenticationError(handle: handle, requestId: requestId, errorCode: U2FErrorCodes.other_error)
                    return
                }
                guard let clientData = response?.clientData.websafeBase64String() else {
                    self.sendFIDOAuthenticationError(handle: handle, requestId: requestId, errorCode: U2FErrorCodes.other_error)
                    return
                }
                
                self.fidoRequests[handle] = authSuccess
                
                self.cleanupFIDOAuthentication(handle: handle)
                if requestId >= 0 {
                    ensureMainThread {
                        self.tab?.webView?.evaluateJavaScript("u2f.postLowLevelSign(\(requestId), \(true), '\(keyHandle)', '\(signature)', '\(clientData)', \(defaultErrorCode), '')", completionHandler: { _, error in
                            if error != nil {
                                let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorAuthentication.rawValue
                                log.error(errorDescription)
                            }
                    }) }
                    return
                }

                ensureMainThread {
                    self.tab?.webView?.evaluateJavaScript("u2f.postSign('\(handle)', \(true), '\(keyHandle)', '\(signature)', '\(clientData)', \(defaultErrorCode), '')", completionHandler: { _, error in
                        if error != nil {
                            let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorAuthentication.rawValue
                            log.error(errorDescription)
                        }
                }) }
            }
        }
    }
    
    private func sendFIDOAuthenticationError(handle: Int, requestId: Int, errorCode: U2FErrorCodes, errorMessage: String = Strings.U2FAuthenticationError) {
        cleanupFIDOAuthentication(handle: handle)
        
        if requestId >= 0 {
            ensureMainThread {
                self.tab?.webView?.evaluateJavaScript("u2f.postLowLevelSign(\(requestId), \(true), '', '', '', \(errorCode.rawValue), '\(errorMessage.toBase64())')", completionHandler: { _, error in
                    if error != nil {
                        let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorAuthentication.rawValue
                        log.error(errorDescription)
                    }
            }) }
            return
        }
        
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("u2f.postSign('\(handle)', \(true), '', '', '', \(errorCode.rawValue), '\(errorMessage.toBase64())')", completionHandler: { _, error in
                if error != nil {
                    let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorAuthentication.rawValue
                    log.error(errorDescription)
                }
        }) }
    }
    
    private func cleanupFIDOAuthentication(handle: Int) {
        u2fActive = false
        guard let index = fidoSignHandles.firstIndex(of: handle) else {
            log.error(U2FErrorMessages.ErrorRegistration)
            return
        }
        fidoSignHandles.remove(at: index)
        fidoSignRequests.removeValue(forKey: handle)
        requestId.removeValue(forKey: handle)
        YubiKitManager.shared.keySession.cancelCommands()
    }
    
    private func handleSessionStateChange() {
        let sessionState = YubiKitManager.shared.keySession.sessionState
        if sessionState == .open { // The key session is ready to be used.
            insertKeyPopup.dismissWithType(dismissType: .flyDown)
            
            if !fido2RegHandles.isEmpty {
                guard let handle = fido2RegHandles.first else {
                    log.error(U2FErrorMessages.ErrorRegistration)
                    return
                }
                guard let request = fido2RegisterRequest[handle] else {
                    sendFIDO2RegistrationError(handle: handle)
                    return
                }
                handleFIDO2Registration(handle: handle, request: request)
            }
            
            if !fidoRegHandles.isEmpty {
                guard let handle = fidoRegHandles.first else {
                    log.error(U2FErrorMessages.ErrorRegistration)
                    return
                }
                guard let request = fidoRegisterRequest[handle] else {
                    sendFIDORegistrationError(handle: handle, requestId: requestId[handle] ?? -1, errorCode: U2FErrorCodes.other_error)
                    return
                }
                handleFIDORegistration(handle: handle, request: request, requestId: requestId[handle] ?? -1)
            }
        
            if !fido2AuthHandles.isEmpty {
                guard let handle = fido2AuthHandles.first else {
                    log.error(U2FErrorMessages.ErrorAuthentication)
                    return
                }
                guard let request = fido2AuthRequest[handle] else {
                    sendFIDO2AuthenticationError(handle: handle)
                    return
                }
                handleFIDO2Authentication(handle: handle, request: request)
            }
        
            if !fidoSignHandles.isEmpty {
                guard let handle = fidoSignHandles.first else {
                    log.error(U2FErrorMessages.ErrorAuthentication)
                    return
                }
                guard let keys = fidoSignRequests[handle] else {
                    sendFIDOAuthenticationError(handle: handle, requestId: requestId[handle] ?? -1, errorCode: U2FErrorCodes.other_error)
                    return
                }
                handleFIDOAuthentication(handle: handle, keys: keys, requestId: requestId[handle] ?? -1)
            }
        } else {
            if u2fActive == true {
                presentInsertKeyModal()
            }
            cleanupPinVerificationPopup()
            touchKeyPopup.dismissWithType(dismissType: .flyDown)
            pinVerificationPopup.dismissWithType(dismissType: .flyDown)
        }
    }
}

// MARK: - TabContentScript
extension U2FExtensions: TabContentScript {
    static func name() -> String {
        return "U2F"
    }
    
    func scriptMessageHandlerName() -> String? {
        return "U2F"
    }
    
    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        if message.name == "U2F", let body = message.body as? NSDictionary {
            
            guard let name = body["name"] as? String, let handle = body["handle"] as? Int else {
                log.error(U2FErrorMessages.Error)
                return
            }
            
            u2fActive = true
            currentHandle = handle
            currentMessageType = U2FMessageType(rawValue: name) ?? U2FMessageType.None
            
            if YubiKitManager.shared.keySession.sessionState != .open {
                presentInsertKeyModal()
            }
            
            switch name {
                case U2FMessageType.FIDO2Create.rawValue:
                    // FIDO2 is the new webauthn API
                    // https://www.w3.org/TR/webauthn/
                    guard let data = body["data"] as? String, let jsonData = data.data(using: String.Encoding.utf8) else {
                        sendFIDO2RegistrationError(handle: handle)
                        return
                    }
                    
                    do {
                        let request =  try JSONDecoder().decode(WebAuthnRegisterRequest.self, from: jsonData)
                        requestFIDO2Registration(handle: handle, request: request)
                    } catch {
                        sendFIDO2RegistrationError(handle: handle, errorDescription: error.localizedDescription)
                    }
                case U2FMessageType.FIDO2Get.rawValue:
                    guard let data = body["data"] as? String, let jsonData = data.data(using: String.Encoding.utf8) else {
                        sendFIDO2AuthenticationError(handle: handle)
                        return
                    }
                    
                    do {
                        let request =  try JSONDecoder().decode(WebAuthnAuthenticateRequest.self, from: jsonData)
                        requestFIDO2Authentication(handle: handle, request: request)
                    } catch {
                        sendFIDO2AuthenticationError(handle: handle, errorDescription: error.localizedDescription)
                    }
                case U2FMessageType.FIDORegister.rawValue:
                    // High Level FIDO U2F API
                    // https://fidoalliance.org/specs/fido-u2f-v1.0-nfc-bt-amendment-20150514/fido-u2f-javascript-api.html#high-level-javascript-api
                    guard let appId = body["appId"] as? String, let requests = body["requests"] as? String else {
                        sendFIDORegistrationError(handle: handle, requestId: -1, errorCode: U2FErrorCodes.bad_request)
                        return
                    }
                    
                    let jsonData = Data(requests.utf8)
                    let decoder = JSONDecoder()
                    do {
                        var registerRequests = try decoder.decode([FIDORegisterRequest].self, from: jsonData)
                        for index in registerRequests.indices {
                            registerRequests[index].appId = appId
                        }
                        requestFIDORegistration(handle: handle, requests: registerRequests, id: -1)
                    } catch {
                        sendFIDORegistrationError(handle: handle, requestId: -1, errorCode: U2FErrorCodes.bad_request, errorMessage: error.localizedDescription)
                    }
                case U2FMessageType.FIDOSign.rawValue:
                    guard let appId = body["appId"] as? String, let challenge = body["challenge"] as? String, let keys = body["keys"] as? String else {
                        sendFIDOAuthenticationError(handle: handle, requestId: -1, errorCode: U2FErrorCodes.bad_request)
                        return
                    }
                    
                    let jsonData = Data(keys.utf8)
                    let decoder = JSONDecoder()
                    do {
                        var registeredKeys = try decoder.decode([FIDOSignRequest].self, from: jsonData)
                        for index in registeredKeys.indices {
                            registeredKeys[index].challenge = challenge
                            registeredKeys[index].appId = appId
                        }
                        requestFIDOAuthentication(handle: handle, keys: registeredKeys, id: -1)
                    } catch {
                        sendFIDOAuthenticationError(handle: handle, requestId: -1, errorCode: U2FErrorCodes.bad_request, errorMessage: error.localizedDescription)
                    }
                case U2FMessageType.FIDOLowLevel.rawValue:
                    // Low Level FIDO U2F API use message port for interaction
                    // https://fidoalliance.org/specs/fido-u2f-v1.0-nfc-bt-amendment-20150514/fido-u2f-javascript-api.html#low-level-messageport-api
                    guard let data = body["data"] as? String else {
                        log.error(U2FErrorMessages.Error.rawValue)
                        return
                    }
                    
                    let jsonData = Data(data.utf8)
                    let decoder = JSONDecoder()
                    do {
                        let lowLevelRequest = try decoder.decode(FIDOLowLevel.self, from: jsonData)
                        if lowLevelRequest.type == registerRequest {
                            let request = try decoder.decode(FIDOLowLevelRegisterRequests.self, from: jsonData)
                            guard let registerRequests = request.registerRequests else {
                                log.error(Strings.U2FRegistrationError)
                                return
                            }
                            requestFIDORegistration(handle: handle, requests: registerRequests, id: lowLevelRequest.requestId)
                        }
                        if lowLevelRequest.type == signRequest {
                            let request = try decoder.decode(FIDOLowLevelSignRequests.self, from: jsonData)
                            guard let signRequests = request.signRequests else {
                                log.error(Strings.U2FAuthenticationError)
                                return
                            }
                            requestFIDOAuthentication(handle: handle, keys: signRequests, id: lowLevelRequest.requestId)
                        }
                    } catch {
                        log.error(error.localizedDescription)
                    }
                default:
                    insertKeyPopup.dismissWithType(dismissType: .flyDown)
                    log.error(U2FErrorMessages.Error)
            }
        }
    }
}

extension Strings {
    //FIDO & FIDO2 Error messages
    public static let tryAgain = NSLocalizedString("tryAgain", tableName: "BraveShared", bundle: Bundle.braveShared, value: ", please try again.", comment: "Suffix for error strings")
    public static let U2FRegistrationError = NSLocalizedString("U2FRegistrationError", tableName: "BraveShared", bundle: Bundle.braveShared, value: "Error registering your security key", comment: "Error handling U2F registration.") + tryAgain
    public static let U2FAuthenticationError = NSLocalizedString("U2FAuthenticationError", tableName: "BraveShared", bundle: Bundle.braveShared, value: "Error authenticating your security key", comment: "Error handling U2F authentication.") + tryAgain
    
    //Lightning Modals
    public static let touchKeyMessage = NSLocalizedString("touchKeyMessage", bundle: Bundle.shared, value: "Touch your key to finish the request for ", comment: "Message for touch key modal.")
    public static let insertKeyMessage = NSLocalizedString("insertKeyMessage", bundle: Bundle.shared, value: "Insert your security key for ", comment: "Message for touch key modal.")
    public static let keyCancel = NSLocalizedString("touchKeyCancel", bundle: Bundle.shared, value: "Cancel", comment: "Text for touch key modal button.")
    
    //PIN
    public static let pinTitle = NSLocalizedString("pinTitle", bundle: Bundle.shared, value: "PIN verification required", comment: "Title for the alert modal when a security key with PIN is inserted.")
    public static let pinPlaceholder = NSLocalizedString("pinPlaceholder", bundle: Bundle.shared, value: "Enter your PIN", comment: "Placeholder text for PIN")
    public static let confirmPin = NSLocalizedString("confirmPin", bundle: Bundle.shared, value: "Verify", comment: "Button text to confirm PIN")
    public static let verificationPending = NSLocalizedString("verificationPending", bundle: Bundle.shared, value: "Verifying your PIN for ", comment: "Title when PIN code is being verified.")
}

extension String {
    /// Encode a String to Base64
    func toBase64() -> String {
        return Data(self.utf8).base64EncodedString()
    }
}
