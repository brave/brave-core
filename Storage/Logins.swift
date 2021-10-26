/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import WebKit
import Shared
import XCGLogger

private var log = Logger.syncLogger

/**
 * LoginData is a wrapper around NSURLCredential and NSURLProtectionSpace to allow us to add extra fields where needed.
 **/
public protocol LoginData: AnyObject {
    var guid: String { get set }                 // It'd be nice if this were read-only.
    var credentials: URLCredential { get }
    var protectionSpace: URLProtectionSpace { get }
    var hostname: String { get }
    var username: String? { get }
    var password: String { get }
    var httpRealm: String? { get set }
    var formSubmitURL: String? { get set }
    var usernameField: String? { get set }
    var passwordField: String? { get set }
    var isValid: Maybe<()> { get }

    // https://bugzilla.mozilla.org/show_bug.cgi?id=1238103
    var hasMalformedHostname: Bool { get set }

    func toDict() -> [String: String]

    func isSignificantlyDifferentFrom(_ login: LoginData) -> Bool
    
    func update(password: String, username: String)
}

public protocol LoginUsageData {
    var timesUsed: Int { get set }
    var timeCreated: MicrosecondTimestamp { get set }
    var timeLastUsed: MicrosecondTimestamp { get set }
    var timePasswordChanged: MicrosecondTimestamp { get set }
}

open class Login: CustomStringConvertible, LoginData, LoginUsageData, Equatable {
    open var guid: String

    open fileprivate(set) var credentials: URLCredential
    public let protectionSpace: URLProtectionSpace

    open var hostname: String {
        if let _ = protectionSpace.`protocol` {
            return protectionSpace.urlString()
        }
        return protectionSpace.host
    }

    open var hasMalformedHostname: Bool = false

    open var username: String? { return credentials.user }
    open var password: String { return credentials.password ?? "" }
    open var usernameField: String?
    open var passwordField: String?

    fileprivate var _httpRealm: String?
    open var httpRealm: String? {
        get { return self._httpRealm ?? protectionSpace.realm }
        set { self._httpRealm = newValue }
    }

    fileprivate var _formSubmitURL: String?
    open var formSubmitURL: String? {
        get {
            return self._formSubmitURL
        }
        set(value) {
            guard let value = value, !value.isEmpty else {
                self._formSubmitURL = nil
                return
            }

            let url2 = URL(string: self.hostname)
            let url1 = URL(string: value)

            if url1?.host != url2?.host {
                log.warning("Form submit URL domain doesn't match login's domain.")
            }

            self._formSubmitURL = value
        }
    }

    // LoginUsageData. These defaults only apply to locally created records.
    open var timesUsed = 0
    open var timeCreated = Date.nowMicroseconds()
    open var timeLastUsed = Date.nowMicroseconds()
    open var timePasswordChanged = Date.nowMicroseconds()

    // Printable
    open var description: String {
        return "Login for \(hostname)"
    }

    open var isValid: Maybe<()> {
        // Referenced from https://mxr.mozilla.org/mozilla-central/source/toolkit/components/passwordmgr/nsLoginManager.js?rev=f76692f0fcf8&mark=280-281#271

        // Logins with empty hostnames are not valid.
        if hostname.isEmpty {
            return Maybe(failure: LoginDataError(description: "Can't add a login with an empty hostname."))
        }

        // Logins with empty passwords are not valid.
        if password.isEmpty {
            return Maybe(failure: LoginDataError(description: "Can't add a login with an empty password."))
        }

        // Logins with both a formSubmitURL and httpRealm are not valid.
        if let _ = formSubmitURL, let _ = httpRealm {
            return Maybe(failure: LoginDataError(description: "Can't add a login with both a httpRealm and formSubmitURL."))
        }

        // Login must have at least a formSubmitURL or httpRealm.
        if (formSubmitURL == nil) && (httpRealm == nil) {
            return Maybe(failure: LoginDataError(description: "Can't add a login without a httpRealm or formSubmitURL."))
        }

        // All good.
        return Maybe(success: ())
    }

    open func update(password: String, username: String) {
        self.credentials =
            URLCredential(user: username, password: password, persistence: credentials.persistence)
    }

    // Essentially: should we sync a change?
    // Desktop ignores usernameField and hostnameField.
    open func isSignificantlyDifferentFrom(_ login: LoginData) -> Bool {
        return login.password != self.password ||
               login.hostname != self.hostname ||
               login.username != self.username ||
               login.formSubmitURL != self.formSubmitURL ||
               login.httpRealm != self.httpRealm
    }

    /* Used for testing purposes since formSubmitURL should be given back to use from the Logins.js script */
    open class func createWithHostname(_ hostname: String, username: String, password: String, formSubmitURL: String?) -> LoginData {
        let loginData = Login(hostname: hostname, username: username, password: password) as LoginData
        loginData.formSubmitURL = formSubmitURL
        return loginData
    }

    open class func createWithHostname(_ hostname: String, username: String, password: String) -> LoginData {
        return Login(hostname: hostname, username: username, password: password) as LoginData
    }

