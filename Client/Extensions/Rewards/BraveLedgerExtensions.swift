// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Shared
import BraveShared

private let log = Logger.braveCoreLogger

extension BraveLedger {
    
    public var isLedgerTransferExpired: Bool {
        if Locale.current.regionCode != "JP" {
            return false
        }
        let now = Date()
        let deadlineComponents = DateComponents(year: 2021, month: 3, day: 13)
        guard let deadlineDate = Calendar(identifier: .gregorian).nextDate(
            after: now,
            matching: deadlineComponents,
            matchingPolicy: .strict
        ) else {
            return true
        }
        return now >= deadlineDate
    }
    
    public func listAutoContributePublishers(_ completion: @escaping (_ publishers: [Ledger.PublisherInfo]) -> Void) {
        let filter: Ledger.ActivityInfoFilter = {
            let sort = Ledger.ActivityInfoFilterOrderPair().then {
                $0.propertyName = "percent"
                $0.ascending = false
            }
            let filter = Ledger.ActivityInfoFilter().then {
                $0.id = ""
                $0.excluded = .filterAllExceptExcluded
                $0.percent = 1 // exclude 0% sites.
                $0.orderBy = [sort]
                $0.nonVerified = allowUnverifiedPublishers
                $0.reconcileStamp = autoContributeProperties.reconcileStamp
            }
            return filter
        }()
        listActivityInfo(fromStart: 0, limit: 0, filter: filter) { list in
            completion(list)
        }
    }
    
    public func updateDrainStatus(_ completion: @escaping (Ledger.DrainStatus?) -> Void) {
        guard let drainID = Preferences.Rewards.transferDrainID.value else {
            completion(nil)
            return
        }
        if !AppConstants.buildChannel.isPublic,
           let overrideValue = Preferences.Rewards.drainStatusOverride.value,
           let status = Ledger.DrainStatus(rawValue: overrideValue) {
            Preferences.Rewards.lastTransferStatus.value = status.rawValue
            completion(status)
            return
        }
        drainStatus(for: drainID) { result, status in
            if result != .ledgerOk {
                if let lastStatus = Preferences.Rewards.lastTransferStatus.value {
                    completion(Ledger.DrainStatus(rawValue: lastStatus))
                } else {
                    completion(nil)
                }
                return
            }
            Preferences.Rewards.lastTransferStatus.value = status.rawValue
            completion(status)
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
            
            self.minimumVisitDuration = 8
            self.minimumNumberOfVisits = 1
            self.allowVideoContributions = true
            self.allowUnverifiedPublishers = false
            self.contributionAmount = Double.greatestFiniteMagnitude
            
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
    
    public func claimPromotion(_ promotion: Ledger.Promotion, completion: @escaping (_ success: Bool, _ shouldReconcileAds: Bool) -> Void) {
        guard let paymentId = self.paymentId else {
            completion(false, false)
            return
        }
        let deviceCheck = DeviceCheckClient(environment: BraveLedger.environment)
        let group = DispatchGroup()
        if !DeviceCheckClient.isDeviceEnrolled() {
            group.enter()
            setupDeviceCheckEnrollment(deviceCheck) {
                if !DeviceCheckClient.isDeviceEnrolled() {
                    DispatchQueue.main.async {
                        completion(false, false)
                    }
                    return
                }
                group.leave()
            }
        }
        group.notify(queue: .main) {
            deviceCheck.generateAttestation(paymentId: paymentId) { (attestation, error) in
                guard let attestation = attestation else {
                    completion(false, false)
                    return
                }
                self.claimPromotion(promotion.id, publicKey: attestation.publicKeyHash) { result, nonce in
                    if result != .ledgerOk {
                        completion(false, false)
                        return
                    }
                    
                    deviceCheck.generateAttestationVerification(nonce: nonce) { verification, error in
                        guard let verification = verification else {
                            completion(false, false)
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
                                self.updatePendingAndFinishedPromotions { shouldReconcileAds in
                                    completion(true, shouldReconcileAds)
                                }
                            } else {
                                completion(false, false)
                            }
                        }
                    }
                }
            }
        }
    }
}
