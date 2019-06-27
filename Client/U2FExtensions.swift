// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Data
import Shared
import WebKit

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
}

private enum U2FMessageType: String {
    case FIDO2Create = "fido2-create"
    case FIDO2Get = "fido2-get"
    case FIDORegister = "fido-register"
    case FIDOSign = "fido-sign"
    case FIDOLowLevel = "fido-low-level"
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
    
    // Using a property style approch to avoid observing twice.
    private var observeKeyUpdates: Bool = false {
        didSet {
            if oldValue == observeKeyUpdates {
                return
            }
            
            let keySession = YubiKitManager.shared.keySession as AnyObject
            
            if observeKeyUpdates {
                keySession.addObserver(self, forKeyPath: #keyPath(YKFKeySession.sessionState), options: [.new, .old], context: &U2FExtensions.observationContext)
            } else {
                keySession.removeObserver(self, forKeyPath: #keyPath(YKFKeySession.sessionState))
            }
        }
    }

    init(tab: Tab) {
        self.tab = tab
        defer {
            observeKeyUpdates = true
        }
        super.init()
        
        // Make sure the session is started
        YubiKitManager.shared.keySession.startSession()
    }
    
    deinit {
        observeKeyUpdates = false
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
        guard let url = self.tab?.webView?.url else {
            return nil
        }
        return url.domainURL.absoluteString
    }
    
    override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
        guard context == &U2FExtensions.observationContext else {
            super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
            return
        }
        
        if keyPath == #keyPath(YKFKeySession.sessionState) {
            ensureMainThread {
                self.handleSessionStateChange()
            }
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
            
            let userVerification = publicKey.authenticatorSelection?.userVerification ?? ""
            let makeOptions = [
                YKFKeyFIDO2MakeCredentialRequestOptionRK: publicKey.authenticatorSelection?.requireResidentKey ?? false,
                YKFKeyFIDO2MakeCredentialRequestOptionUV: userVerification == "required"
            ]
            makeCredentialRequest.options = makeOptions
            
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
                    let errorDescription = error?.localizedDescription ?? Strings.U2FRegistrationError
                    self.sendFIDO2RegistrationError(handle: handle, errorDescription: errorDescription)
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
    
    private func sendFIDO2RegistrationError(handle: Int, errorName: String = FIDO2ErrorMessages.NotAllowedError.rawValue, errorDescription: String = Strings.U2FRegistrationError) {
        cleanupFIDO2Registration(handle: handle)
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("navigator.credentials.postCreate('\(handle)', \(true),'', '', '', '\(errorName)', '\(errorDescription)')", completionHandler: { _, error in
                if error != nil {
                    let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorRegistration.rawValue
                    log.error(errorDescription)
                }
        }) }
    }
    
    private func cleanupFIDO2Registration(handle: Int) {
        guard let index = fido2RegHandles.firstIndex(of: handle) else {
            log.error(U2FErrorMessages.ErrorRegistration)
            return
        }
        fido2RegHandles.remove(at: index)
        fido2RegisterRequest.removeValue(forKey: handle)
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
        let getAssertionRequest = YKFKeyFIDO2GetAssertionRequest()
        guard let url = getCurrentURL() else {
            sendFIDO2AuthenticationError(handle: handle, errorName: FIDO2ErrorMessages.SecurityError.rawValue)
            return
        }
        
        let clientData = WebAuthnClientData(type: WebAuthnClientDataType.get.rawValue, challenge: request.challenge, origin: url)
        do {
            let clientDataJSON = try JSONEncoder().encode(clientData)
            guard let allowCredential = request.allowCredentials.first else {
                sendFIDO2AuthenticationError(handle: handle)
                return
            }
            let requestId = allowCredential
            
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
            
            // userPresence is set to the inverse of userVerification
            // https://www.w3.org/TR/webauthn/#user-verification
            getAssertionRequest.options = [
                YKFKeyFIDO2GetAssertionRequestOptionUP: !request.userVerification,
                YKFKeyFIDO2GetAssertionRequestOptionUV: request.userVerification
            ]
            
            var allowList = [YKFFIDO2PublicKeyCredentialDescriptor]()
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
                    let errorDescription = error?.localizedDescription ?? Strings.U2FAuthenticationError
                    self.sendFIDO2AuthenticationError(handle: handle, errorDescription: errorDescription)
                    return
                }
                
                // The reponse from the key must not be empty at this point.
                guard let response = response else {
                    self.sendFIDO2AuthenticationError(handle: handle)
                    return
                }
                self.finalizeFIDO2Authentication(handle: handle, response: response, requestId: requestId, clientDataJSON: clientDataJSON, error: nil)
            }
        } catch {
            sendFIDO2AuthenticationError(handle: handle, errorDescription: error.localizedDescription)
        }
    }
    
    private func finalizeFIDO2Authentication(handle: Int, response: YKFKeyFIDO2GetAssertionResponse, requestId: String, clientDataJSON: Data, error: NSErrorPointer) {
        guard error == nil else {
            let errorDescription = error?.pointee?.localizedDescription ?? Strings.U2FAuthenticationError
            sendFIDO2AuthenticationError(handle: handle, errorDescription: errorDescription)
            return
        }
        
        let authenticatorData = response.authData.base64EncodedString()
        let clientDataJSONString = clientDataJSON.base64EncodedString()
        let sig = response.signature.base64EncodedString()
        
        cleanupFIDO2Authentication(handle: handle)
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("navigator.credentials.postGet('\(handle)', \(true), '\(requestId)', '\(authenticatorData)', '\(clientDataJSONString)', '\(sig)', '')", completionHandler: { _, error in
                if error != nil {
                    let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorAuthentication.rawValue
                    log.error(errorDescription)
                }
        }) }
    }
    
    private func sendFIDO2AuthenticationError(handle: Int, errorName: String = FIDO2ErrorMessages.NotAllowedError.rawValue, errorDescription: String = Strings.U2FAuthenticationError) {
        cleanupFIDO2Authentication(handle: handle)
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("navigator.credentials.postGet('\(handle)', \(true), '', '', '', '', '\(errorName)', '\(errorDescription)')", completionHandler: { _, error in
                if error != nil {
                    let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorAuthentication.rawValue
                    log.error(errorDescription)
                }
        }) }
    }
    
    private func cleanupFIDO2Authentication(handle: Int) {
        guard let index = fido2AuthHandles.firstIndex(of: handle) else {
            log.error(U2FErrorMessages.ErrorRegistration)
            return
        }
        fido2AuthHandles.remove(at: index)
        fido2AuthRequest.removeValue(forKey: handle)
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
                self.tab?.webView?.evaluateJavaScript("u2f.postLowLevelRegister(\(requestId), \(true), '', '', '', \(errorCode.rawValue), '\(errorMessage)')", completionHandler: { _, error in
                    if error != nil {
                        let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorRegistration.rawValue
                        log.error(errorDescription)
                    }
            }) }
            return
        }
        
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("u2f.postRegister('\(handle)', \(true), '', '', '', \(errorCode.rawValue), '\(errorMessage)')", completionHandler: { _, error in
                if error != nil {
                    let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorRegistration.rawValue
                    log.error(errorDescription)
                }
            }) }
    }
    
    private func cleanupFIDORegistration(handle: Int) {
        guard let index = fidoRegHandles.firstIndex(of: handle) else {
            log.error(U2FErrorMessages.ErrorRegistration)
            return
        }
        fidoRegHandles.remove(at: index)
        fidoRegisterRequest.removeValue(forKey: handle)
        requestId.removeValue(forKey: handle)
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
                self.tab?.webView?.evaluateJavaScript("u2f.postLowLevelSign(\(requestId), \(true), '', '', '', \(errorCode.rawValue), '\(errorMessage)')", completionHandler: { _, error in
                    if error != nil {
                        let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorAuthentication.rawValue
                        log.error(errorDescription)
                    }
            }) }
            return
        }
        
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("u2f.postSign('\(handle)', \(true), '', '', '', \(errorCode.rawValue), '\(errorMessage)')", completionHandler: { _, error in
                if error != nil {
                    let errorDescription = error?.localizedDescription ?? U2FErrorMessages.ErrorAuthentication.rawValue
                    log.error(errorDescription)
                }
        }) }
    }
    
    private func cleanupFIDOAuthentication(handle: Int) {
        guard let index = fidoSignHandles.firstIndex(of: handle) else {
            log.error(U2FErrorMessages.ErrorRegistration)
            return
        }
        fidoSignHandles.remove(at: index)
        fidoSignRequests.removeValue(forKey: handle)
        requestId.removeValue(forKey: handle)
    }
    
    private func handleSessionStateChange() {
        observeKeyUpdates = false
        defer {
            observeKeyUpdates = true
        }
        let sessionState = YubiKitManager.shared.keySession.sessionState
        if sessionState == .open { // The key session is ready to be used.
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
}
