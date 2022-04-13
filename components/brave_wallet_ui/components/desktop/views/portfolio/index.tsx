import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory } from 'react-router'

// Constants
import {
  BraveWallet,
  UserAssetInfoType,
  AddAccountNavTypes,
  WalletState,
  PageState,
  SupportedTestNetworks,
  WalletRoutes
} from '../../../../constants/types'
import { getLocale } from '../../../../../common/locale'
import { CurrencySymbols } from '../../../../utils/currency-symbols'

// Utils
import { sortTransactionByDate } from '../../../../utils/tx-utils'
import { getTokensNetwork, getTokensCoinType } from '../../../../utils/network-utils'
import Amount from '../../../../utils/amount'

// Options
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'

// Components
import { BackButton, LoadingSkeleton, withPlaceholderIcon } from '../../../shared'
import {
  ChartControlBar,
  LineChart,
  EditVisibleAssetsModal,
  WithHideBalancePlaceholder
} from '../../'

import NFTDetails from './components/nft-details'
import TokenLists from './components/token-lists'
import AccountsAndTransactionsList from './components/accounts-and-transctions-list'

// Hooks
import { useAssetManagement, useBalance, usePricing, useTokenInfo, useTransactionParser } from '../../../../common/hooks'
import { useLib } from '../../../../common/hooks/useLib'

// Styled Components
import {
  StyledWrapper,
  TopRow,
  BalanceTitle,
  BalanceText,
  AssetIcon,
  AssetRow,
  AssetColumn,
  PriceRow,
  AssetNameText,
  DetailText,
  InfoColumn,
  PriceText,
  PercentBubble,
  PercentText,
  ArrowIcon,
  BalanceRow,
  ShowBalanceButton,
  NetworkDescription
} from './style'
import { AllNetworksOption } from '../../../../options/network-filter-options'
import { mojoTimeDeltaToJSDate } from '../../../../../common/mojomUtils'
import { WalletPageActions } from '../../../../page/actions'
import { WalletActions } from '../../../../common/actions'

export interface Props {
  toggleNav: () => void
  onClickAddAccount: (tabId: AddAccountNavTypes) => () => void
  onShowVisibleAssetsModal: (showModal: boolean) => void
  showVisibleAssetsModal: boolean
}

const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 12 })

