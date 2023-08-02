// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'

import getWalletPageApiProxy from '../wallet_page_api_proxy'
import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletPageActions from '../actions/wallet_page_actions'
import * as WalletActions from '../../common/actions/wallet_actions'
import {
  BraveWallet,
  CoinType,
  NFTMetadataReturnType
} from '../../constants/types'
import {
  CreateWalletPayloadType,
  UpdateSelectedAssetType,
  ImportAccountPayloadType,
  ImportAccountFromJsonPayloadType,
  ImportFromExternalWalletPayloadType,
  ImportFilecoinAccountPayloadType,
  RestoreWalletPayloadType,
  ImportWalletErrorPayloadType,
  ShowRecoveryPhrasePayload,
  UpdateNftPinningStatusType
} from '../constants/action_types'
import {
  getNFTMetadata,
  translateToNftGateway
} from '../../common/async/lib'
import { Store } from '../../common/async/types'
import { getLocale } from '../../../common/locale'
import { getNetwork } from '../../common/slices/api.slice'

const handler = new AsyncActionHandler()

async function refreshWalletInfo(store: Store) {
  const proxy = getWalletPageApiProxy()
  const { walletInfo } = await proxy.walletHandler.getWalletInfo()
  const { allAccounts } = await proxy.keyringService.getAllAccounts()
  store.dispatch(WalletActions.initialized({ walletInfo, allAccounts }))
  store.dispatch(WalletActions.refreshAll({}))
}

async function importFromExternalWallet (
  walletType: BraveWallet.ExternalWalletType,
  payload: ImportFromExternalWalletPayloadType
): Promise<ImportWalletErrorPayloadType> {
  const {
    braveWalletService,
    keyringService
  } = getWalletPageApiProxy()

  const result = await braveWalletService.importFromExternalWallet(
    walletType,
    payload.password,
    payload.newPassword
  )

  // complete backup if a new password was provided
  if (payload.newPassword && result.success) {
    keyringService.notifyWalletBackupComplete()
  }

  // was the provided import password correct?
  const checkExistingPasswordError = result.errorMessage === getLocale('braveWalletImportPasswordError')
    ? result.errorMessage
    : undefined

  // was import successful (if attempted)
  const importError = payload.newPassword ? result.errorMessage || undefined : undefined

  return {
    hasError: !!(importError || checkExistingPasswordError),
    errorMessage: importError || checkExistingPasswordError,
    incrementAttempts: true
  }
}

handler.on(WalletPageActions.createWallet.type, async (store: Store, payload: CreateWalletPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.createWallet(payload.password)
  store.dispatch(WalletPageActions.walletCreated({ mnemonic: result.mnemonic }))
})

handler.on(WalletPageActions.restoreWallet.type, async (store: Store, payload: RestoreWalletPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.restoreWallet(payload.mnemonic, payload.password, payload.isLegacy)
  if (!result.isValidMnemonic) {
    store.dispatch(WalletPageActions.hasMnemonicError(!result.isValidMnemonic))
    return
  }
  keyringService.notifyWalletBackupComplete()
  await refreshWalletInfo(store)
  store.dispatch(WalletPageActions.setShowIsRestoring(false))
  if (payload?.completeWalletSetup) {
    store.dispatch(WalletPageActions.walletSetupComplete(payload.completeWalletSetup))
  }
})

handler.on(WalletPageActions.showRecoveryPhrase.type, async (store: Store, {
  password,
  show
}: ShowRecoveryPhrasePayload) => {
  if (password) {
    const { keyringService } = getWalletPageApiProxy()
    const { mnemonic } = await keyringService.getMnemonicForDefaultKeyring(password)
    store.dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic }))
    return
  }

  store.dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic: '' }))
})

handler.on(WalletPageActions.walletBackupComplete.type, async (store) => {
  const keyringService = getWalletPageApiProxy().keyringService
  keyringService.notifyWalletBackupComplete()
})

