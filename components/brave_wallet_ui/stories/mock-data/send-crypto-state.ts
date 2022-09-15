import { PendingCryptoSendState } from '../../common/reducers/send_crypto_reducer'

export const mockSendCryptoState: PendingCryptoSendState = {
  sendAmount: '',
  toAddress: '',
  toAddressOrUrl: '',
  addressError: undefined,
  addressWarning: undefined,
  selectedSendAsset: undefined,
  showEnsOffchainLookupOptions: false,
  ensOffchainLookupOptions: undefined
}
