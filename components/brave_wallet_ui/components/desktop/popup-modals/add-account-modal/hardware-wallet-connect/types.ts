import locale from '../../../../../constants/locale'

export enum HardwareWallet {
  Ledger = 'Ledger',
  Trezor = 'Trezor'
}

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
  [HardwareWallet.Ledger]: LedgerDerivationPaths,
  [HardwareWallet.Trezor]: TrezorDerivationPaths
}

export interface HardwareWalletConnectOpts {
  hardware: HardwareWallet

  // TODO: add currency and network as enums
  // currency: string
  // network: string

  startIndex: number
  stopIndex: number
}

// Keep in sync with components/brave_wallet/common/brave_wallet.mojom until
// we auto generate this type file from mojo.
export interface HardwareWalletAccount {
  address: string
  derivationPath: string
  name: string
  hardwareVendor: string
}
