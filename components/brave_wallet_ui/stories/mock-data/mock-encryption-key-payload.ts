import { BraveWallet } from '../../constants/types'
import { mockOriginInfo } from './mock-origin-info'

export const mockEncryptionKeyRequest: BraveWallet.GetEncryptionPublicKeyRequest = {
  address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
  originInfo: mockOriginInfo
}

export const mockDecryptRequest: BraveWallet.DecryptRequest = {
  address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
  unsafeMessage: 'This is a test message.',
  originInfo: mockOriginInfo
}
