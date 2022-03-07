import { BraveWallet, PageState } from '../../constants/types'

export const mockPageState: PageState = {
  hasInitialized: false,
  importAccountError: false,
  importWalletError: {
    hasError: false
  },
  invalidMnemonic: false,
  isCryptoWalletsInitialized: false,
  isFetchingPriceHistory: false,
  isMetaMaskInitialized: false,
  portfolioPriceHistory: [],
  selectedAsset: undefined,
  selectedAssetCryptoPrice: undefined,
  selectedAssetFiatPrice: undefined,
  selectedAssetPriceHistory: [],
  selectedTimeline: BraveWallet.AssetPriceTimeframe.OneDay,
  setupStillInProgress: false,
  showAddModal: false,
  showIsRestoring: false,
  showRecoveryPhrase: false
}
