import { BraveWallet, PageState } from '../../constants/types'
import { mockNFTMetadata } from './mock-nft-metadata'

export const mockPageState: PageState = {
  isFetchingNFTMetadata: false,
  nftMetadata: mockNFTMetadata[0],
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