    open class func createWithCredential(_ credential: URLCredential, protectionSpace: URLProtectionSpace) -> LoginData {
        return Login(credential: credential, protectionSpace: protectionSpace) as LoginData
    }

    public init(guid: String, hostname: String, username: String, password: String) {
        self.guid = guid
        self.credentials = URLCredential(user: username, password: password, persistence: .none)

        // Break down the full url hostname into its scheme/protocol and host components
        let hostnameURL = hostname.asURL
        let host = hostnameURL?.host ?? hostname
        let scheme = hostnameURL?.scheme ?? ""

        // We should ignore any SSL or normal web ports in the URL.
        var port = hostnameURL?.port ?? 0
        if port == 443 || port == 80 {
            port = 0
        }

        self.protectionSpace = URLProtectionSpace(host: host, port: port, protocol: scheme, realm: nil, authenticationMethod: nil)
    }

    convenience init(hostname: String, username: String, password: String) {
        self.init(guid: Bytes.generateGUID(), hostname: hostname, username: username, password: password)
    }

    // Why do we need this initializer to be marked as required? Because otherwise we can't
    // use this type in our factory for MirrorLogin and LocalLogin.
    // SO: http://stackoverflow.com/questions/26280176/swift-generics-not-preserving-type
    // Playground: https://gist.github.com/rnewman/3fb0c4dbd25e7fda7e3d
    // Conversation: https://twitter.com/rnewman/status/611332618412359680
    required public init(credential: URLCredential, protectionSpace: URLProtectionSpace) {
        self.guid = Bytes.generateGUID()
        self.credentials = credential
        self.protectionSpace = protectionSpace
    }

    open func toDict() -> [String: String] {
        return [
            "hostname": hostname,
            "formSubmitURL": formSubmitURL ?? "",
            "httpRealm": httpRealm ?? "",
            "username": username ?? "",
            "password": password,
            "usernameField": usernameField ?? "",
            "passwordField": passwordField ?? ""
        ]
    }

    open class func fromScript(_ url: URL, script: [String: Any]) -> LoginData? {
        guard let username = script["username"] as? String,
              let password = script["password"] as? String else {
                return nil
        }

        guard let origin = getPasswordOrigin(url.absoluteString) else {
            return nil
        }

        let login = Login(hostname: origin, username: username, password: password)

        if let formSubmit = script["formSubmitURL"] as? String {
            login.formSubmitURL = formSubmit
        }

        if let passwordField = script["passwordField"] as? String {
            login.passwordField = passwordField
        }

        if let userField = script["usernameField"] as? String {
            login.usernameField = userField
        }

        return login as LoginData
    }

    fileprivate class func getPasswordOrigin(_ uriString: String, allowJS: Bool = false) -> String? {
        var realm: String?
        if let uri = URL(string: uriString),
            let scheme = uri.scheme, !scheme.isEmpty,
            let host = uri.host {
            if allowJS && scheme == "javascript" {
                return "javascript:"
            }

            realm = "\(scheme)://\(host)"

            // If the URI explicitly specified a port, only include it when
            // it's not the default. (We never want "http://foo.com:80")
            if let port = uri.port {
                realm? += ":\(port)"
            }
        } else {
            // bug 159484 - disallow url types that don't support a hostPort.
            // (although we handle "javascript:..." as a special case above.)
            log.debug("Couldn't parse origin for \(uriString)")
            realm = nil
        }
        return realm
    }
}

public func ==(lhs: Login, rhs: Login) -> Bool {
    return lhs.credentials == rhs.credentials && lhs.protectionSpace == rhs.protectionSpace
}

public protocol BrowserLogins {
    func getUsageDataForLoginByGUID(_ guid: GUID) -> Deferred<Maybe<LoginUsageData>>
    func getLoginDataForGUID(_ guid: GUID) -> Deferred<Maybe<Login>>
    func getLoginsForProtectionSpace(_ protectionSpace: URLProtectionSpace) -> Deferred<Maybe<Cursor<LoginData>>>
    func getLoginsForProtectionSpace(_ protectionSpace: URLProtectionSpace, withUsername username: String?) -> Deferred<Maybe<Cursor<LoginData>>>
    func getAllLogins() -> Deferred<Maybe<Cursor<Login>>>
    func getLoginsForQuery(_ query: String) -> Deferred<Maybe<Cursor<Login>>>
    
    func searchLoginsWithQuery(_ query: String?) -> Deferred<Maybe<Cursor<Login>>>

    // Add a new login regardless of whether other logins might match some fields. Callers
    // are responsible for querying first if they care.
    @discardableResult func addLogin(_ login: LoginData) -> Success

    @discardableResult func updateLoginByGUID(_ guid: GUID, new: LoginData, significant: Bool) -> Success

    // Add the use of a login by GUID.
    @discardableResult func addUseOfLoginByGUID(_ guid: GUID) -> Success
    func removeLoginByGUID(_ guid: GUID) -> Success
    func removeLoginsWithGUIDs(_ guids: [GUID]) -> Success

    func removeAll() -> Success
}

open class LoginDataError: MaybeErrorType {
    public let description: String
    public init(description: String) {
        self.description = description
    }
}
