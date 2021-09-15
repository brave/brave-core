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
  LoadingWrapper,
  NoAssetText,
  NoAssetRow,
  NoAssetButton,
  FormWrapper,
  InputLabel,
  Input,
  Divider,
  ButtonRow
} from './style'

export interface Props {
  onClose: () => void
  onUpdateVisibleTokens: (contractAddress: string, visible: boolean) => void
  onAddCustomToken: (tokenName: string, tokenSymbol: string, tokenContractAddress: string, tokenDecimals: number) => void
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
    onAddCustomToken
  } = props
  const watchlist = React.useMemo(() => {
    const visibleList = userAssetList.map((item) => { return item.asset })
    const notVisibleList = fullAssetList.filter((item) => !userWatchList.includes(item.contractAddress))
    return [...visibleList, ...notVisibleList]
  }, [fullAssetList, userWatchList, userAssetList])
  const [filteredWatchlist, setFilteredWatchlist] = React.useState<TokenInfo[]>([])
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [selectedWatchlist, setSelectedWatchlist] = React.useState<string[]>(userWatchList)
  const [showAddCustomToken, setShowAddCustomToken] = React.useState<boolean>(false)
  const [tokenName, setTokenName] = React.useState<string>('')
  const [tokenSymbol, setTokenSymbol] = React.useState<string>('')
  const [tokenContractAddress, setTokenContractAddress] = React.useState<string>('')
  const [tokenDecimals, setTokenDecimals] = React.useState<string>('')

  const handleTokenNameChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setTokenName(event.target.value)
  }

  const handleTokenSymbolChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setTokenSymbol(event.target.value)
  }

  const handleTokenAddressChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setTokenContractAddress(event.target.value)
  }

  const handleTokenDecimalsChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setTokenDecimals(event.target.value)
  }

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
    setSearchValue(search)
  }

  const onClickAddCustomToken = () => {
    onAddCustomToken(tokenName, tokenSymbol, tokenContractAddress, Number(tokenDecimals))
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

  const toggleShowAddCustomToken = () => {
    setShowAddCustomToken(!showAddCustomToken)
  }

  const onClickSuggestAdd = () => {
    setTokenContractAddress(searchValue)
    toggleShowAddCustomToken()
  }

  const onClickCancel = () => {
    setTokenContractAddress('')
    toggleShowAddCustomToken()
  }

  return (
    <PopupModal title={showAddCustomToken ? locale.watchlistAddCustomAsset : locale.accountsEditVisibleAssets} onClose={onClose}>
      {showAddCustomToken &&
        <Divider />
      }
      <StyledWrapper>
        {showAddCustomToken ? (
          <FormWrapper>
            <InputLabel>{locale.watchListTokenName}</InputLabel>
            <Input
              onChange={handleTokenNameChanged}
            />
            <InputLabel>{locale.watchListTokenAddress}</InputLabel>
            <Input
              value={tokenContractAddress}
              onChange={handleTokenAddressChanged}
            />
            <InputLabel>{locale.watchListTokenSymbol}</InputLabel>
            <Input
              onChange={handleTokenSymbolChanged}
            />
            <InputLabel>{locale.watchListTokenDecimals}</InputLabel>
            <Input
              onChange={handleTokenDecimalsChanged}
              type='number'
            />
            <ButtonRow>
              <NavButton
                onSubmit={onClickCancel}
                text={locale.backupButtonCancel}
                buttonType='secondary'
              />
              <NavButton
                onSubmit={onClickAddCustomToken}
                text={locale.watchListAdd}
                buttonType='primary'
                disabled={
                  tokenName === ''
                  || tokenSymbol === ''
                  || tokenDecimals === ''
                  || tokenContractAddress === ''
                }
              />
            </ButtonRow>
          </FormWrapper>
        ) : (
          <>
            {fullAssetList.length === 0 ? (
              <LoadingWrapper>
                <LoadIcon />
              </LoadingWrapper>
            ) : (
              <>
                <SearchBar
                  value={searchValue}
                  placeholder={locale.watchListSearchPlaceholder}
                  action={filterWatchlist}
                />
                <WatchlistScrollContainer>
                  <>
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
                    {filteredWatchlist.length === 0 &&
                      <NoAssetRow>
                        {searchValue.toLowerCase().startsWith('0x') ? (
                          <NoAssetButton onClick={onClickSuggestAdd}>{locale.watchListAdd} {searchValue} {locale.watchListSuggestion}</NoAssetButton>
                        ) : (
                          <NoAssetText>{locale.watchListNoAsset} {searchValue}</NoAssetText>
                        )}
                      </NoAssetRow>
                    }
                  </>
                </WatchlistScrollContainer>
                <NavButton
                  onSubmit={toggleShowAddCustomToken}
                  text={locale.watchlistAddCustomAsset}
                  buttonType='primary'
                />
              </>
            )}
          </>
        )}
      </StyledWrapper>
    </PopupModal>
  )
}

export default EditVisibleAssetsModal