handler.on(WalletPageActions.selectAsset.type, async (store: Store, payload: UpdateSelectedAssetType) => {
  store.dispatch(WalletPageActions.updateSelectedAsset(payload.asset))
  if (payload.asset) {
    store.dispatch(WalletPageActions.selectPriceTimeframe(payload.timeFrame))

    if (payload.asset.isErc721 || payload.asset.isNft) {
      store.dispatch(WalletPageActions.getNFTMetadata(payload.asset))

      if (store.getState().wallet.isNftPinningFeatureEnabled) {
        store.dispatch(WalletPageActions.getPinStatus(payload.asset))
      }
    }
  } else {
    store.dispatch(WalletPageActions.selectPriceTimeframe(payload.timeFrame))
  }
})

handler.on(WalletPageActions.importAccount.type, async (store: Store, payload: ImportAccountPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.importAccount(payload.accountName, payload.privateKey, payload.coin)
  if (result.account) {
    store.dispatch(WalletPageActions.setImportAccountError(false))
    store.dispatch(WalletPageActions.setShowAddModal(false))
  } else {
    store.dispatch(WalletPageActions.setImportAccountError(true))
  }
})

handler.on(WalletPageActions.importFilecoinAccount.type, async (store: Store, payload: ImportFilecoinAccountPayloadType) => {
  const { keyringService } = getWalletPageApiProxy()
  const result = await keyringService.importFilecoinAccount(payload.accountName, payload.privateKey, payload.network)

  if (result.account) {
    store.dispatch(WalletPageActions.setImportAccountError(false))
    store.dispatch(WalletPageActions.setShowAddModal(false))
  } else {
    store.dispatch(WalletPageActions.setImportAccountError(true))
  }
})

handler.on(WalletPageActions.importAccountFromJson.type, async (store: Store, payload: ImportAccountFromJsonPayloadType) => {
  const keyringService = getWalletPageApiProxy().keyringService
  const result = await keyringService.importAccountFromJson(payload.accountName, payload.password, payload.json)
  if (result.account) {
    store.dispatch(WalletPageActions.setImportAccountError(false))
    store.dispatch(WalletPageActions.setShowAddModal(false))
  } else {
    store.dispatch(WalletPageActions.setImportAccountError(true))
  }
})

handler.on(WalletPageActions.addHardwareAccounts.type, async (store: Store, accounts: BraveWallet.HardwareWalletAccount[]) => {
  const keyringService = getWalletPageApiProxy().keyringService
  keyringService.addHardwareAccounts(accounts)
  store.dispatch(WalletPageActions.setShowAddModal(false))
})

handler.on(WalletPageActions.checkWalletsToImport.type, async (store) => {
  store.dispatch(WalletPageActions.setImportWalletsCheckComplete(false))
  const braveWalletService = getWalletPageApiProxy().braveWalletService
  const cwResult =
    await braveWalletService.isExternalWalletInitialized(
      BraveWallet.ExternalWalletType.CryptoWallets)
  const mmResult =
    await braveWalletService.isExternalWalletInitialized(
      BraveWallet.ExternalWalletType.MetaMask)
  store.dispatch(WalletPageActions.setCryptoWalletsInitialized(cwResult.initialized))
  store.dispatch(WalletPageActions.setMetaMaskInitialized(mmResult.initialized))
  store.dispatch(WalletPageActions.setImportWalletsCheckComplete(true))
})

handler.on(WalletPageActions.importFromCryptoWallets.type, async (store: Store, payload: ImportFromExternalWalletPayloadType) => {
  const results: ImportWalletErrorPayloadType = await importFromExternalWallet(
    BraveWallet.ExternalWalletType.CryptoWallets,
    payload
  )
  store.dispatch(WalletPageActions.setImportWalletError(results))
})

handler.on(WalletPageActions.importFromMetaMask.type, async (store: Store, payload: ImportFromExternalWalletPayloadType) => {
  const results: ImportWalletErrorPayloadType = await importFromExternalWallet(
    BraveWallet.ExternalWalletType.MetaMask,
    payload
  )
  store.dispatch(WalletPageActions.setImportWalletError(results))
})

