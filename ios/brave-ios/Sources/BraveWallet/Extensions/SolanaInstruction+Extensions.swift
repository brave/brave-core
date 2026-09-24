// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

extension BraveWallet.SolanaInstruction {

  var isSystemProgram: Bool {
    programId == BraveWallet.SolanaSystemProgramId
  }

  var isTokenProgram: Bool {
    programId == BraveWallet.SolanaTokenProgramId
  }

  var isAssociatedTokenProgram: Bool {
    programId == BraveWallet.SolanaAssociatedTokenProgramId
  }

  var isSysvarRentProgram: Bool {
    programId == BraveWallet.SolanaSysvarRentProgramId
  }

  var instructionTypeName: String {
    guard let decodedData = self.decodedData else {
      return Strings.Wallet.solanaUnknownInstructionName
    }
    let instructionType = BraveWallet.SolanaSystemInstruction(
      rawValue: Int(decodedData.instructionType)
    )
    return instructionType?.name ?? Strings.Wallet.solanaUnknownInstructionName
  }

  var instructionName: String {
    guard let decodedData = self.decodedData else {
      return Strings.Wallet.solanaUnknownInstructionName
    }
    if isSystemProgram,
      let instructionType = BraveWallet.SolanaSystemInstruction(
        rawValue: Int(decodedData.instructionType)
      )
    {
      let name = instructionType.name
      return String.localizedStringWithFormat(Strings.Wallet.solanaSystemProgramName, name)
    } else if isTokenProgram,
      let instructionType = BraveWallet.SolanaTokenInstruction(
        rawValue: Int(decodedData.instructionType)
      )
    {
      let name = instructionType.name
      return String.localizedStringWithFormat(Strings.Wallet.solanaTokenProgramName, name)
    }
    return Strings.Wallet.solanaUnknownInstructionName
  }

  /// Returns the `to_account` pubkey for the instruction if available
  var toPubkey: String? {
    guard
      let index = decodedData?.accountParams.firstIndex(
        where: { $0.name == BraveWallet.ToAccount }
      )
    else { return nil }

    // Do not show account public key of address table lookup account because
    // it might give the wrong impression to users. For example, users might
    // think they're sending funds to this address table lookup account, but
    // actually they're sending to the account pointed by the index in this
    // address table lookup account.
    let isAddrTableLookupAccount = accountMetas[safe: index]?.addrTableLookupIndex != nil
    if isAddrTableLookupAccount {
      return nil
    }

    return accountMetas[safe: index]?.pubkey
  }

  /// Returns the `from_account` pubkey for the instruction if available
  var fromPubkey: String? {
    guard
      let index = decodedData?.accountParams.firstIndex(
        where: { $0.name == BraveWallet.FromAccount }
      )
    else { return nil }

    // Do not show account public key of address table lookup account because
    // it might give the wrong impression to users. For example, users might
    // think they're sending funds to this address table lookup account, but
    // actually they're sending to the account pointed by the index in this
    // address table lookup account.
    let isAddrTableLookupAccount = accountMetas[safe: index]?.addrTableLookupIndex != nil
    if isAddrTableLookupAccount {
      return nil
    }

    return accountMetas[safe: index]?.pubkey
  }

  /// Generates the `KeyValue` pairs for the accounts in the `SolanaInstruction`.
  var accountKeyValues: [SolanaTxDetails.ParsedSolanaInstruction.KeyValue] {
    guard let decodedData else {
      // Unknown instruction type.
      let addrTableLookupIndexValues = accountMetas.compactMap(\.addrTableLookupIndexValue)
      if !addrTableLookupIndexValues.isEmpty {
        // if the 'to account' is an Address Lookup Table Account, we should display as:
        // Accounts:
        // Address Lookup Table Account: <account pubkey>
        // Address Lookup Table Index: <index in address lookup table>
        // ...
        return [
          SolanaTxDetails.ParsedSolanaInstruction.KeyValue(
            key: Strings.Wallet.solanaInstructionAccounts,
            value: addrTableLookupIndexValues.joined(separator: "\n")
          )
        ]
      }
      // Otherwise display as:
      // Accounts:
      // <account pubkey>
      // ...
      let accountPubkeys = accountMetas.map(\.pubkey)
      return [
        SolanaTxDetails.ParsedSolanaInstruction.KeyValue(
          key: Strings.Wallet.solanaInstructionAccounts,
          value: accountPubkeys.isEmpty ? "[]" : accountPubkeys.joined(separator: "\n")
        )
      ]
    }
    return decodedData.accountParams.enumerated().compactMap {
      (index, param) -> SolanaTxDetails.ParsedSolanaInstruction.KeyValue? in
      if param.name == BraveWallet.Signers {  // special case
        // the signers are the `accountMetas` from this index to the end of the array
        // its possible to have any number of signers, including 0
        if accountMetas[safe: index] != nil {
          let signers = accountMetas[index...].map(\.pubkey)
            .map { pubkey in "\(pubkey)" }
            .joined(separator: "\n")
          return SolanaTxDetails.ParsedSolanaInstruction.KeyValue(
            key: param.localizedName,
            value: signers
          )
        } else {
          return nil  // no signers
        }
      } else {
        guard let account = accountMetas[safe: index] else { return nil }

        if let addrTableLookupIndexValue = accountMetas[safe: index]?.addrTableLookupIndexValue {
          return SolanaTxDetails.ParsedSolanaInstruction.KeyValue(
            key: param.localizedName,  // use localized param name for known instruction type
            value: addrTableLookupIndexValue
          )
        } else {
          // if the 'to account' is not an Address Lookup Table Account, we should display as:
          // To Account:
          // <account pubkey>
          return SolanaTxDetails.ParsedSolanaInstruction.KeyValue(
            key: param.localizedName,
            value: account.pubkey
          )
        }
      }
    }
  }
}

