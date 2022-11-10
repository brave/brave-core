import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import Fuse from 'fuse.js'

import SelectAssetItem from '../select-asset-item'
import { BraveWallet, WalletRoutes, WalletState } from '../../../constants/types'
import { WalletActions } from '../../../common/actions'
import { SearchBar } from '../../shared'
import Header from '../select-header'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  SelectWrapper,
  SelectScrollSearchContainer,
  DivderTextWrapper,
  DividerText
} from '../shared-styles'
import { useHistory } from 'react-router-dom'
import { PanelActions } from '../../../panel/actions'

export interface Props {
  assets: BraveWallet.BlockchainToken[]
  onSelectAsset: (asset: BraveWallet.BlockchainToken) => () => void
  onBack: () => void
}

function SelectAsset (props: Props) {
  const {
    assets,
    onBack,
    onSelectAsset
  } = props

  // routing
  const history = useHistory()

  // redux
  const {
    fullTokenList,
    selectedNetwork
  } = useSelector((state: { wallet: WalletState }) => state.wallet)
  const dispatch = useDispatch()

  // methods
  const showVisibleAssetsModal = () => {
    if (fullTokenList.length === 0) {
      dispatch(WalletActions.getAllTokensList())
    }
    try {
      // in wallet page
      history.push(`${WalletRoutes.AddAssetModal}`)
    } catch (ex) {
      // in wallet panel
      dispatch(PanelActions.expandWalletAddAsset())
    }
  }

  const fuse = React.useMemo(() => new Fuse(assets, {
    shouldSort: true,
    threshold: 0.45,
    location: 0,
    distance: 100,
    minMatchCharLength: 1,
    keys: [
      { name: 'name', weight: 0.5 },
      { name: 'symbol', weight: 0.5 }
    ]
  }), [assets])

  const [filteredAssetList, setFilteredAssetList] = React.useState<BraveWallet.BlockchainToken[]>(assets)

  const filterAssetList = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    const search = event.target.value
    if (search === '') {
      setFilteredAssetList(assets)
    } else {
      const filteredList = fuse.search(search).map((result: Fuse.FuseResult<BraveWallet.BlockchainToken>) => result.item)
      setFilteredAssetList(filteredList)
    }
  }, [fuse, assets])

  const nonFungibleTokens = React.useMemo(() => filteredAssetList.filter((token) => (token.isErc721 || token.isNft)), [filteredAssetList])

  return (
    <SelectWrapper>
      <Header
        title={getLocale('braveWalletSelectAsset')}
        onBack={onBack}
        hasAddButton={true}
        onClickAdd={showVisibleAssetsModal}
      />
      <SearchBar placeholder={getLocale('braveWalletSearchAsset')} action={filterAssetList} autoFocus={true} />
      <SelectScrollSearchContainer>
        {
          // Temp filtering out erc721 tokens, sending will be handled in a different PR
          filteredAssetList.filter((token) => !token.isErc721 && !token.isNft).map((asset: BraveWallet.BlockchainToken) =>
            <SelectAssetItem
              key={asset.contractAddress}
              asset={asset}
              selectedNetwork={selectedNetwork}
              onSelectAsset={onSelectAsset(asset)}
            />
          )
        }
        {nonFungibleTokens.length > 0 &&
          <>
            <DivderTextWrapper>
              <DividerText>{getLocale('braveWalletTopNavNFTS')}</DividerText>
            </DivderTextWrapper>
            {nonFungibleTokens.map((asset: BraveWallet.BlockchainToken) =>
              <SelectAssetItem
                key={asset.contractAddress}
                asset={asset}
                selectedNetwork={selectedNetwork}
                onSelectAsset={onSelectAsset(asset)}
              />
            )
            }
          </>
        }
      </SelectScrollSearchContainer>
    </SelectWrapper>
  )
}

export default SelectAsset
