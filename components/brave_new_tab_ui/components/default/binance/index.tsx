/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import createWidget from '../widget/index'
import {
  WidgetWrapper,
  Copy,
  BuyPromptWrapper,
  FiatInputWrapper,
  FiatInputField,
  FiatDropdown,
  CaratDropdown,
  AssetDropdown,
  AssetDropdownLabel,
  ActionsWrapper,
  ConnectButton,
  Header,
  StyledTitle,
  BinanceIcon,
  StyledTitleText,
  AssetItems,
  AssetItem,
  TLDSwitchWrapper,
  TLDSwitch,
  ActionTray,
  ActionItem,
  ConnectPrompt,
  DisconnectWrapper,
  DisconnectTitle,
  DisconnectCopy,
  DisconnectButton,
  DismissAction,
  InvalidWrapper,
  InvalidTitle,
  StyledEmoji,
  InvalidCopy,
  GenButton,
  ListItem,
  DetailIcons,
  BackArrow,
  ListImg,
  AssetTicker,
  AssetLabel,
  AssetQR,
  DetailArea,
  MemoArea,
  MemoInfo,
  DetailLabel,
  DetailInfo,
  CopyButton,
  ListIcon,
  SearchInput,
  ListLabel,
  BTCSummary,
  ListInfo,
  TradeLabel,
  Balance,
  Converted,
  BlurIcon,
  ConvertInfoWrapper,
  ConvertInfoItem,
  ConvertValue,
  ConvertLabel,
  AvailableLabel,
  BTCValueLabel,
  OtherValueLabel,
  NavigationBar,
  NavigationItem,
  SelectedView,
  TickerLabel
} from './style'
import {
  DisconnectIcon,
  ShowIcon,
  HideIcon
} from './assets/icons'
import { StyledTitleTab } from '../widgetTitleTab'
import currencyData from './data'
import BinanceLogo from './assets/binance-logo'
import { CaratLeftIcon, CaratDownIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../common/locale'
import searchIcon from './assets/search-icon.png'
import bchIcon from './assets/bch.png'
import bnbIcon from './assets/bnb.png'
import btcIcon from './assets/btc.png'
import busdIcon from './assets/busd.png'
import ethIcon from './assets/eth.png'
import xrpIcon from './assets/xrp.png'
import partyIcon from './assets/party.png'
import { getUSDPrice } from '../../../binance-utils'

interface State {
  fiatShowing: boolean
  currenciesShowing: boolean
  disconnectInProgress: boolean
  selectedView: string
  currentDepositSearch: string
  currentDepositAsset: string
  currentTradeSearch: string
  currentTradeAsset: string
  currentTradeAmount: string
  currentConvertAmount: string
  currentConvertFrom: string
  currentConvertTo: string
  insufficientFunds: boolean
  showConvertPreview: boolean
  convertSuccess: boolean
  isBuyView: boolean
}

interface Props {
  initialAmount: string
  initialFiat: string
  initialAsset: string
  showContent: boolean
  userTLDAutoSet: boolean
  userTLD: NewTab.BinanceTLD
  userAuthed: boolean
  authInProgress: boolean
  hideBalance: boolean
  btcBalanceValue: string
  accountBalances: Record<string, string>
  assetUSDValues: Record<string, string>
  assetBTCValues: Record<string, string>
  assetBTCVolumes: Record<string, string>
  btcPrice: string
  btcVolume: string
  binanceClientUrl: string
  onShowContent: () => void
  onBuyCrypto: (coin: string, amount: string, fiat: string) => void
  onBinanceUserTLD: (userTLD: NewTab.BinanceTLD) => void
  onSetInitialFiat: (initialFiat: string) => void
  onSetInitialAmount: (initialAmount: string) => void
  onSetInitialAsset: (initialAsset: string) => void
  onSetUserTLDAutoSet: () => void
  onSetHideBalance: (hide: boolean) => void
  onBinanceBalances: (balances: Record<string, string>) => void
  onBinanceClientUrl: (clientUrl: string) => void
  onDisconnectBinance: () => void
  onConnectBinance: () => void
  onValidAuthCode: () => void
}

class Binance extends React.PureComponent<Props, State> {
  private fiatList: string[]
  private usCurrencies: string[]
  private comCurrencies: string[]
  private currencyNames: Record<string, string>

  constructor (props: Props) {
    super(props)
    this.state = {
      fiatShowing: false,
      currenciesShowing: false,
      disconnectInProgress: false,
      selectedView: 'deposit',
      currentDepositSearch: '',
      currentDepositAsset: '',
      currentTradeSearch: '',
      currentTradeAsset: '',
      currentTradeAmount: '',
      currentConvertAmount: '',
      currentConvertFrom: 'BTC',
      currentConvertTo: '',
      insufficientFunds: false,
      showConvertPreview: false,
      convertSuccess: false,
      isBuyView: true
    }
    this.fiatList = currencyData.fiatList
    this.usCurrencies = currencyData.usCurrencies
    this.comCurrencies = currencyData.comCurrencies
    this.currencyNames =  {
      'BTC': 'Bitcoin',
      'ETH': 'Ethereum',
      'XRP': 'Ripple',
      'BNB': 'Binance Coin',
      'BCH': 'Bitcoin Cash',
      'BUSD': 'US Dollar'     
    }
  }

  async componentDidMount () {
    const { userTLDAutoSet } = this.props

    if (this.props.userAuthed) {
      this.fetchBalance()
    }

    if (this.props.authInProgress) {
      this.checkForOauthCode()
    }

    if (!userTLDAutoSet) {
      chrome.binance.getUserTLD((userTLD: NewTab.BinanceTLD) => {
        this.props.onBinanceUserTLD(userTLD)
        this.props.onSetUserTLDAutoSet()
      })
    }

    chrome.binance.getClientUrl((clientUrl: string) => {
      this.props.onBinanceClientUrl(clientUrl)
    })
  }

  componentDidUpdate (prevProps: Props) {
    if (!prevProps.userAuthed && this.props.userAuthed) {
      this.fetchBalance()
    }
  }

  checkForOauthCode = () => {
    const params = window.location.search
    const urlParams = new URLSearchParams(params)
    const authCode = urlParams.get('code')

    if (authCode) {
      chrome.binance.getAccessToken(authCode, (success: boolean) => {
        if (success) {
          this.props.onValidAuthCode()
        }
      })
    }
  }

  fetchBalance = () => {
    // To Do
  }

  disconnectBinance = () => {
    this.setState({
      disconnectInProgress: true
    })
  }

  connectBinance = () => {
    const { binanceClientUrl } = this.props
    window.open(binanceClientUrl, '_blank')
    this.props.onConnectBinance()
  }

  cancelDisconnect = () => {
    this.setState({
      disconnectInProgress: false
    })
  }

  cancelConvert = () => {
    this.setState({
      insufficientFunds: false,
      showConvertPreview: false,
      currentConvertAmount: '',
      currentConvertTo: '',
      currentConvertFrom: ''
    })
  }

  finishDisconnect = () => {
    this.props.onDisconnectBinance()
    this.cancelDisconnect()
  }

  renderRoutes = () => {
    const { selectedView } = this.state
    const { userAuthed } = this.props

    if (userAuthed) {
      if (selectedView === 'buy') {
        return this.renderBuyView()
      }

      if (selectedView === 'convert') {
        return this.renderConvertView()
      }

      return this.renderAccountView()
    }

    return this.renderBuyView()
  }

  onSetHideBalance = () => {
    this.props.onSetHideBalance(
      !this.props.hideBalance
    )
  }

  setSelectedView (view: string) {
    this.setState({
      selectedView: view
    })
  }

  setCurrentDepositAsset (asset: string) {
    this.setState({
      currentDepositAsset: asset
    })
  }

  setCurrentConvertAsset (asset: string) {
    this.setState({
      currentConvertTo: asset,
      currenciesShowing: false
    })
  }

  getAssetIcon (asset: string) {
    switch (asset) {
      case 'BTC':
        return btcIcon
      case 'ETH':
        return ethIcon
      case 'BCH':
        return bchIcon
      case 'BNB':
        return bnbIcon
      case 'BUSD':
        return busdIcon
      case 'XRP':
        return xrpIcon
      default:
        return null
    }
  }

  setIsBuyView (isBuyView: boolean) {
    this.setState({ isBuyView })
  }

  processConvert = () => {
    const { accountBalances } = this.props
    const { currentConvertAmount } = this.state

    if (parseInt(currentConvertAmount) > parseInt(accountBalances['BTC'])) {
      this.setState({ insufficientFunds: true })
    } else {
      // Temporary
      setTimeout(() => {
        this.setState({ convertSuccess: true })
      }, 1500)
    }
  }

  setInitialAsset (asset: string) {
    this.setState({
      currenciesShowing: false
    })
    this.props.onSetInitialAsset(asset)
  }

  setInitialFiat (fiat: string) {
    this.setState({
      fiatShowing: false
    })
    this.props.onSetInitialFiat(fiat)
  }

  handleFiatChange = () => {
    const { userTLD } = this.props

    if (userTLD === 'us' || this.state.currenciesShowing) {
      return
    }

    this.setState({
      fiatShowing: !this.state.fiatShowing
    })
  }

  toggleCurrenciesShowing = () => {
    this.setState({ currenciesShowing: !this.state.currenciesShowing })
  }

  setInitialAmount = (e: any) => {
    const { value } = e.target

    if (isNaN(parseInt(value, 10)) && value.length > 0) {
      return
    }

    this.props.onSetInitialAmount(e.target.value)
  }

  toggleTLD = () => {
    const { userTLD } = this.props
    const newTLD = userTLD === 'com' ? 'us' : 'com'

    this.props.onBinanceUserTLD(newTLD)

    if (newTLD === 'us') {
      this.props.onSetInitialFiat('USD')
    }

    this.setState({
      fiatShowing: false,
      currenciesShowing: false
    })
  }

  renderTitle () {
    const { selectedView } = this.state
    const { showContent, userAuthed, authInProgress } = this.props    

    return (
      <Header>
        <StyledTitle>
          <BinanceIcon>
            <BinanceLogo />
          </BinanceIcon>
          <StyledTitleText>
            {'Binance'}
          </StyledTitleText>
        </StyledTitle>
        {
            userAuthed && selectedView !== 'buy' && selectedView !== 'convert'
            ? <ActionTray>
                <ActionItem onClick={this.disconnectBinance}>
                  <DisconnectIcon />
                </ActionItem>
              </ActionTray>
            : !userAuthed && !authInProgress && showContent
              ? <ConnectPrompt onClick={this.connectBinance}>
                  {'Connect'}
                </ConnectPrompt>
              : null
          }
      </Header>
    )
  }

  renderTitleTab () {
    const { onShowContent } = this.props

    return (
      <StyledTitleTab onClick={onShowContent}>
        {this.renderTitle()}
      </StyledTitleTab>
    )
  }

  renderDisconnectView = () => {
    return (
      <DisconnectWrapper>
        <DisconnectTitle>
          {getLocale('binanceWidgetDisconnectTitle')}
        </DisconnectTitle>
        <DisconnectCopy>
          {getLocale('binanceWidgetDisconnectText')}
        </DisconnectCopy>
        <DisconnectButton onClick={this.finishDisconnect}>
          {getLocale('binanceWidgetDisconnectButton')}
        </DisconnectButton>
        <DismissAction onClick={this.cancelDisconnect}>
          {getLocale('binanceWidgetCancelText')}
        </DismissAction>
      </DisconnectWrapper>
    )
  }

  renderConvertSuccess = () => {
    const { currentConvertAmount, currentConvertFrom, currentConvertTo } = this.state

    return (
      <InvalidWrapper>
        <StyledEmoji>
          <img src={partyIcon} />
        </StyledEmoji>
        <InvalidTitle>
          {`You converted ${currentConvertAmount} BTC ${currentConvertFrom} to ${currentConvertTo}!`}
        </InvalidTitle>
        <ConnectButton isSmall={true} onClick={() => {
            this.setState({
              convertSuccess: false,
              showConvertPreview: false,
              selectedView: 'deposit'
            })
          }}>
            {'Continue'}
        </ConnectButton>
      </InvalidWrapper>
    )
  }

  renderUnableToConvertView = () => {
    return (
      <InvalidWrapper>
        <InvalidTitle>
          {'Unable to Convert'}
        </InvalidTitle>
        <InvalidCopy>
          {'The amount you entered exceeds the amount you currently have available'}
        </InvalidCopy>
        <GenButton onClick={this.cancelConvert}>
          {'Retry'}
        </GenButton>
      </InvalidWrapper>
    )    
  }

  renderCurrentDepositAsset = () => {
    const { currentDepositAsset } = this.state

    return (
      <>
        <ListItem>
          <DetailIcons>
            <BackArrow>
              <CaratLeftIcon onClick={() => {
                this.setCurrentDepositAsset.bind(this, '')
              }}/>
            </BackArrow>
            <ListImg src={this.getAssetIcon(currentDepositAsset)} />
          </DetailIcons>
          <AssetTicker>
            {currentDepositAsset}
          </AssetTicker>
          <AssetLabel>
            {`(${this.currencyNames[currentDepositAsset]})`}
          </AssetLabel>
          <AssetQR>
          </AssetQR>
        </ListItem>
        <DetailArea>
          <MemoArea>
            <MemoInfo>
              <DetailLabel>
                {`${currentDepositAsset} Memo`}
              </DetailLabel>
              <DetailInfo>
                {`XXXXXXX`}
              </DetailInfo>
            </MemoInfo>
            <CopyButton>
              {'Copy'}
            </CopyButton>
          </MemoArea>
          <MemoArea>
            <MemoInfo>
              <DetailLabel>
                {`${currentDepositAsset} Deposit Address`}
              </DetailLabel>
              <DetailInfo>
                {`XXXXXX`}
              </DetailInfo>
            </MemoInfo>
          </MemoArea>
        </DetailArea>
      </>   
    )
  }

  renderDepositView = () => {
    const { currencyNames } = this
    const { userTLD } = this.props
    const { currentDepositSearch, currentDepositAsset } = this.state
    const currencyList = userTLD === 'us' ? this.usCurrencies : this.comCurrencies

    if (currentDepositAsset) {
      return this.renderCurrentDepositAsset()
    }

    return (
      <>
        <ListItem>
          <ListIcon/>
          <SearchInput
            type={'text'}
            placeholder={'Search'}
            onChange={({ target }) => {
              this.setState({ currentDepositSearch: target.value })
            }}
          />
        </ListItem>
        {currencyList.map((asset: string) => {
          const cleanName = currencyNames[asset]
          const lowerAsset = asset.toLowerCase()
          const lowerName = cleanName.toLowerCase()
          const lowerSearch = currentDepositSearch.toLowerCase()

          if (lowerAsset.indexOf(lowerSearch) < 0 &&
              lowerName.indexOf(lowerSearch) < 0) {
            return null
          }

          return (
            <ListItem
              key={`list-${asset}`}
              onClick={this.setCurrentDepositAsset.bind(this, asset)}
            >
              <ListIcon>
                <ListImg src={this.getAssetIcon(asset)} />
              </ListIcon>
              <ListLabel>
                {`${asset} (${currencyNames[asset]})`}
              </ListLabel>
            </ListItem>
          )
        })}
      </>
    )
  }

  renderSummaryView = () => {
    const { accountBalances, btcBalanceValue, hideBalance, userTLD } = this.props
    const currencyList = userTLD === 'us' ? this.usCurrencies : this.comCurrencies

    return (
      <>
        <BTCSummary>
          <ListInfo position={'left'}>
            <TradeLabel>
              <Balance isBTC={true} hideBalance={hideBalance}>
                {accountBalances['BTC']} <TickerLabel>{getLocale('binanceWidgetBTCTickerText')}</TickerLabel>
              </Balance>
              <Converted hideBalance={hideBalance}>
                {`= $${btcBalanceValue}`}
              </Converted>
            </TradeLabel>
          </ListInfo>
          <ListInfo position={'right'}>
            <TradeLabel>
              <BlurIcon onClick={this.onSetHideBalance}>
                {
                  hideBalance
                  ? <ShowIcon />
                  : <HideIcon />
                }
              </BlurIcon>
            </TradeLabel>
          </ListInfo>          
        </BTCSummary>
        {currencyList.map((asset: string) => {
          return (
            <ListItem key={`list-${asset}`}>
              <ListInfo isAsset={true} position={'left'}>
                <ListIcon>
                  <ListImg src={this.getAssetIcon(asset)} />
                </ListIcon>
                <ListLabel>
                  {asset}
                </ListLabel>
              </ListInfo>
              <ListInfo position={'right'}>
                <Balance isBTC={false} hideBalance={hideBalance}>
                  {accountBalances[asset]}
                </Balance>
                <Converted isBTC={false} hideBalance={hideBalance}>
                  {`= $${btcBalanceValue}`}
                </Converted>                
              </ListInfo>
            </ListItem>
          )
        })}
      </>
    )
  }

  renderConvertConfirm = () => {
    const { currentConvertAmount, currentConvertTo } = this.state
    const receivedAmount = currentConvertAmount === '6' ? '0': '0'

    return (
      <InvalidWrapper>
        <InvalidTitle>
          {'Confirm Conversion'}
        </InvalidTitle>
        <ConvertInfoWrapper>
          <ConvertInfoItem>
            <ConvertLabel>{'Convert'}</ConvertLabel>
            <ConvertValue>{`${currentConvertAmount} BTC`}</ConvertValue>
          </ConvertInfoItem>
          <ConvertInfoItem>
            <ConvertLabel>{'Rate'}</ConvertLabel>
            <ConvertValue>{`1 BTC = X ${currentConvertTo}`}</ConvertValue>
          </ConvertInfoItem>
          <ConvertInfoItem>
            <ConvertLabel>{'Fee'}</ConvertLabel>
            <ConvertValue>{'0.0005 BNB'}</ConvertValue>
          </ConvertInfoItem>
          <ConvertInfoItem isLast={true}>
            <ConvertLabel>{'You will receive'}</ConvertLabel>
            <ConvertValue>{`${receivedAmount} ${currentConvertTo}`}</ConvertValue>
          </ConvertInfoItem>
        </ConvertInfoWrapper>
        <ActionsWrapper>
          <ConnectButton isSmall={true} onClick={this.processConvert}>
            {`Confirm`}
          </ConnectButton>
          <DismissAction onClick={() => {
            this.setState({
              currentConvertFrom: '',
              currentConvertTo: '',
              showConvertPreview: false
            })
          }}>
            {'Cancel'}
          </DismissAction>
        </ActionsWrapper>
      </InvalidWrapper>
    )   
  }

  renderConvertView = () => {
    const { accountBalances, userTLD } = this.props
    const { currentConvertAmount, currentConvertTo } = this.state
    const currencyList = userTLD === 'us' ? this.usCurrencies : this.comCurrencies

    return (
      <>
        <Copy>
          {'Convert'}
        </Copy>
        <AvailableLabel>
          {`Available ${accountBalances['BTC']} BTC`}
        </AvailableLabel>
        <BuyPromptWrapper>
          <FiatInputWrapper>
            <FiatInputField
              type={'text'}
              placeholder={'I want to convert...'}
              value={currentConvertAmount}
              onChange={({ target }) => {
                this.setState({ currentConvertAmount: target.value })
              }}
            />
            <FiatDropdown>
              <span>
                {'BTC'}
              </span>
              <CaratDropdown>
                <CaratDownIcon />
              </CaratDropdown>
            </FiatDropdown>
          </FiatInputWrapper>
          <AssetDropdown
            itemsShowing={this.state.currenciesShowing}
            onClick={() => { this.setState({
              currenciesShowing: !this.state.currenciesShowing
            })}}
          >
            <AssetDropdownLabel>
              {currentConvertTo || 'BNB'}
            </AssetDropdownLabel>
            <CaratDropdown>
              <CaratDownIcon />
            </CaratDropdown>
          </AssetDropdown>
          {
            this.state.currenciesShowing
            ? <AssetItems>
                {currencyList.map((asset: string, i: number) => {
                  if (asset === 'ETH' || asset == 'BNB') {
                    return null
                  }

                  return (
                    <AssetItem
                      key={`choice-${asset}`}
                      isLast={i === (currencyList.length - 1)}
                      onClick={this.setCurrentConvertAsset.bind(this, asset)}
                    >
                      {asset}
                    </AssetItem>
                  )
                })}
              </AssetItems>
            : null
          }
        </BuyPromptWrapper>
        <ActionsWrapper>
          <ConnectButton onClick={() => {
            this.setState({
              showConvertPreview: true
            })
          }}>
            {`Preview Conversion`}
          </ConnectButton>
          <DismissAction onClick={this.setSelectedView.bind(this, 'deposit')}>
            {'Cancel'}
          </DismissAction>
        </ActionsWrapper>
      </>
    )
  }

  renderTradeView = () => {
    const { currencyNames } = this
    const { currentTradeSearch } = this.state
    const { accountBalances, btcPrice, btcVolume, btcBalanceValue, assetBTCValues, assetBTCVolumes } = this.props

    return (
      <>
        <ListItem>
          <ListIcon>
            <ListImg src={searchIcon} />
          </ListIcon>
          <SearchInput
            type={'text'}
            placeholder={'Search trading pairs'}
            onChange={({ target }) => {
              this.setState({ currentTradeSearch: target.value
            })}}
          />
        </ListItem>
        <ListItem
          onClick={() => { this.setState({
            currentTradeAsset: `BTC / USDT`
          })}}
        >
          <ListInfo position={'left'}>
            <TradeLabel>
              <span style={{ color: '#fff' }}>{'BTC'}</span>
              <span>{' / USDT'}</span>
            </TradeLabel>
            <TradeLabel>
              {`Vol ${btcVolume}`}
            </TradeLabel>
          </ListInfo>
          <ListInfo position={'right'}>
            <BTCValueLabel>
              {accountBalances['BTC']}
            </BTCValueLabel>
            <TradeLabel>
              {`$${btcBalanceValue}`}
            </TradeLabel>
          </ListInfo>          
        </ListItem>
        {Object.keys(assetBTCValues).map((pair: string) => {
          const lowerPair = pair.toLowerCase()
          const cleanName = currencyNames[pair].toLowerCase()
          const lowerSearch = currentTradeSearch.toLowerCase()

          if (lowerPair.indexOf(lowerSearch) < 0 &&
              cleanName.indexOf(lowerSearch) < 0) {
            return null
          }

          return (
            <ListItem
              key={`trade-${pair}`}
              style={{ cursor: 'pointer' }}
              onClick={() => { this.setState({
                currentTradeAsset: `${pair} / BTC`
              })}}
            >
              <ListInfo position={'left'}>
                <TradeLabel>
                  <span style={{ color: '#fff' }}>{pair}</span>
                  <span>{' / BTC'}</span>
                </TradeLabel>
                <TradeLabel>
                  {`Vol ${assetBTCVolumes[pair]}`}
                </TradeLabel>
              </ListInfo>
              <ListInfo position={'right'}>
                <OtherValueLabel>
                  {assetBTCValues[pair]}
                </OtherValueLabel>
                <TradeLabel>
                  {`$${getUSDPrice(assetBTCValues[pair], btcPrice)}`}
                </TradeLabel>
              </ListInfo>
            </ListItem>
          )
        })}
      </>
    )
  }

  renderSelectedView = () => {
    const { selectedView } = this.state

    switch (selectedView) {
      case 'deposit':
        return this.renderDepositView()
      case 'summary':
        return this.renderSummaryView()
      default:
        return null
    }
  }

  renderAccountView = () => {
    const { selectedView } = this.state

    return (
      <>
        <NavigationBar>
          <NavigationItem
            isActive={selectedView === 'deposit'}
            onClick={this.setSelectedView.bind(this, 'deposit')}>
            {'Deposit'}
          </NavigationItem>
          <NavigationItem
            isActive={selectedView === 'convert'}
            onClick={this.setSelectedView.bind(this, 'convert')}>
            {'Convert'}
          </NavigationItem>
          <NavigationItem
            isActive={selectedView === 'summary'}
            onClick={this.setSelectedView.bind(this, 'summary')}>
            {'Summary'}
          </NavigationItem>
          <NavigationItem
            isActive={selectedView === 'buy'}
            onClick={this.setSelectedView.bind(this, 'buy')}>
            {'Buy'}
          </NavigationItem>
        </NavigationBar>
        <SelectedView>
          {this.renderSelectedView()}
        </SelectedView>
      </>
    )
  }

  renderBuyView = () => {
    const {
      onBuyCrypto,
      userTLD,
      initialAsset,
      initialFiat,
      initialAmount
    } = this.props
    const {
      fiatShowing,
      currenciesShowing
    } = this.state
    const isUS = userTLD === 'us'
    const currencies = isUS ? this.usCurrencies : this.comCurrencies

    return (
      <>
        <Copy>
          {getLocale('binanceWidgetBuyCrypto')}
        </Copy>
        <TLDSwitchWrapper>
          <TLDSwitch
            onClick={this.toggleTLD}
            isActive={userTLD === 'com'}
          >
            {'.com'}
          </TLDSwitch>
          <TLDSwitch
            onClick={this.toggleTLD}
            isActive={userTLD === 'us'}
          >
            {'.us'}
          </TLDSwitch>
        </TLDSwitchWrapper>
        <BuyPromptWrapper>
          <FiatInputWrapper>
            <FiatInputField
              type={'text'}
              placeholder={getLocale('binanceWidgetBuyDefault')}
              value={initialAmount}
              onChange={this.setInitialAmount}
            />
            <FiatDropdown
              disabled={isUS}
              itemsShowing={fiatShowing}
              onClick={this.handleFiatChange}
            >
              <span>
                {initialFiat}
              </span>
              <CaratDropdown hide={isUS}>
                <CaratDownIcon />
              </CaratDropdown>
            </FiatDropdown>
            {
              fiatShowing
              ? <AssetItems isFiat={true}>
                  {this.fiatList.map((fiat: string, i: number) => {
                    if (fiat === initialFiat) {
                      return null
                    }

                    return (
                      <AssetItem
                        key={`choice-${fiat}`}
                        isLast={i === (currencies.length - 1)}
                        onClick={this.setInitialFiat.bind(this, fiat)}
                      >
                        {fiat}
                      </AssetItem>
                    )
                  })}
                </AssetItems>
              : null
            }
          </FiatInputWrapper>
          <AssetDropdown
            itemsShowing={currenciesShowing}
            onClick={this.toggleCurrenciesShowing}
          >
            <AssetDropdownLabel>
              {initialAsset}
            </AssetDropdownLabel>
            <CaratDropdown>
              <CaratDownIcon />
            </CaratDropdown>
          </AssetDropdown>
          {
            currenciesShowing
            ? <AssetItems>
                {currencies.map((asset: string, i: number) => {
                  if (asset === initialAsset) {
                    return null
                  }

                  return (
                    <AssetItem
                      key={`choice-${asset}`}
                      isLast={i === (currencies.length - 1)}
                      onClick={this.setInitialAsset.bind(this, asset)}
                    >
                      {asset}
                    </AssetItem>
                  )
                })}
              </AssetItems>
            : null
          }
        </BuyPromptWrapper>
        <ActionsWrapper>
          <ConnectButton onClick={onBuyCrypto.bind(this, initialAsset, initialAmount, initialFiat)}>
            {`${getLocale('binanceWidgetBuy')} ${initialAsset}`}
          </ConnectButton>
        </ActionsWrapper>
      </>
    )
  }

  render () {
    const { disconnectInProgress, showConvertPreview, insufficientFunds, convertSuccess } = this.state
    const { showContent } = this.props 

    if (!showContent) {
      return this.renderTitleTab()
    }

    if (insufficientFunds) {
      return (
        <WidgetWrapper>
          {this.renderUnableToConvertView()}
        </WidgetWrapper>
      )
    }

    if (convertSuccess) {
      return (
        <WidgetWrapper>
          {this.renderConvertSuccess()}
        </WidgetWrapper>
      )
    }

    if (showConvertPreview) {
      return (
        <WidgetWrapper>
          {this.renderConvertConfirm()}
        </WidgetWrapper>
      )
    }

    if (disconnectInProgress) {
      return (
        <WidgetWrapper>
          {this.renderDisconnectView()}
        </WidgetWrapper>
      )
    }

    return (
      <WidgetWrapper>
        {this.renderTitle()}
        {this.renderRoutes()}
      </WidgetWrapper>
    )    
  }
}

export const BinanceWidget = createWidget(Binance)
