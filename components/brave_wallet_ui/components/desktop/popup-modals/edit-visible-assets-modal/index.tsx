import * as React from 'react'
import {
  TokenInfo,
  AccountAssetOptionType
} from '../../../../constants/types'
import {
  PopupModal,
  AssetWatchlistItem
} from '../..'
import { NavButton } from '../../../extension'
import { SearchBar } from '../../../shared'
import locale from '../../../../constants/locale'

// Styled Components
import {
  StyledWrapper,
  WatchlistScrollContainer,
  LoadIcon,
  LoadingWrapper
} from './style'

export interface Props {
  onClose: () => void
  onUpdateVisibleTokens: (list: string[]) => void
  fullAssetList: TokenInfo[]
  userAssetList: AccountAssetOptionType[]
  userWatchList: string[]
}

const EditVisibleAssetsModal = (props: Props) => {
  const {
    userWatchList,
    fullAssetList,
    userAssetList,
    onClose,
    onUpdateVisibleTokens
  } = props
  const watchlist = React.useMemo(() => {
    const visibleList = userAssetList.map((item) => { return item.asset })
    const notVisibleList = fullAssetList.filter((item) => !userWatchList.includes(item.contractAddress))
    return [...visibleList, ...notVisibleList]
  }, [fullAssetList, userWatchList, userAssetList])
  const [filteredWatchlist, setFilteredWatchlist] = React.useState<TokenInfo[]>([])
  const [selectedWatchlist, setSelectedWatchlist] = React.useState<string[]>(userWatchList)

  React.useMemo(() => {
    setFilteredWatchlist(watchlist)
  }, [watchlist])

  const filterWatchlist = (event: any) => {
    const search = event.target.value
    if (search === '') {
      setFilteredWatchlist(watchlist)
    } else {
      const filteredList = watchlist.filter((item) => {
        return (
          item.name.toLowerCase() === search.toLowerCase() ||
          item.name.toLowerCase().startsWith(search.toLowerCase()) ||
          item.symbol.toLocaleLowerCase() === search.toLowerCase() ||
          item.symbol.toLowerCase().startsWith(search.toLowerCase()) ||
          item.contractAddress.toLocaleLowerCase() === search.toLowerCase()
        )
      })
      setFilteredWatchlist(filteredList)
    }
  }

  const onClickUpdateVisibleTokens = () => {
    onUpdateVisibleTokens(selectedWatchlist)
    onClose()
  }

  const onCheckWatchlistItem = (key: string, selected: boolean) => {
    if (selected) {
      const newList = [...selectedWatchlist, key]
      setSelectedWatchlist(newList)
    } else {
      const newList = selectedWatchlist.filter((id) => id !== key)
      setSelectedWatchlist(newList)
    }
  }

  const isAssetSelected = (key: string): boolean => {
    return selectedWatchlist.includes(key)
  }

  return (
    <PopupModal title={locale.accountsEditVisibleAssets} onClose={onClose}>
      <StyledWrapper>
        {fullAssetList.length === 0 ? (
          <LoadingWrapper>
            <LoadIcon />
          </LoadingWrapper>
        ) : (
          <>
            <SearchBar
              placeholder={locale.watchListSearchPlaceholder}
              action={filterWatchlist}
            />
            <WatchlistScrollContainer>
              {filteredWatchlist.map((item) =>
                <AssetWatchlistItem
                  key={item.contractAddress}
                  id={item.contractAddress}
                  assetBalance='0'
                  icon={item.icon}
                  name={item.name}
                  symbol={item.symbol}
                  isSelected={isAssetSelected(item.contractAddress)}
                  onSelectAsset={onCheckWatchlistItem}
                />
              )}
            </WatchlistScrollContainer>
            <NavButton
              onSubmit={onClickUpdateVisibleTokens}
              text={locale.watchlistButton}
              buttonType='primary'
            />
          </>
        )}
      </StyledWrapper>
    </PopupModal>
  )
}

export default EditVisibleAssetsModal
