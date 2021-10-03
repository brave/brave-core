// Keep in sync with components/brave_wallet/common/brave_wallet.mojom until
// we auto generate this type file from mojo.
export interface HardwareWalletAccount {
    address: string
    derivationPath: string
    name: string
    hardwareVendor: string
  }
