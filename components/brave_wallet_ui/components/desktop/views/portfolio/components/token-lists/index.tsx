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
  AddButton
} from '../../../../'

// Styled Components
import {
  ButtonRow,
  DividerText,
  SubDivider,
  Spacer
} from '../../style'

export interface Props {
  filteredAssetList: UserAssetInfoType[]
  tokenPrices: BraveWallet.AssetPrice[]
  defaultCurrencies: DefaultCurrencies
  userAssetList: UserAssetInfoType[]
  onSetFilteredAssetList: (filteredList: UserAssetInfoType[]) => void
  onSelectAsset: (asset: BraveWallet.BlockchainToken | undefined) => () => void
  onShowAssetModal: () => void
}

const TokenLists = (props: Props) => {
  const {
    filteredAssetList,
    tokenPrices,
    defaultCurrencies,
    userAssetList,
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
      <SearchBar placeholder={getLocale('braveWalletSearchText')} action={onFilterAssets} />
      {filteredAssetList.filter((asset) => !asset.asset.isErc721).map((item) =>
        <PortfolioAssetItem
          spotPrices={tokenPrices}
          defaultCurrencies={defaultCurrencies}
          action={onSelectAsset(item.asset)}
          key={item.asset.contractAddress}
          assetBalance={item.assetBalance}
          token={item.asset}
        />
      )}
      {erc721TokenList.length !== 0 &&
        <>
          <Spacer />
          <DividerText>{getLocale('braveWalletTopNavNFTS')}</DividerText>
          <SubDivider />
          {erc721TokenList.map((item) =>
            <PortfolioAssetItem
              spotPrices={tokenPrices}
              defaultCurrencies={defaultCurrencies}
              action={onSelectAsset(item.asset)}
              key={item.asset.contractAddress}
              assetBalance={item.assetBalance}
              token={item.asset}
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
