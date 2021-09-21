import * as React from 'react'
import {
  TokenInfo
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
  ButtonRow,
  ErrorText
} from './style'

export interface Props {
  onClose: () => void
  onAddUserAsset: (token: TokenInfo) => void
  onSetUserAssetVisible: (contractAddress: string, isVisible: boolean) => void
  onRemoveUserAsset: (contractAddress: string) => void
  addUserAssetError: boolean
  fullAssetList: TokenInfo[]
  userVisibleTokensInfo: TokenInfo[]
}

const EditVisibleAssetsModal = (props: Props) => {
  const {
    fullAssetList,
    userVisibleTokensInfo,
    addUserAssetError,
    onClose,
    onAddUserAsset,
    onRemoveUserAsset,
    onSetUserAssetVisible
  } = props

  const [filteredTokenList, setFilteredTokenList] = React.useState<TokenInfo[]>([])
  const [isLoading, setIsLoading] = React.useState<boolean>(false)
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [showAddCustomToken, setShowAddCustomToken] = React.useState<boolean>(false)
  const [tokenName, setTokenName] = React.useState<string>('')
  const [tokenSymbol, setTokenSymbol] = React.useState<string>('')
  const [tokenContractAddress, setTokenContractAddress] = React.useState<string>('')
  const [tokenDecimals, setTokenDecimals] = React.useState<string>('')
  const [foundToken, setFoundToken] = React.useState<TokenInfo>()
  const [hasError, setHasError] = React.useState<boolean>(false)

  const handleTokenNameChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (hasError) {
      setHasError(false)
    }
    setTokenName(event.target.value)
  }

  const handleTokenSymbolChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (hasError) {
      setHasError(false)
    }
    setTokenSymbol(event.target.value)
  }

  const handleTokenAddressChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (hasError) {
      setHasError(false)
    }
    setTokenContractAddress(event.target.value)
  }

  const handleTokenDecimalsChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (hasError) {
      setHasError(false)
    }
    setTokenDecimals(event.target.value)
  }

  const tokenList = React.useMemo(() => {
    const visibleContracts = userVisibleTokensInfo.map((token) => token.contractAddress)
    const notVisibleList = fullAssetList.filter((token) => !visibleContracts.includes(token.contractAddress))
    return [...userVisibleTokensInfo, ...notVisibleList]
  }, [fullAssetList, userVisibleTokensInfo])

  React.useMemo(() => {
    setFilteredTokenList(tokenList)
    if (!addUserAssetError) {
      setShowAddCustomToken(false)
    }
    setHasError(addUserAssetError)
    setIsLoading(false)
  }, [tokenList, addUserAssetError])

  const filterWatchlist = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    const search = event.target.value
    if (search === '') {
      setFilteredTokenList(tokenList)
    } else {
      const filteredList = tokenList.filter((item) => {
        return (
          item.name.toLowerCase() === search.toLowerCase() ||
          item.name.toLowerCase().startsWith(search.toLowerCase()) ||
          item.symbol.toLocaleLowerCase() === search.toLowerCase() ||
          item.symbol.toLowerCase().startsWith(search.toLowerCase()) ||
          item.contractAddress.toLocaleLowerCase() === search.toLowerCase()
        )
      })
      setFilteredTokenList(filteredList)
    }
    setSearchValue(search)
  }, [tokenList])

  const onClickAddCustomToken = () => {
    if (foundToken) {
      onAddUserAsset(foundToken)
    } else {
      const newToken: TokenInfo = {
        contractAddress: tokenContractAddress,
        decimals: Number(tokenDecimals),
        isErc20: true,
        isErc721: false,
        name: tokenName,
        symbol: tokenSymbol,
        logo: '',
        visible: true
      }
      onAddUserAsset(newToken)
    }
    setIsLoading(true)
  }

  const isAssetSelected = (token: TokenInfo): boolean => {
    const isUserToken = userVisibleTokensInfo.map(e => e.contractAddress).includes(token.contractAddress)
    return (isUserToken && token.visible) ?? false
  }

  const isCustomToken = React.useCallback((token: TokenInfo): boolean => {
    const assetListContracts = fullAssetList.map((token) => token.contractAddress)
    if (token.isErc20) {
      return !assetListContracts.includes(token.contractAddress)
    } else {
      return false
    }
  }, [fullAssetList])

  const onCheckWatchlistItem = (key: string, selected: boolean, token: TokenInfo, isCustom: boolean) => {
    const isUserToken = userVisibleTokensInfo.includes(token)
    if (isUserToken) {
      if (isCustom) {
        selected ? onSetUserAssetVisible(token.contractAddress, true) : onSetUserAssetVisible(token.contractAddress, false)
      } else {
        selected ? onAddUserAsset(token) : onRemoveUserAsset(token.contractAddress)
      }
    } else {
      onAddUserAsset(token)
    }
    setIsLoading(true)
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

  const onRemoveAsset = (token: TokenInfo) => {
    setIsLoading(true)
    onRemoveUserAsset(token.contractAddress)
  }

  const isDisabled = React.useMemo(() => {
    const token = fullAssetList.find((token) => token.contractAddress.toLowerCase() === tokenContractAddress.toLowerCase())
    if (token) {
      setFoundToken(token)
      setTokenName(token.name)
      setTokenSymbol(token.symbol)
      setTokenDecimals(token.decimals.toString())
      return true
    } else {
      setFoundToken(undefined)
      return false
    }
  }, [tokenContractAddress, fullAssetList])

  return (
    <PopupModal title={showAddCustomToken ? locale.watchlistAddCustomAsset : locale.accountsEditVisibleAssets} onClose={onClose}>
      {showAddCustomToken &&
        <Divider />
      }
      <StyledWrapper>
        {fullAssetList.length === 0 || isLoading ? (
          <LoadingWrapper>
            <LoadIcon />
          </LoadingWrapper>
        ) : (
          <>
            {showAddCustomToken ? (
              <FormWrapper>
                <InputLabel>{locale.watchListTokenName}</InputLabel>
                <Input
                  value={tokenName}
                  onChange={handleTokenNameChanged}
                  disabled={isDisabled}
                />
                <InputLabel>{locale.watchListTokenAddress}</InputLabel>
                <Input
                  value={tokenContractAddress}
                  onChange={handleTokenAddressChanged}
                />
                <InputLabel>{locale.watchListTokenSymbol}</InputLabel>
                <Input
                  value={tokenSymbol}
                  onChange={handleTokenSymbolChanged}
                  disabled={isDisabled}
                />
                <InputLabel>{locale.watchListTokenDecimals}</InputLabel>
                <Input
                  value={tokenDecimals}
                  onChange={handleTokenDecimalsChanged}
                  disabled={isDisabled}
                  type='number'
                />
                {hasError &&
                  <ErrorText>{locale.watchListError}</ErrorText>
                }
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
                      || !tokenContractAddress.toLowerCase().startsWith('0x')
                    }
                  />
                </ButtonRow>
              </FormWrapper>
            ) : (
              <>
                <SearchBar
                  value={searchValue}
                  placeholder={locale.watchListSearchPlaceholder}
                  action={filterWatchlist}
                />
                <WatchlistScrollContainer>
                  <>
                    {filteredTokenList.map((token) =>
                      <AssetWatchlistItem
                        key={token.contractAddress}
                        isCustom={isCustomToken(token)}
                        token={token}
                        onRemoveAsset={onRemoveAsset}
                        isSelected={isAssetSelected(token)}
                        onSelectAsset={onCheckWatchlistItem}
                      />
                    )}
                    {filteredTokenList.length === 0 &&
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