handler.on(WalletPageActions.openWalletSettings.type, async (store) => {
  chrome.tabs.create({ url: 'chrome://settings/wallet' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(
  WalletPageActions.getNFTMetadata.type,
  async (store: Store, payload: BraveWallet.BlockchainToken) => {
    store.dispatch(WalletPageActions.setIsFetchingNFTMetadata(true))
    const result = await getNFTMetadata(payload)
    if (!result?.error) {
      const response = result?.response && JSON.parse(result.response)
      const attributes = Array.isArray(response.attributes)
      ? response.attributes.map((attr: { trait_type: string; value: string }) => ({ traitType: attr.trait_type, value: attr.value }))
      : []
      const tokenNetwork = await getNetwork(getWalletPageApiProxy(), payload)

      const nftMetadata: NFTMetadataReturnType = {
        metadataUrl: result?.tokenUrl || '',
        chainName: tokenNetwork?.chainName || '',
        tokenType:
          payload.coin === CoinType.ETH
            ? 'ERC721'
            : payload.coin === CoinType.SOL
            ? 'SPL'
            : '',
        tokenID: payload.tokenId,
        imageURL: response.image.startsWith('data:image/')
          ? response.image
          : await translateToNftGateway(response.image),
        imageMimeType: 'image/*',
        floorFiatPrice: '',
        floorCryptoPrice: '',
        contractInformation: {
          address: payload.contractAddress,
          name: response.name,
          description: response.description,
          website: '',
          facebook: '',
          logo: '',
          twitter: ''
        },
        attributes
      }
      store.dispatch(WalletPageActions.updateNFTMetadata(nftMetadata))
      store.dispatch(WalletPageActions.updateNftMetadataError(undefined))
    } else {
      store.dispatch(
        WalletPageActions.updateNftMetadataError(result.errorMessage)
      )
    }
    store.dispatch(WalletPageActions.setIsFetchingNFTMetadata(false))
  }
)

handler.on(WalletPageActions.getIsAutoPinEnabled.type, async (store) => {
  if (!store.getState().wallet.isNftPinningFeatureEnabled) return

  const { braveWalletAutoPinService } = getWalletPageApiProxy()
  const { enabled } = await braveWalletAutoPinService.isAutoPinEnabled()
  store.dispatch(WalletPageActions.updateAutoPinEnabled(enabled))
})

handler.on(WalletPageActions.setAutoPinEnabled.type, async (store, payload: boolean) => {
  if (!store.getState().wallet.isNftPinningFeatureEnabled) return

  const { braveWalletAutoPinService } = getWalletPageApiProxy()
  store.dispatch(WalletPageActions.updateEnablingAutoPin(true))
  await braveWalletAutoPinService.setAutoPinEnabled(payload)
  store.dispatch(WalletPageActions.updateEnablingAutoPin(false))
  const { enabled } = await braveWalletAutoPinService.isAutoPinEnabled()
  store.dispatch(WalletPageActions.updateAutoPinEnabled(enabled))
})

handler.on(WalletPageActions.getPinStatus.type, async (store, payload: BraveWallet.BlockchainToken) => {
  if (!store.getState().wallet.isNftPinningFeatureEnabled) return

  const braveWalletPinService = getWalletPageApiProxy().braveWalletPinService
  const result = await braveWalletPinService.getTokenStatus(payload)
  if (result.status) {
    store.dispatch(WalletPageActions.updateNFTPinStatus(result.status))
  } else {
    store.dispatch(WalletPageActions.updateNFTPinStatus(undefined))
  }
})

handler.on(WalletPageActions.getNftsPinningStatus.type, async (store, payload: BraveWallet.BlockchainToken[]) => {
  if (!store.getState().wallet.isNftPinningFeatureEnabled) return

  const { braveWalletPinService } = getWalletPageApiProxy()

  const results = await mapLimit(
    payload,
    10,
    async (token: BraveWallet.BlockchainToken) =>
      await braveWalletPinService.getTokenStatus(token)
  )

  const nftsPinningStatus = results.map((result, index) => {
    const status: UpdateNftPinningStatusType = {
      token: payload[index],
      status: result.status?.local || undefined,
      error: result.error || undefined
    }

    return status
  })
  store.dispatch(WalletPageActions.setNftsPinningStatus(nftsPinningStatus))
})

handler.on(WalletPageActions.getLocalIpfsNodeStatus.type, async (store) => {
  if (!store.getState().wallet.isNftPinningFeatureEnabled) return

  const { braveWalletPinService } = getWalletPageApiProxy()
  const isNodeRunning = await braveWalletPinService.isLocalNodeRunning()
  store.dispatch(WalletPageActions.updateLocalIpfsNodeStatus(isNodeRunning.result))
})

export default handler.middleware
