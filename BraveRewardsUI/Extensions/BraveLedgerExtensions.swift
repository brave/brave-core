/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveRewards
import Shared
import BraveShared

private let log = Logger.rewardsLogger

extension BraveLedger {
  
  // MARK: - Auto-Contribute Publisher Helpers
  
  var supportedPublishersFilter: ActivityInfoFilter {
    let sort = ActivityInfoFilterOrderPair().then {
      $0.propertyName = "percent"
      $0.ascending = false
    }
    let filter = ActivityInfoFilter().then {
      $0.id = ""
      $0.excluded = .filterAllExceptExcluded
      $0.percent = 1 //exclude 0% sites.
      $0.orderBy = [sort]
      $0.nonVerified = self.allowUnverifiedPublishers
      $0.reconcileStamp = self.autoContributeProperties.reconcileStamp
    }
    return filter
  }
  
  var excludedPublishersFilter: ActivityInfoFilter {
    return ActivityInfoFilter().then {
      $0.id = ""
      $0.excluded = .filterExcluded
      $0.nonVerified = self.allowUnverifiedPublishers
      $0.reconcileStamp = self.autoContributeProperties.reconcileStamp
    }
  }
  
  // MARK: - External Wallet Helpers
  
  var walletBalances: [WalletType: Double] {
    guard let wallets = balance?.wallets else { return [:] }
    var balances: [WalletType: Double] = [:]
    for wallet in wallets {
      balances[WalletType(rawValue: wallet.key)] = wallet.value.doubleValue
    }
    return balances
  }
  
  var walletContainsBraveFunds: Bool {
    let balances = walletBalances
    return (balances[.anonymous] ?? 0) > 0 || (balances[.unblindedTokens] ?? 0) > 0
  }
  
  var upholdWalletStatus: WalletStatus {
    guard let userWallet = externalWallets[.uphold] else { return .notConnected }
    return userWallet.status
  }
  
  /// The total balance or 0 if the balance hasn't been loaded yet
  fileprivate var balanceTotal: Double {
    return balance?.total ?? 0
  }
  
  // MARK: - Balance and Currency Helpers
  
  /// Get the current BAT wallet balance for display
  var balanceString: String { return BATValue(balanceTotal).displayString }
  
  /// Get the current USD wallet balance for display
  var usdBalanceString: String {
    return dollarStringForBATAmount(balanceTotal) ?? ""
  }
  
  /// Gets the dollar string for some BAT amount using rates from the users wallet with the
  /// currency code appended (i.e. "6.42 USD")
  func dollarStringForBATAmount(_ amount: Double, currencyCode: String = "USD", includeCurrencyCode: Bool = true) -> String? {
    guard let parameters = rewardsParameters else { return nil }
    let conversionRate = parameters.rate
    
    let currencyFormatter = NumberFormatter()
    currencyFormatter.currencySymbol = ""
    currencyFormatter.numberStyle = .currency
    currencyFormatter.locale = Locale(identifier: "en_US")
    let valueString = currencyFormatter.string(from: NSNumber(value: amount * conversionRate)) ?? "0.00"
    if includeCurrencyCode {
      return "\(valueString) \(currencyCode)"
    }
    return "\(valueString)"
  }
  
  /// Takes BAT amount as String, and returns a String converted to selected currency.
  /// Returns '0.00' if the method failed or could not cast `amountString` as `Double`
  func dollarStringForBATAmount(_ amountString: String, currencyCode: String = "USD", includeCurrencyCode: Bool = true) -> String {
    let stringToDouble = Double(amountString) ?? 0.0
    guard let string = dollarStringForBATAmount(stringToDouble, currencyCode: currencyCode, includeCurrencyCode: includeCurrencyCode) else {
      if includeCurrencyCode {
        return "0.00 USD"
      }
      return "0.00"
    }
    return string
  }
  
  /// Options around minimum visits for publisher relavancy
  enum MinimumVisitsOptions: Int32, CaseIterable, DisplayableOption {
    case one = 1
    case five = 5
    case ten = 10
    
    var displayString: String {
      switch self {
      case .one: return Strings.minimumVisitsChoices0
      case .five: return Strings.minimumVisitsChoices1
      case .ten: return Strings.minimumVisitsChoices2
      }
    }
  }
  
  /// Options around minimum page time before logging a visit (in seconds)
  enum MinimumVisitDurationOptions: Int32, CaseIterable, DisplayableOption {
    case fiveSeconds = 5
    case eightSeconds = 8
    case oneMinute = 60
    
