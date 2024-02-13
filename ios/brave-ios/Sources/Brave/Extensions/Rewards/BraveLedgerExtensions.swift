// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Shared
import BraveShared
import os.log

extension BraveRewardsAPI {

  public var isLedgerTransferExpired: Bool {
    if Locale.current.regionCode != "JP" {
      return false
    }
    let now = Date()
    let deadlineComponents = DateComponents(year: 2021, month: 3, day: 13)
    guard
      let deadlineDate = Calendar(identifier: .gregorian).nextDate(
        after: now,
        matching: deadlineComponents,
        matchingPolicy: .strict
      )
    else {
      return true
    }
    return now >= deadlineDate
  }

  public func listAutoContributePublishers(_ completion: @escaping (_ publishers: [BraveCore.BraveRewards.PublisherInfo]) -> Void) {
    fetchAutoContributeProperties { autoContributeProperties in
      let filter: BraveCore.BraveRewards.ActivityInfoFilter = {
        let sort = BraveCore.BraveRewards.ActivityInfoFilterOrderPair().then {
          $0.propertyName = "percent"
          $0.ascending = false
        }
        let filter = BraveCore.BraveRewards.ActivityInfoFilter().then {
          $0.id = ""
          $0.excluded = .filterAllExceptExcluded
          $0.percent = 1  // exclude 0% sites.
          $0.orderBy = [sort]
          $0.nonVerified = false
          $0.reconcileStamp = autoContributeProperties?.reconcileStamp ?? 0
        }
        return filter
      }()
      self.listActivityInfo(fromStart: 0, limit: 0, filter: filter) { list in
        completion(list)
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

      self.setMinimumVisitDuration(8)
      self.setMinimumNumberOfVisits(1)
      self.setContributionAmount(Double.greatestFiniteMagnitude)

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
      group.notify(
        queue: .main,
        execute: {
          completion(success)
        })
    }
  }
  
  @MainActor private func fetchPaymentId() async -> String? {
    await withCheckedContinuation { c in
      rewardsInternalInfo { info in
        c.resume(returning: info?.paymentId)
      }
    }
  }
  
  public func setupDeviceCheckEnrollment(_ client: DeviceCheckClient, completion: @escaping () -> Void) {
    // Enroll in DeviceCheck
    client.generateToken { [weak self] (token, error) in
      guard let self = self else { return }
      if let error = error {
        Logger.module.error("Failed to generate DeviceCheck token: \(error.localizedDescription)")
        completion()
        return
      }
      Task { @MainActor in
        do {
          guard let paymentId = await self.fetchPaymentId(), !paymentId.isEmpty else {
            // No wallet to register
            completion()
            return
          }
          let registration = try client.generateEnrollment(paymentId: paymentId, token: token)
          try await client.registerDevice(enrollment: registration)
        } catch {
          Logger.module.error("Failed to register device with mobile attestation server: \(error.localizedDescription)")
        }
        completion()
      }
    }
  }

  @MainActor public func claimPromotion(_ promotion: BraveCore.BraveRewards.Promotion) async -> Bool {
    guard let paymentId = await fetchPaymentId() else {
      return false
    }
    let deviceCheck = DeviceCheckClient()
    if !DeviceCheckClient.isDeviceEnrolled() {
      let didEnroll: Bool = await withCheckedContinuation { c in
        setupDeviceCheckEnrollment(deviceCheck) {
          c.resume(returning: DeviceCheckClient.isDeviceEnrolled())
        }
      }
      if !didEnroll {
        Logger.module.error("Cannot solve adaptive captcha as user is not enrolled with the attestation server")
        return false
      }
    }
    guard let attestation = try? deviceCheck.generateAttestation(paymentId: paymentId) else {
      return false
    }
    return await withCheckedContinuation { c in
      self.claimPromotion(promotion.id, publicKey: attestation.publicKeyHash) { result, nonce in
        if result != .ok {
          c.resume(returning: false)
          return
        }
        do {
          let verification = try deviceCheck.generateAttestationVerification(nonce: nonce)
          let solution = PromotionSolution()
          solution.nonce = nonce
          solution.signature = verification.signature
          solution.blob = try verification.attestationBlob.bsonData().base64EncodedString()
          
          self.attestPromotion(promotion.id, solution: solution) { result, promotion in
            if result == .ok {
              self.updatePendingAndFinishedPromotions {
                c.resume(returning: true)
              }
            } else {
              c.resume(returning: false)
            }
          }
        } catch {
          c.resume(returning: false)
        }
      }
    }
  }
  
  func solveAdaptiveCaptcha(paymentId: String, captchaId: String) async throws {
    let deviceCheck = DeviceCheckClient()
    if !DeviceCheckClient.isDeviceEnrolled() {
      let didEnroll: Bool = await withCheckedContinuation { c in
        setupDeviceCheckEnrollment(deviceCheck) {
          c.resume(returning: DeviceCheckClient.isDeviceEnrolled())
        }
      }
      if !didEnroll {
        Logger.module.error("Cannot solve adaptive captcha as user is not enrolled with the attestation server")
        return
      }
    }
    
    let attestation = try deviceCheck.generateAttestation(paymentId: paymentId)
    let blob = try await deviceCheck.getAttestation(attestation: attestation)
    let verification = try deviceCheck.generateAttestationVerification(nonce: blob.nonce)
    try await deviceCheck.setAttestation(nonce: blob.nonce, verification: verification)
    try await deviceCheck.solveAdaptiveCaptcha(paymentId: paymentId, captchaId: captchaId, verification: verification)
  }
}
