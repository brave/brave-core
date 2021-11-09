import {
  LEDGER_HARDWARE_VENDOR,
  TREZOR_HARDWARE_VENDOR
} from '../../../../../constants/types'

export enum LedgerDerivationPaths {
  LedgerLive = 'ledger-live',
  Legacy = 'legacy'
}

export enum TrezorDerivationPaths {
  Default = 'trezor'
}

export const HardwareWalletDerivationPathLocaleMapping = {
  [LedgerDerivationPaths.LedgerLive]: 'Ledger Live',
  [LedgerDerivationPaths.Legacy]: 'Legacy (MEW/MyCrypto)',
  [TrezorDerivationPaths.Default]: 'Default'
}

export const HardwareWalletDerivationPathsMapping = {
  [LEDGER_HARDWARE_VENDOR]: LedgerDerivationPaths,
  [TREZOR_HARDWARE_VENDOR]: TrezorDerivationPaths
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
  deviceId: string
}

export interface ErrorMessage {
  error: string
  userHint: string
}

export interface TrezorBridgeAccountsPayload {
  success: boolean,
  accounts: HardwareWalletAccount[],
  error?: string
}
