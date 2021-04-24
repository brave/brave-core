import * as React from 'react'

// Constants
import {
  ChartTimelineType,
  PriceDataObjectType,
  AssetOptionType,
  RPCResponseType
} from '../../../../constants/types'
import locale from '../../../../constants/locale'

// Utils
import { formatePrices } from '../../../../utils/format-prices'

// Options
import { ChartTimelineOptions } from '../../../../options/chart-timeline-options'
import { AssetOptions } from '../../../../options/asset-options'

// Components
import { SearchBar } from '../../../shared'
import {
  ChartControlBar,
  LineChart,
  PortfolioAssetItem,
  AddButton,
  PortfolioAccountItem,
  PortfolioTransactionItem
} from '../../'

// Mock Data
import { PriceHistoryMockData } from '../../../../stories/mock-data/price-history-data'
import { mockRPCResponse } from '../../../../stories/mock-data/rpc-response'
import { mockUserAccounts } from '../../../../stories/mock-data/user-accounts'
import { mockUserWalletPreferences } from '../../../../stories/mock-data/user-wallet-preferences'
import { CurrentPriceMockData } from '../../../../stories/mock-data/current-price-data'

// Styled Components
import {
  StyledWrapper,
  TopRow,
  BalanceTitle,
  BalanceText,
  ButtonRow,
  BackButton,
  BackIcon,
  AssetIcon,
  AssetRow,
  PriceRow,
  AssetNameText,
  DetailText,
  InfoColumn,
  PriceText,
  DividerText,
  SubDivider,
  PercentBubble,
  PercentText,
  ArrowIcon
} from './style'

export interface Props {
  toggleNav: () => void
}

interface MockAPIResponse {
  usd: string
  btc: number
  change24Hour: number
}