const Portfolio = (props: Props) => {
  const {
    toggleNav,
    onClickAddAccount,
    onShowVisibleAssetsModal,
    showVisibleAssetsModal
  } = props

  // routing
  const history = useHistory()

  // redux
  const {
    defaultCurrencies,
    addUserAssetError,
    userVisibleTokensInfo,
    selectedNetwork,
    portfolioPriceHistory,
    selectedPortfolioTimeline,
    accounts,
    networkList,
    transactions,
    isFetchingPortfolioPriceHistory,
    transactionSpotPrices,
    fullTokenList,
    selectedNetworkFilter
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  const {
    isFetchingPriceHistory: isLoading,
    selectedTimeline,
    selectedAsset,
    selectedAssetPriceHistory,
    selectedAssetFiatPrice,
    selectedAssetCryptoPrice
  } = useSelector(({ page }: { page: PageState }) => page)

  // custom hooks
  const { getBlockchainTokenInfo } = useLib()
  const getAccountBalance = useBalance(networkList)
  const { computeFiatAmount } = usePricing(transactionSpotPrices)
  const {
    onFindTokenInfoByContractAddress,
    foundTokenInfoByContractAddress
  } = useTokenInfo(getBlockchainTokenInfo, userVisibleTokensInfo, fullTokenList, selectedNetwork)
  const {
    onAddCustomAsset,
    onUpdateVisibleAssets
  } = useAssetManagement()

  // This will scrape all the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    const tokensCoinType = getTokensCoinType(networkList, asset)
    const amounts = accounts.filter((account) => account.coin === tokensCoinType).map((account) =>
      getAccountBalance(account, asset))

    // If a user has not yet created a FIL or SOL account,
    // we return 0 until they create an account
    if (amounts.length === 0) {
      return '0'
    }

    return amounts.reduce(function (a, b) {
      return a !== '' && b !== ''
        ? new Amount(a).plus(b).format()
        : ''
    })
  }, [accounts, networkList, getAccountBalance])

  // This looks at the users asset list and returns the full balance for each asset
  const userAssetList = React.useMemo(() => {
    const allAssets = userVisibleTokensInfo.map((asset) => ({
      asset: asset,
      assetBalance: fullAssetBalance(asset)
    }) as UserAssetInfoType)
    // By default we dont show any testnetwork assets
    if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
      return allAssets.filter((asset) => !SupportedTestNetworks.includes(asset.asset.chainId))
    }
    // If chainId is Localhost we also do a check for coinType to return
    // the correct asset
    if (selectedNetworkFilter.chainId === BraveWallet.LOCALHOST_CHAIN_ID) {
      return allAssets.filter((asset) =>
        asset.asset.chainId === selectedNetworkFilter.chainId &&
        getTokensCoinType(networkList, asset.asset) === selectedNetworkFilter.coin
      )
    }
    // Filter by all other assets by chainId's
    return allAssets.filter((asset) => asset.asset.chainId === selectedNetworkFilter.chainId)
  }, [userVisibleTokensInfo, selectedNetworkFilter, fullAssetBalance, networkList])

  // This will scrape all of the user's accounts and combine the fiat value for every asset
  const fullPortfolioFiatBalance = React.useMemo(() => {
    const visibleAssetOptions = userAssetList
      .filter((token) =>
        token.asset.visible &&
        !token.asset.isErc721
      )

    if (visibleAssetOptions.length === 0) {
      return ''
    }

    const visibleAssetFiatBalances = visibleAssetOptions
      .map((item) => {
        return computeFiatAmount(item.assetBalance, item.asset.symbol, item.asset.decimals)
      })

    const grandTotal = visibleAssetFiatBalances.reduce(function (a, b) {
      return a.plus(b)
    })
    return grandTotal.formatAsFiat()
  }, [userAssetList, computeFiatAmount])

  // state
  const [filteredAssetList, setfilteredAssetList] = React.useState<UserAssetInfoType[]>(userAssetList)
  const [hoverBalance, setHoverBalance] = React.useState<string>()
  const [hoverPrice, setHoverPrice] = React.useState<string>()
  const [hideBalances, setHideBalances] = React.useState<boolean>(false)
  const selectedAssetsNetwork = React.useMemo(() => {
    if (!selectedAsset) {
      return selectedNetwork
    }
    return getTokensNetwork(networkList, selectedAsset)
  }, [selectedNetwork, selectedAsset, networkList])

  // more custom hooks
  const parseTransaction = useTransactionParser(selectedAssetsNetwork, accounts, transactionSpotPrices, userVisibleTokensInfo)
  const { isFetchingNFTMetadata, nftMetadata } = useSelector((state: { page: PageState }) => state.page)
  const dispatch = useDispatch()

  // memos / computed

  const formattedPriceHistory = React.useMemo(() => {
    return selectedAssetPriceHistory.map((obj) => {
      return {
        date: mojoTimeDeltaToJSDate(obj.date),
        close: Number(obj.price)
      }
    })
  }, [selectedAssetPriceHistory])

  const priceHistory = React.useMemo(() => {
    if (parseFloat(fullPortfolioFiatBalance) === 0) {
      return []
    } else {
      return portfolioPriceHistory
    }
  }, [portfolioPriceHistory, fullPortfolioFiatBalance])

  const transactionsByNetwork = React.useMemo(() => {
    const accountsByNetwork = accounts.filter((account) => account.coin === selectedAssetsNetwork.coin)
    return accountsByNetwork.map((account) => {
      return transactions[account.address]
    }).flat(1)
  }, [accounts, transactions, selectedAssetsNetwork])

  const selectedAssetTransactions = React.useMemo((): BraveWallet.TransactionInfo[] => {
    if (selectedAsset) {
      const filteredTransactions = transactionsByNetwork.filter((tx) => {
        return tx && parseTransaction(tx).symbol === selectedAsset?.symbol
      })
      return sortTransactionByDate(filteredTransactions, 'descending')
    }
    return []
  }, [selectedAsset, transactionsByNetwork, parseTransaction])

  const fullAssetBalances = React.useMemo(() => {
    if (selectedAsset?.contractAddress === '') {
      return filteredAssetList.find(
        (asset) =>
          asset.asset.symbol.toLowerCase() === selectedAsset?.symbol.toLowerCase() &&
          asset.asset.chainId === selectedAsset?.chainId
      )
    }
    return filteredAssetList.find(
      (asset) =>
        asset.asset.contractAddress.toLowerCase() === selectedAsset?.contractAddress.toLowerCase() &&
        asset.asset.chainId === selectedAsset?.chainId
    )
  }, [filteredAssetList, selectedAsset])

  const formattedFullAssetBalance = fullAssetBalances?.assetBalance
    ? '(' + new Amount(fullAssetBalances?.assetBalance ?? '')
      .divideByDecimals(selectedAsset?.decimals ?? 18)
      .formatAsAsset(6, selectedAsset?.symbol ?? '') + ')'
    : ''

  const fullAssetFiatBalance = React.useMemo(() => fullAssetBalances?.assetBalance
    ? computeFiatAmount(
        fullAssetBalances.assetBalance,
        fullAssetBalances.asset.symbol,
        fullAssetBalances.asset.decimals
      )
    : Amount.empty(),
    [fullAssetBalances]
  )

  // methods
  const onChangeTimeline = (timeline: BraveWallet.AssetPriceTimeframe) => {
    if (selectedAsset) {
      dispatch(WalletPageActions.selectAsset({
        asset: selectedAsset,
        timeFrame: timeline
      }))
    } else {
      dispatch(WalletActions.selectPortfolioTimeline(timeline))
    }
  }

  const selectAsset = (asset: BraveWallet.BlockchainToken | undefined) => {
    if (asset) {
      if (asset.contractAddress === '') {
        history.push(`${WalletRoutes.Portfolio}/${asset.symbol}`)
        return
      }
      history.push(`${WalletRoutes.Portfolio}/${asset.contractAddress}`)
    } else {
      dispatch(WalletPageActions.selectAsset({ asset, timeFrame: selectedTimeline }))
      history.push(WalletRoutes.Portfolio)
    }
  }

  const onSelectAsset = (asset: BraveWallet.BlockchainToken) => () => {
    selectAsset(asset)
    toggleNav()
  }

  const goBack = () => {
    selectAsset(undefined)
    if (nftMetadata) {
      dispatch(WalletPageActions.updateNFTMetadata(undefined))
    }
    setfilteredAssetList(userAssetList)
    toggleNav()
  }

  const onUpdateBalance = (value: number | undefined) => {
    if (!selectedAsset) {
      if (value) {
        setHoverBalance(new Amount(value).formatAsFiat())
      } else {
        setHoverBalance(undefined)
      }
    } else {
      if (value) {
        setHoverPrice(new Amount(value).formatAsFiat())
      } else {
        setHoverPrice(undefined)
      }
    }
  }

  const toggleShowVisibleAssetModal = () => {
    onShowVisibleAssetsModal(!showVisibleAssetsModal)
  }

  const onToggleHideBalances = () => {
    setHideBalances(!hideBalances)
  }

  // effects
  React.useEffect(() => {
    setfilteredAssetList(userAssetList)
  }, [userAssetList])

  // render
  return (
    <StyledWrapper>
      <TopRow>
        <BalanceRow>
          {!selectedAsset ? (
            <BalanceTitle>{getLocale('braveWalletBalance')}</BalanceTitle>
          ) : (
            <BackButton onSubmit={goBack} />
          )}
        </BalanceRow>
        <BalanceRow>
          {!selectedAsset?.isErc721 &&
            <ChartControlBar
              onSubmit={onChangeTimeline}
              selectedTimeline={selectedAsset ? selectedTimeline : selectedPortfolioTimeline}
              timelineOptions={ChartTimelineOptions()}
            />
          }
          <ShowBalanceButton
            hideBalances={hideBalances}
            onClick={onToggleHideBalances}
          />
        </BalanceRow>
      </TopRow>

      {!selectedAsset ? (
        <WithHideBalancePlaceholder
          size='big'
          hideBalances={hideBalances}
        >
          <BalanceText>
            {fullPortfolioFiatBalance !== ''
              ? `${CurrencySymbols[defaultCurrencies.fiat]}${hoverBalance || fullPortfolioFiatBalance}`
              : <LoadingSkeleton width={150} height={32} />
            }
          </BalanceText>
        </WithHideBalancePlaceholder>
      ) : (
        <>
          {!selectedAsset.isErc721 &&
            <InfoColumn>
              <AssetRow>
                <AssetIconWithPlaceholder asset={selectedAsset} network={selectedAssetsNetwork} />
                <AssetColumn>
                  <AssetNameText>{selectedAsset.name}</AssetNameText>
                  <NetworkDescription>{selectedAsset.symbol} on {selectedAssetsNetwork?.chainName ?? ''}</NetworkDescription>
                </AssetColumn>
              </AssetRow>
              {/* <DetailText>{selectedAsset.name} {getLocale('braveWalletPrice')} ({selectedAsset.symbol})</DetailText> */}
              <PriceRow>
                <PriceText>{CurrencySymbols[defaultCurrencies.fiat]}{hoverPrice || (selectedAssetFiatPrice ? new Amount(selectedAssetFiatPrice.price).formatAsFiat() : 0.00)}</PriceText>
                <PercentBubble isDown={selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0 : false}>
                  <ArrowIcon isDown={selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0 : false} />
                  <PercentText>{selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange).toFixed(2) : 0.00}%</PercentText>
                </PercentBubble>
              </PriceRow>
              <DetailText>
                {
                  selectedAssetCryptoPrice
                    ? new Amount(selectedAssetCryptoPrice.price)
                      .formatAsAsset(undefined, defaultCurrencies.crypto)
                    : ''
                }
              </DetailText>
            </InfoColumn>
          }
        </>
      )}

      {!selectedAsset?.isErc721 &&
        <LineChart
          isDown={selectedAsset && selectedAssetFiatPrice ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0 : false}
          isAsset={!!selectedAsset}
          priceData={
            selectedAsset
              ? formattedPriceHistory
              : priceHistory
          }
          onUpdateBalance={onUpdateBalance}
          isLoading={selectedAsset ? isLoading : parseFloat(fullPortfolioFiatBalance) === 0 ? false : isFetchingPortfolioPriceHistory}
          isDisabled={selectedAsset ? false : parseFloat(fullPortfolioFiatBalance) === 0}
        />
      }

      {selectedAsset?.isErc721 &&
        <NFTDetails
          isLoading={isFetchingNFTMetadata}
          selectedAsset={selectedAsset}
          nftMetadata={nftMetadata}
          defaultCurrencies={defaultCurrencies}
          selectedNetwork={selectedNetwork}
        />
      }

      <AccountsAndTransactionsList
        formattedFullAssetBalance={formattedFullAssetBalance}
        fullAssetFiatBalance={fullAssetFiatBalance}
        selectedAsset={selectedAsset}
        selectedAssetTransactions={selectedAssetTransactions}
        onClickAddAccount={onClickAddAccount}
        hideBalances={hideBalances}
        networkList={networkList}
      />

      {!selectedAsset &&
        <TokenLists
          defaultCurrencies={defaultCurrencies}
          userAssetList={userAssetList}
          filteredAssetList={filteredAssetList}
          tokenPrices={transactionSpotPrices}
          networks={networkList}
          onSetFilteredAssetList={setfilteredAssetList}
          onSelectAsset={onSelectAsset}
          onShowAssetModal={toggleShowVisibleAssetModal}
          hideBalances={hideBalances}
        />
      }

      {showVisibleAssetsModal &&
        <EditVisibleAssetsModal
          fullAssetList={fullTokenList}
          userVisibleTokensInfo={userVisibleTokensInfo}
          addUserAssetError={addUserAssetError}
          onClose={toggleShowVisibleAssetModal}
          onAddCustomAsset={onAddCustomAsset}
          selectedNetwork={selectedNetwork}
          networkList={networkList}
          onFindTokenInfoByContractAddress={onFindTokenInfoByContractAddress}
          foundTokenInfoByContractAddress={foundTokenInfoByContractAddress}
          onUpdateVisibleAssets={onUpdateVisibleAssets}
        />
      }
    </StyledWrapper>
  )
}

export default Portfolio
