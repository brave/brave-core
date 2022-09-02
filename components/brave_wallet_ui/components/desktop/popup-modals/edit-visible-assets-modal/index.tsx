import * as React from 'react'
import {
  BraveWallet,
  WalletState
} from '../../../../constants/types'
import {
  PopupModal
} from '../..'
import { NavButton } from '../../../extension'
import { SearchBar } from '../../../shared'
import { getLocale } from '../../../../../common/locale'

// Styled Components
import {
  Divider,
  LoadIcon,
  LoadingWrapper,
  NoAssetButton,
  NoAssetRow,
  NoAssetText,
  StyledWrapper,
  TopRow
} from './style'

// hooks
import { useAssetManagement } from '../../../../common/hooks'
import { useSelector } from 'react-redux'
import { AddCustomTokenForm } from '../../../shared/add-custom-token-form/add-custom-token-form'
import { VirtualizedVisibleAssetsList } from './virtualized-visible-assets-list'

export interface Props {
  onClose: () => void
}

const EditVisibleAssetsModal = ({ onClose }: Props) => {
  // redux
  const userVisibleTokensInfo = useSelector(({ wallet }: { wallet: WalletState }) => wallet.userVisibleTokensInfo)
  const fullTokenList = useSelector(({ wallet }: { wallet: WalletState }) => wallet.fullTokenList)
  const selectedNetwork = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetwork)
  const networkList = useSelector(({ wallet }: { wallet: WalletState }) => wallet.networkList)
  // custom hooks
  const {
    onUpdateVisibleAssets
  } = useAssetManagement()

  // Token List States
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [filteredTokenList, setFilteredTokenList] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [updatedTokensList, setUpdatedTokensList] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [tokenContractAddress, setTokenContractAddress] = React.useState<string>('')

  // Modal UI States
  const [showAddCustomToken, setShowAddCustomToken] = React.useState<boolean>(false)
  const [isLoading, setIsLoading] = React.useState<boolean>(false)

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

  const nativeAsset = React.useMemo(() => {
    return {
      contractAddress: '',
      decimals: selectedNetwork.decimals,
      isErc20: false,
      isErc721: false,
      logo: selectedNetwork.iconUrls[0] ?? '',
      name: selectedNetwork.symbolName,
      symbol: selectedNetwork.symbol,
      visible: true,
      tokenId: '',
      coingeckoId: '',
      chainId: selectedNetwork.chainId,
      coin: selectedNetwork.coin
    }
  }, [selectedNetwork])

  const tokenList = React.useMemo(() => {
    const userVisibleContracts = isUserVisibleTokensInfoEmpty
      ? []
      : userVisibleTokensInfo.map((token) => token.contractAddress.toLowerCase())

    const fullAssetsListPlusNativeToken = userVisibleContracts.includes('')
      ? fullTokenList
      : [nativeAsset, ...fullTokenList]

    const filteredTokenRegistry = fullAssetsListPlusNativeToken
      .filter((token) => !userVisibleContracts.includes(token.contractAddress.toLowerCase()))

    return isUserVisibleTokensInfoEmpty
      ? filteredTokenRegistry
      : [...userVisibleTokensInfo, ...filteredTokenRegistry]
  }, [isUserVisibleTokensInfoEmpty, fullTokenList, userVisibleTokensInfo, nativeAsset])

  React.useEffect(() => {
    // Added this timeout to throttle setting the list
    // to allow the modal to appear instantly
    const timeoutId = setTimeout(function () {
      setFilteredTokenList(tokenList)
      setIsLoading(false)
    }, 500)

    return () => clearTimeout(timeoutId)
  }, [tokenList])

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

  const findUpdatedTokenInfo = React.useCallback((token: BraveWallet.BlockchainToken) => {
    return updatedTokensList.find((t) =>
      t.symbol.toLowerCase() === token.symbol.toLowerCase() &&
      t.contractAddress.toLowerCase() === token.contractAddress.toLowerCase() &&
      t.chainId === token.chainId)
  }, [updatedTokensList])

  const isUserToken = React.useCallback((token: BraveWallet.BlockchainToken) => {
    return updatedTokensList.some(t =>
    (
      t.contractAddress.toLowerCase() === token.contractAddress.toLowerCase() &&
      t.chainId === token.chainId &&
      t.symbol.toLowerCase() === token.symbol.toLowerCase())
    )
  }, [updatedTokensList])

  const isAssetSelected = React.useCallback((token: BraveWallet.BlockchainToken): boolean => {
    return (isUserToken(token) && findUpdatedTokenInfo(token)?.visible) ?? false
  }, [isUserToken, findUpdatedTokenInfo])

  const isCustomToken = React.useCallback((token: BraveWallet.BlockchainToken): boolean => {
    if (token.contractAddress === '') {
      return false
    }

    return !fullTokenList
      .some(each => each.contractAddress.toLowerCase() === token.contractAddress.toLowerCase())
  }, [fullTokenList])

  const addOrRemoveTokenFromList = React.useCallback((selected: boolean, token: BraveWallet.BlockchainToken) => {
    if (selected) {
      return [...updatedTokensList, token]
    }
    return updatedTokensList.filter((t) => t !== token)
  }, [updatedTokensList])

  const onCheckWatchlistItem = React.useCallback((key: string, selected: boolean, token: BraveWallet.BlockchainToken, isCustom: boolean) => {
    if (isUserToken(token)) {
      if (isCustom || token.contractAddress === '') {
        const updatedToken = selected ? { ...token, visible: true } : { ...token, visible: false }
        const tokenIndex = updatedTokensList.findIndex((t) =>
          t.contractAddress.toLowerCase() === token.contractAddress.toLowerCase() &&
          t.symbol.toLowerCase() === token.symbol.toLowerCase() &&
          t.chainId === token.chainId)
        let newList = [...updatedTokensList]
        newList.splice(tokenIndex, 1, updatedToken)
        setUpdatedTokensList(newList)
      } else {
        if (token.isErc721) setTokenContractAddress(token.contractAddress)
        setUpdatedTokensList(addOrRemoveTokenFromList(selected, token))
      }
    } else {
      setUpdatedTokensList(addOrRemoveTokenFromList(selected, token))
    }
  }, [isUserToken, updatedTokensList, addOrRemoveTokenFromList])

  const toggleShowAddCustomToken = () => setShowAddCustomToken(prev => !prev)

  const onClickSuggestAdd = React.useCallback(() => {
    setTokenContractAddress(searchValue)
    toggleShowAddCustomToken()
  }, [searchValue, toggleShowAddCustomToken])

  const onRemoveAsset = React.useCallback((token: BraveWallet.BlockchainToken) => {
    const filterFn = (t: BraveWallet.BlockchainToken) => !(t.contractAddress.toLowerCase() === token.contractAddress.toLowerCase() && t.tokenId === token.tokenId)
    const newUserList = updatedTokensList.filter(filterFn)
    setUpdatedTokensList(newUserList)
    const newFilteredTokenList = filteredTokenList.filter(filterFn)
    setFilteredTokenList(newFilteredTokenList)
  }, [updatedTokensList, filteredTokenList])

  const onClickDone = React.useCallback(() => {
    onUpdateVisibleAssets(updatedTokensList)
    onClose()
  }, [updatedTokensList, onUpdateVisibleAssets, onClose])

  return (
    <PopupModal
      title={showAddCustomToken
        ? getLocale('braveWalletWatchlistAddCustomAsset')
        : getLocale('braveWalletAccountsEditVisibleAssets')
      }
      onClose={onClose}
    >
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
            {showAddCustomToken
              ? <AddCustomTokenForm
                contractAddress={tokenContractAddress}
                onHideForm={toggleShowAddCustomToken}
              />
              : <>
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
                {filteredTokenList.length === 0
                  ? <NoAssetRow>
                      {searchValue.toLowerCase().startsWith('0x') ? (
                        <NoAssetButton
                          onClick={onClickSuggestAdd}>{getLocale('braveWalletWatchListSuggestion').replace('$1', searchValue)}</NoAssetButton>
                      ) : (
                        <NoAssetText>{getLocale('braveWalletWatchListNoAsset')} {searchValue}</NoAssetText>
                      )}
                  </NoAssetRow>
                  : <VirtualizedVisibleAssetsList
                    tokenList={filteredTokenList}
                    networkList={networkList}
                    isCustomToken={isCustomToken}
                    onRemoveAsset={onRemoveAsset}
                    isAssetSelected={isAssetSelected}
                    onCheckWatchlistItem={onCheckWatchlistItem}
                  />
                }
                <NavButton
                  onSubmit={onClickDone}
                  text={getLocale('braveWalletWatchListDoneButton')}
                  buttonType='primary'
                />
              </>
            }
          </>
        )}
      </StyledWrapper>
    </PopupModal>
  )
}

export default EditVisibleAssetsModal
