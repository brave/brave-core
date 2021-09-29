import locale from '../../../../../constants/locale'

import {
  kLedgerHardwareVendor,
  kTrezorHardwareVendor
} from '../../../../../constants/types'

export enum LedgerDerivationPaths {
  LedgerLive = 'ledger-live',
  Legacy = 'legacy'
}

export enum TrezorDerivationPaths {
  Default = 'trezor'
}

export const HardwareWalletDerivationPathLocaleMapping = {
  [LedgerDerivationPaths.LedgerLive]: locale.ledgerLiveDerivationPath,
  [LedgerDerivationPaths.Legacy]: locale.ledgerLegacyDerivationPath,
  [TrezorDerivationPaths.Default]: 'Default'
}

export const HardwareWalletDerivationPathsMapping = {
  [kLedgerHardwareVendor]: LedgerDerivationPaths,
  [kTrezorHardwareVendor]: TrezorDerivationPaths
}

export interface HardwareWalletConnectOpts {
  hardware: string
  // TODO: add currency and network as enums
  // currency: string
  // network: string

  startIndex: number
  stopIndex: number
  scheme: string
}

// Keep in sync with components/brave_wallet/common/brave_wallet.mojom until
// we auto generate this type file from mojo.
export interface HardwareWalletAccount {
  address: string
  derivationPath: string
  name: string
  hardwareVendor: string
}