    var displayString: String {
      switch self {
      case .fiveSeconds: return Strings.minimumLengthChoices0
      case .eightSeconds: return Strings.minimumLengthChoices1
      case .oneMinute: return Strings.minimumLengthChoices2
      }
    }
  }
  
  // MARK: -
  
  /// Creates the ledger wallet and fetches wallet properties and balances
  ///
  /// Use this is in UI instead of `createWallet` directly unless required
  public func createWalletAndFetchDetails(_ completion: @escaping (Bool) -> Void) {
    createWallet { [weak self] (error) in
      guard let self = self else { return }
      
      if let _ = error {
        completion(false)
        return
      }
      
      let group = DispatchGroup()
      var success = true
      group.enter()
      self.getRewardsParameters { details in
        if details == nil {
          success = false
        }
        group.leave()
      }
      group.enter()
      self.fetchBalance { balance in
        if balance == nil {
          success = false
        }
        group.leave()
      }
      group.notify(queue: .main, execute: {
        completion(success)
      })
    }
  }
  
  var paymentId: String? {
    var id: String?
    rewardsInternalInfo { info in
      id = info?.paymentId
    }
    return id
  }
  
  public func setupDeviceCheckEnrollment(_ client: DeviceCheckClient, completion: @escaping () -> Void) {
    // Enroll in DeviceCheck
    client.generateToken { [weak self] (token, error) in
      guard let self = self else { return }
      if let error = error {
        log.error("Failed to generate DeviceCheck token: \(error)")
        completion()
        return
      }
      let paymentId = self.paymentId ?? ""
      client.generateEnrollment(paymentId: paymentId, token: token) { registration, error in
        if let error = error {
          log.error("Failed to enroll in DeviceCheck: \(error)")
          completion()
          return
        }
        guard let registration = registration else { return }
        client.registerDevice(enrollment: registration) { error in
          if let error = error {
            log.error("Failed to register device with mobile attestation server: \(error)")
            completion()
            return
          }
        }
      }
    }
  }
  
  public func claimPromotion(_ promotion: Promotion, completion: @escaping (_ success: Bool) -> Void) {
    guard let paymentId = self.paymentId else {
      completion(false)
      return
    }
    let deviceCheck = DeviceCheckClient(environment: BraveLedger.environment)
    let group = DispatchGroup()
    if !DeviceCheckClient.isDeviceEnrolled() {
      group.enter()
      setupDeviceCheckEnrollment(deviceCheck) {
        if !DeviceCheckClient.isDeviceEnrolled() {
          DispatchQueue.main.async {
            completion(false)
          }
          return
        }
        group.leave()
      }
    }
    group.notify(queue: .main) {
      deviceCheck.generateAttestation(paymentId: paymentId) { (attestation, error) in
        guard let attestation = attestation else {
          completion(false)
          return
        }
        self.claimPromotion(promotion.id, publicKey: attestation.publicKeyHash) { result, nonce in
          if result != .ledgerOk {
            completion(false)
            return
          }
          
          deviceCheck.generateAttestationVerification(nonce: nonce) { verification, error in
            guard let verification = verification else {
              completion(false)
              return
            }
            
            let solution = PromotionSolution()
            solution.nonce = nonce
            solution.signature = verification.signature
            do {
              solution.blob = try verification.attestationBlob.bsonData().base64EncodedString()
            } catch {
              log.error("Couldn't serialize attestation blob. The attest promotion will fail")
            }
            
            self.attestPromotion(promotion.id, solution: solution) { result, promotion in
              if result == .ledgerOk {
                if promotion?.type == .ads {
                  MonthlyAdsGrantReminder.cancelCurrentMonth()
                }
                self.updatePendingAndFinishedPromotions {
                  completion(true)
                }
              } else {
                completion(false)
              }
            }
          }
        }
      }
    }
  }
}

extension PublisherInfo {
  /// Temporary conveninece method for converting the raw int now in
  /// PublisherInfo, to the enum we've been using all along
  var rewardsCategory: RewardsType {
    guard let category = RewardsType(rawValue: Int(self.category)) else {
      fatalError("Could not get a rewards category from category = \(self.category)")
    }
    return category
  }
}

extension RewardsType {
  var displayString: String {
    switch self {
    case .autoContribute:
      return Strings.settingsAutoContributeTitle
    case .oneTimeTip:
      return Strings.settingsTipsTitle
    case .recurringTip:
      return Strings.settingsMonthlyTipsTitle
    @unknown default:
      return ""
    }
  }
}
