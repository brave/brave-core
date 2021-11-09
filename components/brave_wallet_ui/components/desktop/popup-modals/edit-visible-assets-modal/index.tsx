import * as React from 'react'
import {
  ERCToken,
  EthereumChain,
  MAINNET_CHAIN_ID
} from '../../../../constants/types'
import {
  PopupModal,
  AssetWatchlistItem
} from '../..'
import { NavButton } from '../../../extension'
import { SearchBar } from '../../../shared'
import { getLocale } from '../../../../../common/locale'
import { toHex } from '../../../../utils/format-balances'

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
  ErrorText,
  TopRow
} from './style'

export interface Props {
  onClose: () => void
  onAddUserAsset: (token: ERCToken) => void
  onSetUserAssetVisible: (token: ERCToken, isVisible: boolean) => void
  onRemoveUserAsset: (token: ERCToken) => void
  addUserAssetError: boolean
  fullAssetList: ERCToken[]
  userVisibleTokensInfo: ERCToken[]
  selectedNetwork: EthereumChain
}

const EditVisibleAssetsModal = (props: Props) => {
  const {
    fullAssetList,
    userVisibleTokensInfo,
    addUserAssetError,
    selectedNetwork,
    onClose,
    onAddUserAsset,
    onRemoveUserAsset,
    onSetUserAssetVisible
  } = props

  const [filteredTokenList, setFilteredTokenList] = React.useState<ERCToken[]>([])
  const [isLoading, setIsLoading] = React.useState<boolean>(false)
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [showAddCustomToken, setShowAddCustomToken] = React.useState<boolean>(false)
  const [tokenName, setTokenName] = React.useState<string>('')
  const [tokenID, setTokenID] = React.useState<string>('')
  const [showTokenIDRequired, setShowTokenIDRequired] = React.useState<boolean>(false)
  const [tokenSymbol, setTokenSymbol] = React.useState<string>('')
  const [tokenContractAddress, setTokenContractAddress] = React.useState<string>('')
  const [tokenDecimals, setTokenDecimals] = React.useState<string>('')
  const [foundToken, setFoundToken] = React.useState<ERCToken>()
  const [hasError, setHasError] = React.useState<boolean>(false)

  const handleTokenNameChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (hasError) {
      setHasError(false)
    }
    setTokenName(event.target.value)
  }

  const handleTokenIDChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (hasError || showTokenIDRequired) {
      setHasError(false)
      setShowTokenIDRequired(false)
    }
    setTokenDecimals('0')
    setTokenID(event.target.value)
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

  const nativeAsset = {
    contractAddress: '',
    decimals: selectedNetwork.decimals,
    isErc20: false,
    isErc721: false,
    logo: selectedNetwork.iconUrls[0] ?? '',
    name: selectedNetwork.symbolName,
    symbol: selectedNetwork.symbol,
    visible: true,
    tokenId: ''
  }

  const tokenList = React.useMemo(() => {
    const visibleContracts = userVisibleTokensInfo.map((token) => token.contractAddress)
    const fullList = visibleContracts.includes('') ? fullAssetList : [nativeAsset, ...fullAssetList]
    const notVisibleList = fullList.filter((token) => !visibleContracts.includes(token.contractAddress))
    return selectedNetwork.chainId !== MAINNET_CHAIN_ID
      ? visibleContracts.includes('')
        ? userVisibleTokensInfo
        : [...userVisibleTokensInfo, nativeAsset]
      : [...userVisibleTokensInfo, ...notVisibleList]
  }, [fullAssetList, userVisibleTokensInfo, selectedNetwork])

  React.useEffect(() => {
    // Added this timeout to throttle setting the list
    // to allow the modal to appear instantly
    setTimeout(function () {
      setFilteredTokenList(tokenList)
    }, 500)
    if (!addUserAssetError) {
      setShowAddCustomToken(false)
    }
    setHasError(addUserAssetError)
    setIsLoading(false)
  }, [tokenList, addUserAssetError])

  const filterWatchlist = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    const search = event.target.value
    if (search === '') {
      setTimeout(function () {
        setFilteredTokenList(tokenList)
      }, 100)
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
      if (foundToken.isErc721) {
        let token = foundToken
        token.tokenId = tokenID ? toHex(tokenID) : ''
        setIsLoading(true)
        onAddUserAsset(token)
        return
      }
      onAddUserAsset(foundToken)
    } else {
      const newToken: ERCToken = {
        contractAddress: tokenContractAddress,
        decimals: Number(tokenDecimals),
        isErc20: tokenID ? false : true,
        isErc721: tokenID ? true : false,
        name: tokenName,
        symbol: tokenSymbol,
        tokenId: tokenID ? toHex(tokenID) : '',
        logo: '',
        visible: true
      }
      onAddUserAsset(newToken)
    }
    setIsLoading(true)
  }

  const isUserToken = (token: ERCToken) => {
    return userVisibleTokensInfo.map(e => e.contractAddress.toLowerCase()).includes(token.contractAddress.toLowerCase())
  }

  const isAssetSelected = (token: ERCToken): boolean => {
    return (isUserToken(token) && token.visible) ?? false
  }

  const isCustomToken = React.useCallback((token: ERCToken): boolean => {
    const assetListContracts = fullAssetList.map((token) => token.contractAddress)
    if (token.isErc20 || token.isErc721) {
      return !assetListContracts.includes(token.contractAddress)
    } else {
      return false
    }
  }, [fullAssetList])

  const onCheckWatchlistItem = (key: string, selected: boolean, token: ERCToken, isCustom: boolean) => {
    if (isUserToken(token)) {
      if (isCustom) {
        selected ? onSetUserAssetVisible(token, true) : onSetUserAssetVisible(token, false)
      } else {
        selected ? onAddUserAsset(token) : onRemoveUserAsset(token)
      }
    } else {
      if (token.isErc721) {
        setTokenContractAddress(token.contractAddress)
        setShowTokenIDRequired(true)
        setShowAddCustomToken(true)
        return
      }
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

  const onRemoveAsset = (token: ERCToken) => {
    setIsLoading(true)
    onRemoveUserAsset(token)
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

  React.useMemo(() => {
    if (foundToken?.isErc721) {
      if (tokenID === '') {
        setShowTokenIDRequired(true)
      }
    }
  }, [foundToken, tokenID])

  const buttonDisabled = React.useMemo((): boolean => {
    return tokenName === ''
      || tokenSymbol === ''
      || (tokenDecimals === '0' && tokenID === '')
      || tokenDecimals === ''
      || tokenContractAddress === ''
      || !tokenContractAddress.toLowerCase().startsWith('0x')
  }, [tokenName, tokenSymbol, tokenDecimals, tokenID, tokenContractAddress])

  return (
    <PopupModal title={showAddCustomToken ? getLocale('braveWalletWatchlistAddCustomAsset') : getLocale('braveWalletAccountsEditVisibleAssets')} onClose={onClose}>
      {showAddCustomToken &&
        <Divider />
      }
      <StyledWrapper>
        {(filteredTokenList.length === 0 && searchValue === '') || isLoading ? (
          <LoadingWrapper>
            <LoadIcon />
          </LoadingWrapper>
        ) : (
          <>
            {showAddCustomToken ? (
              <FormWrapper>
                <InputLabel>{getLocale('braveWalletWatchListTokenName')}</InputLabel>
                <Input
                  value={tokenName}
                  onChange={handleTokenNameChanged}
                  disabled={isDisabled}
                />
                <InputLabel>{getLocale('braveWalletWatchListTokenAddress')}</InputLabel>
                <Input
                  value={tokenContractAddress}
                  onChange={handleTokenAddressChanged}
                />
                <InputLabel>{getLocale('braveWalletWatchListTokenSymbol')}</InputLabel>
                <Input
                  value={tokenSymbol}
                  onChange={handleTokenSymbolChanged}
                  disabled={isDisabled}
                />
                <InputLabel>{getLocale('braveWalletWatchListTokenDecimals')}</InputLabel>
                <Input
                  value={tokenDecimals}
                  onChange={handleTokenDecimalsChanged}
                  disabled={isDisabled || tokenID !== ''}
                  type='number'
                />
                <InputLabel>{getLocale('braveWalletWatchListTokenId')}</InputLabel>
                <Input
                  value={tokenID}
                  onChange={handleTokenIDChanged}
                  type='number'
                  disabled={Number(tokenDecimals) > 0}
                />
                {showTokenIDRequired &&
                  <ErrorText>{getLocale('braveWalletWatchListTokenIdError')}</ErrorText>
                }
                {hasError &&
                  <ErrorText>{getLocale('braveWalletWatchListError')}</ErrorText>
                }
                <ButtonRow>
                  <NavButton
                    onSubmit={onClickCancel}
                    text={getLocale('braveWalletBackupButtonCancel')}
                    buttonType='secondary'
                  />
                  <NavButton
                    onSubmit={onClickAddCustomToken}
                    text={getLocale('braveWalletWatchListAdd')}
                    buttonType='primary'
                    disabled={buttonDisabled}
                  />
                </ButtonRow>
              </FormWrapper>
            ) : (
              <>
                <SearchBar
                  value={searchValue}
                  placeholder={getLocale('braveWalletWatchListSearchPlaceholder')}
                  action={filterWatchlist}
                />
                {!searchValue.toLowerCase().startsWith('0x') &&
                  <TopRow>
                    <NoAssetButton onClick={toggleShowAddCustomToken}>
                      {getLocale('braveWalletWatchlistAddCustomAsset')}
                    </NoAssetButton>
                  </TopRow>
                }
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
                          <NoAssetButton onClick={onClickSuggestAdd}>{getLocale('braveWalletWatchListSuggestion').replace('$1', searchValue)}</NoAssetButton>
                        ) : (
                          <NoAssetText>{getLocale('braveWalletWatchListNoAsset')} {searchValue}</NoAssetText>
                        )}
                      </NoAssetRow>
                    }
                  </>
                </WatchlistScrollContainer>
                <NavButton
                  onSubmit={onClose}
                  text={getLocale('braveWalletWatchListDoneButton')}
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
