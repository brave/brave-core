import * as React from 'react'

// Types
import {
  BraveWallet,
  UserAssetInfoType,
  DefaultCurrencies
} from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Components
import { SearchBar } from '../../../../../shared'
import {
  PortfolioAssetItem,
  AddButton,
  NetworkFilterSelector
} from '../../../../'

// Styled Components
import {
  ButtonRow,
  DividerText,
  SubDivider,
  Spacer,
  FilterTokenRow
} from '../../style'

export interface Props {
  filteredAssetList: UserAssetInfoType[]
  defaultCurrencies: DefaultCurrencies
  userAssetList: UserAssetInfoType[]
  hideBalances: boolean
  networks: BraveWallet.NetworkInfo[]
  onSetFilteredAssetList: (filteredList: UserAssetInfoType[]) => void
  onSelectAsset: (asset: BraveWallet.BlockchainToken | undefined) => () => void
  onShowAssetModal: () => void
}

const TokenLists = (props: Props) => {
  const {
    filteredAssetList,
    defaultCurrencies,
    userAssetList,
    hideBalances,
    networks,
    onSelectAsset,
    onShowAssetModal,
    onSetFilteredAssetList
  } = props

  const erc721TokenList = React.useMemo(() => filteredAssetList.filter((token) => token.asset.isErc721), [filteredAssetList])

  // This filters a list of assets when the user types in search bar
  const onFilterAssets = (event: React.ChangeEvent<HTMLInputElement>) => {
    const search = event.target.value
    if (search === '') {
      onSetFilteredAssetList(userAssetList)
    } else {
      const filteredList = userAssetList.filter((item) => {
        return (
          item.asset.name.toLowerCase() === search.toLowerCase() ||
          item.asset.name.toLowerCase().startsWith(search.toLowerCase()) ||
          item.asset.symbol.toLocaleLowerCase() === search.toLowerCase() ||
          item.asset.symbol.toLowerCase().startsWith(search.toLowerCase())
        )
      })
      onSetFilteredAssetList(filteredList)
    }
  }

  return (
    <>
      <FilterTokenRow>
        <SearchBar placeholder={getLocale('braveWalletSearchText')} action={onFilterAssets} />
        <NetworkFilterSelector />
      </FilterTokenRow>
      {filteredAssetList.filter((asset) => !asset.asset.isErc721).map((item) =>
        <PortfolioAssetItem
          defaultCurrencies={defaultCurrencies}
          action={onSelectAsset(item.asset)}
          key={`${item.asset.contractAddress}-${item.asset.symbol}-${item.asset.chainId}`}
          assetBalance={item.assetBalance}
          token={item.asset}
          hideBalances={hideBalances}
          networks={networks}
        />
      )}
      {erc721TokenList.length !== 0 &&
        <>
          <Spacer />
          <DividerText>{getLocale('braveWalletTopNavNFTS')}</DividerText>
          <SubDivider />
          {erc721TokenList.map((item) =>
            <PortfolioAssetItem
              defaultCurrencies={defaultCurrencies}
              action={onSelectAsset(item.asset)}
              key={`${item.asset.contractAddress}-${item.asset.tokenId}-${item.asset.chainId}`}
              assetBalance={item.assetBalance}
              token={item.asset}
              hideBalances={hideBalances}
              networks={networks}
            />
          )}
        </>
      }
      <ButtonRow>
        <AddButton
          buttonType='secondary'
          onSubmit={onShowAssetModal}
          text={getLocale('braveWalletAccountsEditVisibleAssets')}
        />
      </ButtonRow>
    </>
  )
}

export default TokenLists