extension BraveWallet.SolanaAccountMeta {
  var addrTableLookupIndexValue: String? {
    guard let addrTableLookupIndex = addrTableLookupIndex else { return nil }
    // if the 'to account' is an Address Lookup Table Account, we should display as:
    // To Account: (localized name from core)
    // Address Lookup Table Account: <account pubkey>
    // Address Lookup Table Index: <index in address lookup table>
    let addressLookupTableAccount = "\(Strings.Wallet.solanaInstructionAddressLookupAcc): \(pubkey)"
    let addressLookupTableIndex =
      "\(Strings.Wallet.solanaInstructionAddressLookupIndex): \(addrTableLookupIndex.val)"
    return "\(addressLookupTableAccount)\n\(addressLookupTableIndex)"
  }
}

extension BraveWallet.SolanaSystemInstruction {
  var name: String {
    switch self {
    case .transfer:
      return Strings.Wallet.solanaTransferInstructionName
    case .transferWithSeed:
      return Strings.Wallet.solanaTransferWithSeedInstructionName
    case .withdrawNonceAccount:
      return Strings.Wallet.solanaWithdrawNonceAccountInstructionName
    case .createAccount:
      return Strings.Wallet.solanaCreateAccountInstructionName
    case .createAccountWithSeed:
      return Strings.Wallet.solanaCreateAccountWithSeedInstructionName
    case .assign:
      return Strings.Wallet.solanaAssignInstructionName
    case .assignWithSeed:
      return Strings.Wallet.solanaAssignWithSeedInstructionName
    case .allocate:
      return Strings.Wallet.solanaAllocateInstructionName
    case .allocateWithSeed:
      return Strings.Wallet.solanaAllocateWithSeedInstructionName
    case .advanceNonceAccount:
      return Strings.Wallet.solanaAdvanceNonceAccountInstructionName
    case .initializeNonceAccount:
      return Strings.Wallet.solanaInitializeNonceAccountInstructionName
    case .authorizeNonceAccount:
      return Strings.Wallet.solanaAuthorizeNonceAccountInstructionName
    case .upgradeNonceAccount:
      return Strings.Wallet.solanaUpgradeNonceAccountInstructionName
    default:
      return Strings.Wallet.solanaUnknownInstructionName
    }
  }
}

extension BraveWallet.SolanaTokenInstruction {
  var name: String {
    switch self {
    case .initializeMint:
      return Strings.Wallet.solanaInitializeMintInstructionName
    case .initializeMint2:
      return Strings.Wallet.solanaInitializeMint2InstructionName
    case .initializeAccount:
      return Strings.Wallet.solanaInitializeAccountInstructionName
    case .initializeAccount2:
      return Strings.Wallet.solanaInitializeAccount2InstructionName
    case .initializeAccount3:
      return Strings.Wallet.solanaInitializeAccount3InstructionName
    case .initializeMultisig:
      return Strings.Wallet.solanaInitializeMultisigInstructionName
    case .initializeMultisig2:
      return Strings.Wallet.solanaInitializeMultisig2InstructionName
    case .approve:
      return Strings.Wallet.solanaApproveInstructionName
    case .transfer:
      return Strings.Wallet.solanaTransferInstructionName
    case .revoke:
      return Strings.Wallet.solanaRevokeInstructionName
    case .setAuthority:
      return Strings.Wallet.solanaSetAuthorityInstructionName
    case .mintTo:
      return Strings.Wallet.solanaMintToInstructionName
    case .burn:
      return Strings.Wallet.solanaBurnInstructionName
    case .closeAccount:
      return Strings.Wallet.solanaCloseAccountInstructionName
    case .freezeAccount:
      return Strings.Wallet.solanaFreezeAccountInstructionName
    case .thawAccount:
      return Strings.Wallet.solanaThawAccountInstructionName
    case .approveChecked:
      return Strings.Wallet.solanaApproveCheckedInstructionName
    case .transferChecked:
      return Strings.Wallet.solanaTransferCheckedInstructionName
    case .mintToChecked:
      return Strings.Wallet.solanaMintToInstructionName
    case .burnChecked:
      return Strings.Wallet.solanaBurnCheckedInstructionName
    case .syncNative:
      return Strings.Wallet.solanaSyncNativeInstructionName
    @unknown default:
      return Strings.Wallet.solanaUnknownInstructionName
    }
  }
}

extension BraveWallet.DecodedSolanaInstructionData {
  func paramFor(_ paramName: String) -> BraveWallet.SolanaInstructionParam? {
    params.first(where: { $0.name == paramName })
  }
}
