import * as React from 'react'
import { BraveWallet } from '../../../../constants/types'
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
  TopRow,
  DividerText,
  SubDivider,
  DividerRow,
  AdvancedButton,
  AdvancedIcon
} from './style'

export interface Props {
  onClose: () => void
  onAddCustomAsset: (token: BraveWallet.BlockchainToken) => void
  onUpdateVisibleAssets: (updatedTokensList: BraveWallet.BlockchainToken[]) => void
  addUserAssetError: boolean
  fullAssetList: BraveWallet.BlockchainToken[]
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
  selectedNetwork: BraveWallet.EthereumChain
  onFindTokenInfoByContractAddress: (contractAddress: string) => void
  foundTokenInfoByContractAddress?: BraveWallet.BlockchainToken
}

const EditVisibleAssetsModal = (props: Props) => {
  const {
    fullAssetList,
    userVisibleTokensInfo,
    addUserAssetError,
    selectedNetwork,
    onClose,
    onAddCustomAsset,
    onFindTokenInfoByContractAddress,
    onUpdateVisibleAssets,
    foundTokenInfoByContractAddress
  } = props

  // Token List States
  const [filteredTokenList, setFilteredTokenList] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [updatedTokensList, setUpdatedTokensList] = React.useState<BraveWallet.BlockchainToken[]>([])

  // Modal UI States
  const [showAddCustomToken, setShowAddCustomToken] = React.useState<boolean>(false)
  const [showAdvancedFields, setShowAdvancedFields] = React.useState<boolean>(false)
  const [isLoading, setIsLoading] = React.useState<boolean>(false)
  const [hasError, setHasError] = React.useState<boolean>(false)
  const [showTokenIDRequired, setShowTokenIDRequired] = React.useState<boolean>(false)

  // Form States
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [tokenName, setTokenName] = React.useState<string>('')
  const [tokenID, setTokenID] = React.useState<string>('')
  const [tokenSymbol, setTokenSymbol] = React.useState<string>('')
  const [tokenContractAddress, setTokenContractAddress] = React.useState<string>('')
  const [tokenDecimals, setTokenDecimals] = React.useState<string>('')
  const [coingeckoID, setCoingeckoID] = React.useState<string>('')
  const [iconURL, setIconURL] = React.useState<string>('')

  // If a user removes all of their assets from the userVisibleTokenInfo list,
  // there is a check in the async/lib.ts folder that will still return the networks
  // native asset (example 'ETH') with the value of visible: false to prevent breaking
  // other parts of the wallet.

  // This method here is used to determine if that is the case
  // and allows us to handle our true visible lists.
  const isUserVisibleTokensInfoEmpty = React.useMemo((): boolean => {
    return userVisibleTokensInfo.length === 1 &&
      userVisibleTokensInfo[0].contractAddress === '' &&
      !userVisibleTokensInfo[0].visible
  }, [userVisibleTokensInfo])

  React.useEffect(() => {
    if (isUserVisibleTokensInfoEmpty) {
      return
    }
    setUpdatedTokensList(userVisibleTokensInfo)
  }, [userVisibleTokensInfo])

  // Handle Form Input Changes
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

  const handleCoingeckoIDChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (hasError) {
      setHasError(false)
    }
    setCoingeckoID(event.target.value)
  }

  const handleIconURLChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (hasError) {
      setHasError(false)
    }
    setIconURL(event.target.value)
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
    tokenId: '',
    coingeckoId: ''
  }

  const tokenList = React.useMemo(() => {
    const userVisibleContracts = isUserVisibleTokensInfoEmpty
      ? []
      : userVisibleTokensInfo.map((token) => token.contractAddress.toLowerCase())

    const fullAssetsListPlusNativeToken = userVisibleContracts.includes('')
      ? fullAssetList
      : [nativeAsset, ...fullAssetList]

    const filteredTokenRegistry = fullAssetsListPlusNativeToken.filter((token) => !userVisibleContracts.includes(token.contractAddress.toLowerCase()))

    return isUserVisibleTokensInfoEmpty
      ? filteredTokenRegistry
      : [...userVisibleTokensInfo, ...filteredTokenRegistry]
  }, [fullAssetList, userVisibleTokensInfo])

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
    if (foundTokenInfoByContractAddress) {
      if (foundTokenInfoByContractAddress.isErc721) {
        let token = foundTokenInfoByContractAddress
        token.tokenId = tokenID ? toHex(tokenID) : ''
        token.logo = iconURL
        setIsLoading(true)
        onAddCustomAsset(token)
        return
      }
      let foundToken = foundTokenInfoByContractAddress
      foundToken.coingeckoId = coingeckoID !== '' ? coingeckoID : foundTokenInfoByContractAddress.coingeckoId
      foundToken.logo = iconURL
      onAddCustomAsset(foundToken)
    } else {
      const newToken: BraveWallet.BlockchainToken = {
        contractAddress: tokenContractAddress,
        decimals: Number(tokenDecimals),
        isErc20: !tokenID,
        isErc721: !!tokenID,
        name: tokenName,
        symbol: tokenSymbol,
        tokenId: tokenID ? toHex(tokenID) : '',
        logo: iconURL,
        visible: true,
        coingeckoId: coingeckoID
      }
      onAddCustomAsset(newToken)
    }
    setIsLoading(true)
  }

  const findUpdatedTokenInfo = (token: BraveWallet.BlockchainToken) => {
    return updatedTokensList.find((t) => t.contractAddress.toLowerCase() === token.contractAddress.toLowerCase())
  }

  const isUserToken = (token: BraveWallet.BlockchainToken) => {
    return updatedTokensList.some(e => e.contractAddress.toLowerCase() === token.contractAddress.toLowerCase())
  }

  const isAssetSelected = (token: BraveWallet.BlockchainToken): boolean => {
    return (isUserToken(token) && findUpdatedTokenInfo(token)?.visible) ?? false
  }

  const isCustomToken = React.useCallback((token: BraveWallet.BlockchainToken): boolean => {
    if (token.contractAddress === '') {
      return false
    }

    return !fullAssetList
      .some(each => each.contractAddress.toLowerCase() === token.contractAddress.toLowerCase())
  }, [fullAssetList])

  const addOrRemoveTokenFromList = (selected: boolean, token: BraveWallet.BlockchainToken) => {
    if (selected) {
      const addededToList = [...updatedTokensList, token]
      return addededToList
    }
    const removedFromList = updatedTokensList.filter((t) => t.contractAddress !== token.contractAddress)
    return removedFromList
  }

  const onCheckWatchlistItem = (key: string, selected: boolean, token: BraveWallet.BlockchainToken, isCustom: boolean) => {
    if (isUserToken(token)) {
      if (isCustom) {
        const updatedCustomToken = selected ? { ...token, visible: true } : { ...token, visible: false }
        const tokenIndex = updatedTokensList.findIndex((t) => t.contractAddress === token.contractAddress)
        let newList = [...updatedTokensList]
        newList.splice(tokenIndex, 1, updatedCustomToken)
        setUpdatedTokensList(newList)
      } else {
        setUpdatedTokensList(addOrRemoveTokenFromList(selected, token))
      }
    } else {
      if (token.isErc721) {
        setTokenContractAddress(token.contractAddress)
        setShowTokenIDRequired(true)
        setShowAddCustomToken(true)
        return
      }
      setUpdatedTokensList(addOrRemoveTokenFromList(selected, token))
    }
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

  const onRemoveAsset = (token: BraveWallet.BlockchainToken) => {
    const newUserList = updatedTokensList.filter((t) => t.contractAddress.toLowerCase() !== token.contractAddress.toLowerCase())
    setUpdatedTokensList(newUserList)
    const newFilteredTokenList = filteredTokenList.filter((t) => t.contractAddress.toLowerCase() !== token.contractAddress.toLowerCase())
    setFilteredTokenList(newFilteredTokenList)
  }

  const isDecimalDisabled = React.useMemo((): boolean => {
    return foundTokenInfoByContractAddress?.isErc721 ?? tokenID !== ''
  }, [foundTokenInfoByContractAddress, tokenID])

  React.useEffect(() => {
    if (tokenContractAddress === '') {
      setTokenName('')
      setTokenSymbol('')
      setTokenDecimals('')
      setTokenID('')
      setCoingeckoID('')
      setIconURL('')
      return
    }
    onFindTokenInfoByContractAddress(tokenContractAddress)
    if (foundTokenInfoByContractAddress) {
      setTokenName(foundTokenInfoByContractAddress.name)
      setTokenSymbol(foundTokenInfoByContractAddress.symbol)
      setTokenDecimals(foundTokenInfoByContractAddress.decimals.toString())
    }
    if (foundTokenInfoByContractAddress?.isErc721) {
      if (tokenID === '') {
        setShowTokenIDRequired(true)
        setShowAdvancedFields(true)
      }
    } else {
      setShowTokenIDRequired(false)
    }
  }, [foundTokenInfoByContractAddress, tokenID, tokenContractAddress])

  const buttonDisabled = React.useMemo((): boolean => {
    return tokenName === '' ||
      tokenSymbol === '' ||
      (tokenDecimals === '0' && tokenID === '') ||
      tokenDecimals === '' ||
      tokenContractAddress === '' ||
      !tokenContractAddress.toLowerCase().startsWith('0x')
  }, [tokenName, tokenSymbol, tokenDecimals, tokenID, tokenContractAddress])

  const onClickDone = () => {
    onUpdateVisibleAssets(updatedTokensList)
    onClose()
  }

  const onToggleShowAdvancedFields = () => {
    setShowAdvancedFields(!showAdvancedFields)
  }

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
                />
                <InputLabel>{getLocale('braveWalletWatchListTokenDecimals')}</InputLabel>
                <Input
                  value={tokenDecimals}
                  onChange={handleTokenDecimalsChanged}
                  disabled={isDecimalDisabled}
                  type='number'
                />
                <DividerRow>
                  <AdvancedButton onClick={onToggleShowAdvancedFields}>
                    <DividerText>{getLocale('braveWalletWatchListAdvanced')}</DividerText>
                  </AdvancedButton>
                  <AdvancedButton onClick={onToggleShowAdvancedFields}>
                    <AdvancedIcon rotated={showAdvancedFields} />
                  </AdvancedButton>
                </DividerRow>
                <SubDivider />
                {showAdvancedFields &&
                  <>
                    <InputLabel>{getLocale('braveWalletIconURL')}</InputLabel>
                    <Input
                      value={iconURL}
                      onChange={handleIconURLChanged}
                    />
                    <InputLabel>{getLocale('braveWalletWatchListCoingeckoId')}</InputLabel>
                    <Input
                      value={coingeckoID}
                      onChange={handleCoingeckoIDChanged}
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
                  </>
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
                  autoFocus={true}
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
                  onSubmit={onClickDone}
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