const Portfolio = (props: Props) => {
  const { toggleNav } = props
  const createdAssetList = React.useMemo(() => {
    const userList = mockUserWalletPreferences.viewableAssets
    return AssetOptions.filter((asset) => userList.includes(asset.id))
  }, [mockUserWalletPreferences.viewableAssets])
  const [userAssetList, setUserAssetList] = React.useState<AssetOptionType[]>(createdAssetList)
  const [selectedTimeline, setSelectedTimeline] = React.useState<ChartTimelineType>('24HRS')
  const [priceData, setPriceData] = React.useState<PriceDataObjectType[]>(PriceHistoryMockData.slice(15, 20))
  const [selectedAsset, setSelectedAsset] = React.useState<AssetOptionType>()

  // This will change once we hit a real api for pricing
  const timeline = (path: ChartTimelineType) => {
    switch (path) {
      case '5MIN':
        return 17
      case '24HRS':
        return 15
      case '7Day':
        return 12
      case '1Month':
        return 10
      case '3Months':
        return 8
      case '1Year':
        return 4
      case 'AllTime':
        return 0
    }
  }

  // This updates the price chart timeline
  const changeTimeline = (path: ChartTimelineType) => {
    setPriceData(PriceHistoryMockData.slice(timeline(path), 20))
    setSelectedTimeline(path)
  }

  // This returns the price info of an asset, will change once we have an api
  const priceApi = (asset: AssetOptionType) => {
    const data = CurrentPriceMockData.find((coin) => coin.symbol === asset.symbol)
    const usdValue = data ? data.usd : 0
    const btcValue = data ? data.btc : 0
    const change24Hour = data ? data.change24Hour : 0
    const response: MockAPIResponse = {
      usd: formatePrices(usdValue),
      btc: btcValue,
      change24Hour: change24Hour
    }
    return response
  }

  // This returns info about a single asset
  const assetInfo = (account: RPCResponseType) => {
    return account.assets.find((a) => a.id === selectedAsset?.id)
  }

  // This calculates the fiat value of a single accounts asset balance
  const singleAccountFiatBalance = (account: RPCResponseType) => {
    const asset = assetInfo(account)
    const data = CurrentPriceMockData.find((coin) => coin.symbol === asset?.symbol)
    const value = data ? asset ? asset.balance * data.usd : 0 : 0
    return formatePrices(value)
  }

  // This returns the balance of a single accounts asset
  const singleAccountBalance = (account: RPCResponseType) => {
    const balance = assetInfo(account)?.balance
    return balance ? balance.toString() : ''
  }

  // This returns a list of accounts with a balance of the selected asset
  const filteredAccountsByAsset = React.useMemo(() => {
    const id = selectedAsset ? selectedAsset.id : ''
    const list = mockRPCResponse.filter((account) => account.assets.map((assetID) => assetID.id).includes(id))
    return list
  }, [selectedAsset])

  // This will return the name of an account from user preferences
  const accountName = (asset: RPCResponseType) => {
    const foundName = mockUserAccounts.find((account) => account.address === asset.address)?.name
    const name = foundName ? foundName : locale.account
    return name
  }

  // This filters a list of assets when the user types in search bar
  const filterAssets = (event: any) => {
    const search = event.target.value
    if (search === '') {
      setUserAssetList(createdAssetList)
    } else {
      const filteredList = createdAssetList.filter((asset) => {
        return (
          asset.name.toLowerCase() === search.toLowerCase() ||
          asset.name.toLowerCase().startsWith(search.toLowerCase()) ||
          asset.symbol.toLocaleLowerCase() === search.toLowerCase() ||
          asset.symbol.toLowerCase().startsWith(search.toLowerCase())
        )
      })
      setUserAssetList(filteredList)
    }
  }

  // This returns a list of transactions from all accounts filtered by selected asset
  const transactions = React.useMemo(() => {
    const response = mockRPCResponse
    const transactionList = response.map((account) => {
      const id = selectedAsset ? selectedAsset.id : ''
      return account.transactions.find((item) => item.assetId === id)
    })
    const removedEmptyTransactions = transactionList.filter(x => x)
    return removedEmptyTransactions
  }, [selectedAsset])

  // This will scrape all of the user's accounts and combine the balances for a single asset
  const scrapedFullAssetBalance = (asset: AssetOptionType) => {
    const response = mockRPCResponse
    const amounts = response.map((account) => {
      const balance = account.assets.find((item) => item.id === asset.id)?.balance
      return balance ? balance : 0
    })
    const grandTotal = amounts.reduce(function (a, b) {
      return a + b
    }, 0)
    return grandTotal
  }

  // This will scrape all of the user's accounts and combine the fiat value for a single asset
  const scrapedFullAssetFiatBalance = (asset: AssetOptionType) => {
    const fullBallance = scrapedFullAssetBalance(asset)
    const price = CurrentPriceMockData.find((coin) => coin.symbol === asset?.symbol)?.usd
    const value = price ? price * fullBallance : 0
    return value
  }

  // This will scrape all of the user's accounts and combine the fiat value for every asset
  const scrapedFullPortfolioBalance = () => {
    const amountList = createdAssetList.map((asset) => {
      return scrapedFullAssetFiatBalance(asset)
    })
    const grandTotal = amountList.reduce(function (a, b) {
      return a + b
    }, 0)
    return formatePrices(grandTotal)
  }

  const addCoin = () => {
    alert('Will Show New Coins To Add!!')
  }

  const addAccount = () => {
    alert('Will Show Add Account!!')
  }

  const moreDetails = () => {
    alert('Will Show More Details Popover!!')
  }

  const selectAsset = (asset: AssetOptionType) => () => {
    setSelectedAsset(asset)
    toggleNav()
  }

  const goBack = () => {
    setSelectedAsset(undefined)
    setUserAssetList(createdAssetList)
    toggleNav()
  }

  return (
    <StyledWrapper>
      <TopRow>
        {!selectedAsset ? (
          <BalanceTitle>{locale.balance}</BalanceTitle>
        ) : (
          <BackButton onClick={goBack}><BackIcon />{locale.back}</BackButton>
        )}
        <ChartControlBar
          onSubmit={changeTimeline}
          selectedTimeline={selectedTimeline}
          timelineOptions={ChartTimelineOptions}
        />
      </TopRow>
      {!selectedAsset ? (
        <BalanceText>${scrapedFullPortfolioBalance()}</BalanceText>
      ) : (
        <InfoColumn>
          <AssetRow>
            <AssetIcon icon={selectedAsset.icon} />
            <AssetNameText>{selectedAsset.name}</AssetNameText>
          </AssetRow>
          <DetailText>{selectedAsset.name} {locale.price} ({selectedAsset.symbol})</DetailText>
          <PriceRow>
            <PriceText>${priceApi(selectedAsset).usd}</PriceText>
            <PercentBubble isDown={priceApi(selectedAsset).change24Hour < 0}>
              <ArrowIcon isDown={priceApi(selectedAsset).change24Hour < 0} />
              <PercentText>{priceApi(selectedAsset).change24Hour}%</PercentText>
            </PercentBubble>
          </PriceRow>
          <DetailText>{priceApi(selectedAsset).btc} BTC</DetailText>
        </InfoColumn>
      )}
      <LineChart priceData={priceData} />
      {selectedAsset &&
        <>
          <DividerText>{locale.accounts}</DividerText>
          <SubDivider />
          {filteredAccountsByAsset.map((account) =>
            <PortfolioAccountItem
              key={account.address}
              action={moreDetails}
              assetTicker={selectedAsset.symbol}
              name={accountName(account)}
              address={account.address}
              fiatBalance={singleAccountFiatBalance(account)}
              assetBalance={singleAccountBalance(account)}
            />
          )}
          <ButtonRow>
            <AddButton
              buttonType='secondary'
              onSubmit={addAccount}
              text={locale.addAccount}
            />
          </ButtonRow>
          <DividerText>{locale.transactions}</DividerText>
          <SubDivider />
          {transactions.map((transaction) =>
            <PortfolioTransactionItem
              action={moreDetails}
              key={transaction?.hash}
              amount={transaction?.amount ? transaction.amount : 0}
              from={transaction?.from ? transaction.from : ''}
              to={transaction?.to ? transaction.to : ''}
              ticker={selectedAsset.symbol}
            />
          )}
        </>
      }
      {!selectedAsset &&
        <>
          <SearchBar placeholder={locale.searchText} action={filterAssets} />
          {userAssetList.map((asset) =>
            <PortfolioAssetItem
              action={selectAsset(asset)}
              key={asset.id}
              name={asset.name}
              assetBalance={scrapedFullAssetBalance(asset)}
              fiatBalance={formatePrices(scrapedFullAssetFiatBalance(asset))}
              symbol={asset.symbol}
              icon={asset.icon}
            />
          )}
          <ButtonRow>
            <AddButton
              buttonType='secondary'
              onSubmit={addCoin}
              text={locale.addCoin}
            />
          </ButtonRow>
        </>
      }
    </StyledWrapper>
  )
}

export default Portfolio
