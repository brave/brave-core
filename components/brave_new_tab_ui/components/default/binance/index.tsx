/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { AnyStyledComponent } from 'styled-components'

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
  NavigationBar,
  NavigationItem,
  SelectedView,
  TickerLabel,
  ActionButton,
  AssetIcon,
  QRImage,
  CopyButton,
  DropdownIcon,
  ConnectAction
} from './style'
import {
  ShowIcon,
  HideIcon,
  PartyIcon,
  QRIcon,
  SearchIcon
} from '../exchangeWidget/shared-assets'
import { StyledTitleTab } from '../widgetTitleTab'
import currencyData from './data'
import BinanceLogo from './assets/binance-logo'
import { CaratLeftIcon, CaratDownIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../common/locale'
import cryptoColors from '../exchangeWidget/colors'

interface State {
  fiatShowing: boolean
  currenciesShowing: boolean
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
  convertFailed: boolean
  convertError: string
  isBuyView: boolean
  currentQRAsset: string
  convertFromShowing: boolean
  convertToShowing: boolean
  currentConvertId: string
  currentConvertPrice: string
  currentConvertFee: string
  currentConvertTransAmount: string
  currentConvertExpiryTime: number
  currentConvertMinimum: string
  underMinimumConvertAmount: boolean
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
  btcPrice: string
  binanceClientUrl: string
  assetDepositInfo: Record<string, any>
  assetDepoitQRCodeSrcs: Record<string, string>
  convertAssets: Record<string, chrome.binance.ConvertSubAsset[]>
  accountBTCValue: string
  accountBTCUSDValue: string
  disconnectInProgress: boolean
  authInvalid: boolean
  selectedView: string
  stackPosition: number
  onShowContent: () => void
  onBuyCrypto: (coin: string, amount: string, fiat: string) => void
  onBinanceUserTLD: (userTLD: NewTab.BinanceTLD) => void
  onBinanceUserLocale: (userLocale: string) => void
  onSetInitialFiat: (initialFiat: string) => void
  onSetInitialAmount: (initialAmount: string) => void
  onSetInitialAsset: (initialAsset: string) => void
  onSetUserTLDAutoSet: () => void
  onSetHideBalance: (hide: boolean) => void
  onBinanceClientUrl: (clientUrl: string) => void
  onDisconnectBinance: () => void
  onCancelDisconnect: () => void
  onConnectBinance: () => void
  onValidAuthCode: () => void
  onUpdateActions: () => void
  onDismissAuthInvalid: () => void
  onSetSelectedView: (view: string) => void
  getCurrencyList: () => string[]
}

class Binance extends React.PureComponent<Props, State> {
  private fiatList: string[]
  private currencyNames: Record<string, string>
  private convertTimer: any
  private refreshInterval: any

  constructor (props: Props) {
    super(props)
    this.state = {
      fiatShowing: false,
      currenciesShowing: false,
      currentDepositSearch: '',
      currentDepositAsset: '',
      currentTradeSearch: '',
      currentTradeAsset: '',
      currentTradeAmount: '',
      currentConvertAmount: '',
      currentConvertFrom: 'BTC',
      currentConvertTo: 'BNB',
      currentConvertId: '',
      currentConvertPrice: '',
      currentConvertFee: '',
      currentConvertTransAmount: '',
      insufficientFunds: false,
      showConvertPreview: false,
      convertSuccess: false,
      convertFailed: false,
      convertError: '',
      isBuyView: true,
      currentQRAsset: '',
      convertFromShowing: false,
      convertToShowing: false,
      currentConvertExpiryTime: 30,
      currentConvertMinimum: '',
      underMinimumConvertAmount: false
    }
    this.fiatList = currencyData.fiatList
    this.currencyNames = {
      'BAT': 'Basic Attent...',
      'BTC': 'Bitcoin',
      'ETH': 'Ethereum',
      'XRP': 'Ripple',
      'BNB': 'Binance Coin',
      'BCH': 'Bitcoin Cash',
      'BUSD': 'US Dollar'
    }
  }

  componentDidMount () {
    const { userTLDAutoSet } = this.props

    if (this.props.userAuthed) {
      this.props.onUpdateActions()
      this.checkSetRefreshInterval()
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

    chrome.binance.getLocaleForURL((userLocale: string) => {
      this.props.onBinanceUserLocale(userLocale)
    })

    this.getClientURL()
  }

  getClientURL = () => {
    chrome.binance.getClientUrl((clientUrl: string) => {
      this.props.onBinanceClientUrl(clientUrl)
    })
  }

  componentWillUnmount () {
    clearInterval(this.refreshInterval)
  }

  componentDidUpdate (prevProps: Props) {
    if (!prevProps.userAuthed && this.props.userAuthed) {
      this.props.onUpdateActions()
      this.checkSetRefreshInterval()
    }

    if (prevProps.userAuthed && !this.props.userAuthed) {
      this.getClientURL()
      this.clearIntervals()
    }
  }

  checkSetRefreshInterval = () => {
    if (!this.refreshInterval) {
      this.refreshInterval = setInterval(() => {
        this.props.onUpdateActions()
      }, 30000)
    }
  }

  checkForOauthCode = () => {
    const params = window.location.search
    const urlParams = new URLSearchParams(params)
    const binanceAuth = urlParams.get('binanceAuth')

    if (binanceAuth) {
      chrome.binance.getAccessToken((success: boolean) => {
        if (success) {
          this.props.onValidAuthCode()
          this.props.onUpdateActions()
        } else {
          console.warn('Backend indicated there was a problem retrieving and storing binance access token.')
        }
      })
    }
  }

  clearIntervals = () => {
    clearInterval(this.convertTimer)
    clearInterval(this.refreshInterval)
  }

  connectBinance = () => {
    const { binanceClientUrl } = this.props
    window.open(binanceClientUrl, '_self', 'noopener')
    this.props.onConnectBinance()
  }

  cancelDisconnect = () => {
    this.props.onCancelDisconnect()
  }

  cancelConvert = () => {
    clearInterval(this.convertTimer)
    this.setState({
      insufficientFunds: false,
      showConvertPreview: false,
      convertSuccess: false,
      convertFailed: false,
      currentConvertAmount: '',
      currentConvertFrom: 'BTC',
      currentConvertTo: 'BNB',
      currentConvertId: '',
      currentConvertPrice: '',
      currentConvertFee: '',
      currentConvertTransAmount: '',
      currentConvertExpiryTime: 30,
      currentConvertMinimum: '',
      underMinimumConvertAmount: false
    })
  }

  retryConvert = () => {
    clearInterval(this.convertTimer)
    this.setState({
      insufficientFunds: false,
      showConvertPreview: false,
      convertSuccess: false,
      convertFailed: false,
      convertError: '',
      currentConvertId: '',
      currentConvertPrice: '',
      currentConvertFee: '',
      currentConvertTransAmount: '',
      currentConvertExpiryTime: 30,
      currentConvertMinimum: '',
      underMinimumConvertAmount: false
    })
  }

  finishDisconnect = () => {
    this.clearIntervals()
    chrome.binance.revokeToken(() => {
      this.props.onDisconnectBinance()
      this.cancelDisconnect()
    })
  }

  renderRoutes = () => {
    const { userAuthed } = this.props

    if (userAuthed) {
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
    this.props.onSetSelectedView(view)
  }

  setCurrentDepositAsset (asset: string) {
    this.setState({
      currentDepositAsset: asset
    })

    if (!asset) {
      this.setState({
        currentDepositSearch: ''
      })
    }
  }

  setCurrentConvertAsset (asset: string) {
    this.setState({
      currentConvertTo: asset,
      convertToShowing: false
    })
  }

  setIsBuyView (isBuyView: boolean) {
    this.setState({ isBuyView })
  }

  processConvert = () => {
    const { currentConvertId } = this.state
    chrome.binance.confirmConvert(currentConvertId, (success: boolean, message: string) => {
      if (success) {
        this.setState({ convertSuccess: true })
      } else {
        this.setState({
          convertFailed: true,
          convertError: message
        })
      }
    })
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

  finishConvert = () => {
    this.cancelConvert()
    this.props.onSetSelectedView('summary')
  }

  setCurrentDepositSearch = ({ target }: any) => {
    this.setState({
      currentDepositSearch: target.value
    })
  }

  setCurrentConvertAmount = ({ target }: any) => {
    this.setState({ currentConvertAmount: target.value })
  }

  setCurrentTradeSearch = ({ target }: any) => {
    this.setState({ currentTradeSearch: target.value })
  }

  setCurrentTradeAsset = (asset: string) => {
    this.setState({ currentTradeAsset: asset })
  }

  checkMeetsConvertMinimum = () => {
    const { convertAssets } = this.props
    const {
      currentConvertFrom,
      currentConvertTo,
      currentConvertAmount
    } = this.state

    const subSelector = convertAssets[currentConvertFrom].find((item: chrome.binance.ConvertSubAsset) => {
      return item.assetName === currentConvertTo
    })

    const minAmount = (subSelector && subSelector.minAmount) || 0
    const meetsMinimum = parseFloat(currentConvertAmount) >= minAmount

    return {
      success: meetsMinimum,
      minimum: this.formatCryptoBalance(minAmount)
    }
  }

  shouldShowConvertPreview = () => {
    const {
      currentConvertFrom,
      currentConvertTo,
      currentConvertAmount
    } = this.state
    const { accountBalances } = this.props

    // As there are trading fees we shouldn't proceed even in equal amounts
    if (!accountBalances[currentConvertFrom] ||
        parseFloat(currentConvertAmount) >= parseFloat(accountBalances[currentConvertFrom])) {
      this.setState({ insufficientFunds: true })
      return
    }

    const { success, minimum } = this.checkMeetsConvertMinimum()
    if (!success) {
      this.setState({
        currentConvertMinimum: minimum,
        underMinimumConvertAmount: true
      })
      return
    }

    chrome.binance.getConvertQuote(currentConvertFrom, currentConvertTo, currentConvertAmount, (quote: any) => {
      if (!quote.id || !quote.price || !quote.fee || !quote.amount) {
        this.setState({ convertFailed: true })
        return
      }

      this.setState({
        currentConvertId: quote.id,
        currentConvertPrice: quote.price,
        currentConvertFee: quote.fee,
        currentConvertTransAmount: quote.amount,
        showConvertPreview: true
      })

      this.convertTimer = setInterval(() => {
        const { currentConvertExpiryTime } = this.state

        if (currentConvertExpiryTime - 1 === 0) {
          clearInterval(this.convertTimer)
          this.cancelConvert()
          return
        }

        this.setState({
          currentConvertExpiryTime: (currentConvertExpiryTime - 1)
        })
      }, 1000)
    })
  }

  setQR = (asset: string) => {
    this.setState({
      currentQRAsset: asset
    })
  }

  cancelQR = () => {
    this.setState({
      currentQRAsset: ''
    })
  }

  handleConvertFromChange = () => {
    if (this.state.convertFromShowing) {
      return
    }

    this.setState({
      convertFromShowing: !this.state.convertFromShowing
    })
  }

  handleConvertToChange = () => {
    if (this.state.convertToShowing) {
      return
    }

    this.setState({
      convertToShowing: !this.state.convertToShowing
    })
  }

  setCurrentConvertFrom = (asset: string) => {
    this.setState({
      convertFromShowing: false,
      currentConvertFrom: asset
    })
  }

  unpersistDropdowns = (event: any) => {
    const {
      fiatShowing,
      convertToShowing,
      convertFromShowing,
      currenciesShowing
    } = this.state

    if (!fiatShowing && !convertToShowing &&
        !convertFromShowing && !currenciesShowing) {
      return
    }

    if (!event.target.classList.contains('asset-dropdown')) {
      this.setState({
        fiatShowing: false,
        convertToShowing: false,
        convertFromShowing: false,
        currenciesShowing: false
      })
    }
  }

  copyToClipboard = async (address: string) => {
    try {
      await navigator.clipboard.writeText(address)
    } catch (e) {
      console.log(`Could not copy address ${e.toString()}`)
    }
  }

  renderIconAsset = (key: string, isDetail: boolean = false) => {
    const iconColor = cryptoColors[key] || '#fff'

    return (
      <AssetIcon
        isDetail={isDetail}
        style={{ color: iconColor }}
        className={`crypto-icon icon-${key}`}
      />
    )
  }

  renderTitle () {
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
      </Header>
    )
  }

  renderTitleTab () {
    const { onShowContent, stackPosition } = this.props

    return (
      <StyledTitleTab onClick={onShowContent} stackPosition={stackPosition}>
        {this.renderTitle()}
      </StyledTitleTab>
    )
  }

  renderAuthInvalid = () => {
    const { onDismissAuthInvalid } = this.props

    return (
      <InvalidWrapper>
        <InvalidTitle>
          {getLocale('binanceWidgetAuthInvalid')}
        </InvalidTitle>
        <InvalidCopy>
          {getLocale('binanceWidgetAuthInvalidCopy')}
        </InvalidCopy>
        <GenButton onClick={onDismissAuthInvalid}>
          {getLocale('binanceWidgetDone')}
        </GenButton>
      </InvalidWrapper>
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
    const {
      currentConvertAmount,
      currentConvertFrom,
      currentConvertTo,
      currentConvertTransAmount
    } = this.state

    return (
      <InvalidWrapper>
        <StyledEmoji>
          <img src={PartyIcon} />
        </StyledEmoji>
        <InvalidTitle>
          {`${getLocale('binanceWidgetConverted')} ${currentConvertAmount} ${currentConvertFrom} to ${currentConvertTransAmount} ${currentConvertTo}!`}
        </InvalidTitle>
        <ConnectButton isSmall={true} onClick={this.finishConvert}>
          {getLocale('binanceWidgetContinue')}
        </ConnectButton>
      </InvalidWrapper>
    )
  }

  renderInsufficientFundsView = () => {
    return (
      <InvalidWrapper>
        <InvalidTitle>
          {getLocale('binanceWidgetUnableToConvert')}
        </InvalidTitle>
        <InvalidCopy>
          {getLocale('binanceWidgetInsufficientFunds')}
        </InvalidCopy>
        <GenButton onClick={this.retryConvert}>
          {getLocale('binanceWidgetRetry')}
        </GenButton>
      </InvalidWrapper>
    )
  }

  renderUnderMinimumConvertView = () => {
    const { currentConvertFrom, currentConvertMinimum } = this.state

    return (
      <InvalidWrapper>
        <InvalidTitle>
          {getLocale('binanceWidgetUnableToConvert')}
        </InvalidTitle>
        <InvalidCopy>
          {getLocale('binanceWidgetUnderMinimum')} {currentConvertMinimum} {currentConvertFrom}
        </InvalidCopy>
        <GenButton onClick={this.retryConvert}>
          {getLocale('binanceWidgetRetry')}
        </GenButton>
      </InvalidWrapper>
    )
  }

  renderUnableToConvertView = () => {
    const { convertError } = this.state
    const errorMessage = convertError || getLocale('binanceWidgetConversionFailed')

    return (
      <InvalidWrapper>
        <InvalidTitle>
          {getLocale('binanceWidgetUnableToConvert')}
        </InvalidTitle>
        <InvalidCopy>
          {errorMessage}
        </InvalidCopy>
        <GenButton onClick={this.retryConvert}>
          {getLocale('binanceWidgetRetry')}
        </GenButton>
      </InvalidWrapper>
    )
  }

  renderQRView = () => {
    const { assetDepoitQRCodeSrcs } = this.props
    const imageSrc = assetDepoitQRCodeSrcs[this.state.currentQRAsset]

    return (
      <InvalidWrapper>
        <QRImage src={imageSrc} />
        <GenButton onClick={this.cancelQR}>
          {getLocale('binanceWidgetDone')}
        </GenButton>
      </InvalidWrapper>
    )
  }

  formatCryptoBalance = (balance: number | string) => {
    if (!balance) {
      return '0.000000'
    }

    if (typeof balance === 'string') {
      balance = parseFloat(balance)
    }

    return balance.toFixed(6)
  }

  renderCurrentDepositAsset = () => {
    const { currentDepositAsset } = this.state
    const assetDepositInfo = this.props.assetDepositInfo || {}
    const addressInfo = assetDepositInfo[currentDepositAsset]
    const address = addressInfo && addressInfo.address
    const tag = addressInfo && addressInfo.tag
    const cleanName = this.currencyNames[currentDepositAsset]
    const cleanNameDisplay = cleanName ? `(${cleanName})` : ''
    const depositData = tag || address

    return (
      <>
        <ListItem>
          <DetailIcons>
            <BackArrow>
              <CaratLeftIcon
                onClick={this.setCurrentDepositAsset.bind(this, '')}
              />
            </BackArrow>
            {this.renderIconAsset(currentDepositAsset.toLowerCase(), true)}
          </DetailIcons>
          <AssetTicker>
            {currentDepositAsset}
          </AssetTicker>
          <AssetLabel>
            {cleanNameDisplay}
          </AssetLabel>
          {
            depositData
            ? <AssetQR onClick={this.setQR.bind(this, currentDepositAsset)}>
                <img style={{ width: '25px', marginRight: '5px' }} src={QRIcon} />
              </AssetQR>
            : null
          }
        </ListItem>
        <DetailArea>
          {
            !depositData
            ? <MemoArea>
                <MemoInfo>
                  <DetailLabel>
                    {`${currentDepositAsset}`}
                  </DetailLabel>
                  <DetailInfo>
                    {getLocale('binanceWidgetAddressUnavailable')}
                  </DetailInfo>
                </MemoInfo>
              </MemoArea>
            : null
          }
          {
            address
            ? <MemoArea>
                <MemoInfo>
                  <DetailLabel>
                    {`${currentDepositAsset} ${getLocale('binanceWidgetDepositAddress')}`}
                  </DetailLabel>
                  <DetailInfo>
                    {address}
                  </DetailInfo>
                </MemoInfo>
                <CopyButton onClick={this.copyToClipboard.bind(this, address)}>
                  {getLocale('binanceWidgetCopy')}
                </CopyButton>
              </MemoArea>
            : null
          }
          {
            tag
            ? <MemoArea>
                <MemoInfo>
                  <DetailLabel>
                    {`${currentDepositAsset} ${getLocale('binanceWidgetDepositMemo')}`}
                  </DetailLabel>
                  <DetailInfo>
                    {tag}
                  </DetailInfo>
                </MemoInfo>
                <CopyButton onClick={this.copyToClipboard.bind(this, tag)}>
                  {getLocale('binanceWidgetCopy')}
                </CopyButton>
              </MemoArea>
            : null
          }
        </DetailArea>
      </>
    )
  }

  renderDepositView = () => {
    const { currencyNames } = this
    const { currentDepositSearch, currentDepositAsset } = this.state
    const currencyList = this.props.getCurrencyList()

    if (currentDepositAsset) {
      return this.renderCurrentDepositAsset()
    }

    return (
      <>
        <ListItem>
          <ListIcon>
            <ListImg src={SearchIcon} />
          </ListIcon>
          <SearchInput
            type={'text'}
            placeholder={getLocale('binanceWidgetSearch')}
            onChange={this.setCurrentDepositSearch}
          />
        </ListItem>
        {currencyList.map((asset: string) => {
          const cleanName = currencyNames[asset] || asset
          const lowerAsset = asset.toLowerCase()
          const lowerName = cleanName.toLowerCase()
          const lowerSearch = currentDepositSearch.toLowerCase()
          const currencyName = currencyNames[asset] || false
          const nameString = currencyName ? `(${currencyName})` : ''

          if (lowerAsset.indexOf(lowerSearch) < 0 &&
              lowerName.indexOf(lowerSearch) < 0 && currentDepositSearch) {
            return null
          }

          return (
            <ListItem
              key={`list-${asset}`}
              onClick={this.setCurrentDepositAsset.bind(this, asset)}
            >
              <ListIcon>
                {this.renderIconAsset(asset.toLowerCase())}
              </ListIcon>
              <ListLabel clickable={true}>
                {`${asset} ${nameString}`}
              </ListLabel>
            </ListItem>
          )
        })}
      </>
    )
  }

  renderSummaryView = () => {
    const {
      accountBalances,
      hideBalance,
      accountBTCValue,
      accountBTCUSDValue,
      assetUSDValues,
      getCurrencyList
    } = this.props
    const currencyList = getCurrencyList()
    const totalBTCUSDValue = accountBTCUSDValue || '0.00'
    const totalBTCValue = accountBTCValue ? this.formatCryptoBalance(accountBTCValue) : '0.000000'

    return (
      <>
        <BTCSummary>
          <ListInfo position={'left'}>
            <TradeLabel>
              <Balance isBTC={true} hideBalance={hideBalance}>
                {totalBTCValue} <TickerLabel>{getLocale('binanceWidgetBTCTickerText')}</TickerLabel>
              </Balance>
              <Converted isBTC={true} hideBalance={hideBalance}>
                {`= $${totalBTCUSDValue}`}
              </Converted>
            </TradeLabel>
          </ListInfo>
          <ListInfo position={'right'} isBTC={true}>
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
          // Initial migration display
          const assetAccountBalance = accountBalances ? accountBalances[asset] : '0.00'
          const assetUSDValue = assetUSDValues ? (assetUSDValues[asset] || '0.00') : '0.00'
          const assetBalance = this.formatCryptoBalance(assetAccountBalance)

          return (
            <ListItem key={`list-${asset}`}>
              <ListInfo isAsset={true} position={'left'}>
                <ListIcon>
                  {this.renderIconAsset(asset.toLowerCase())}
                </ListIcon>
                <ListLabel>
                  {asset}
                </ListLabel>
              </ListInfo>
              <ListInfo position={'right'}>
                <Balance isBTC={false} hideBalance={hideBalance}>
                  {assetBalance}
                </Balance>
                <Converted isBTC={false} hideBalance={hideBalance}>
                  {`= $${assetUSDValue}`}
                </Converted>
              </ListInfo>
            </ListItem>
          )
        })}
      </>
    )
  }

  renderConvertConfirm = () => {
    const {
      currentConvertAmount,
      currentConvertFrom,
      currentConvertTo,
      currentConvertFee,
      currentConvertTransAmount,
      currentConvertExpiryTime
    } = this.state
    const displayConvertAmount = this.formatCryptoBalance(currentConvertAmount)
    const displayConvertFee = this.formatCryptoBalance(currentConvertFee)
    const displayReceiveAmount = this.formatCryptoBalance(currentConvertTransAmount)

    return (
      <InvalidWrapper>
        <InvalidTitle>
          {getLocale('binanceWidgetConfirmConversion')}
        </InvalidTitle>
        <ConvertInfoWrapper>
          <ConvertInfoItem>
            <ConvertLabel>{getLocale('binanceWidgetConvert')}</ConvertLabel>
            <ConvertValue>{`${displayConvertAmount} ${currentConvertFrom}`}</ConvertValue>
          </ConvertInfoItem>
          <ConvertInfoItem>
            <ConvertLabel>{getLocale('binanceWidgetFee')}</ConvertLabel>
            <ConvertValue>{`${displayConvertFee} ${currentConvertFrom}`}</ConvertValue>
          </ConvertInfoItem>
          <ConvertInfoItem isLast={true}>
            <ConvertLabel>{getLocale('binanceWidgetWillReceive')}</ConvertLabel>
            <ConvertValue>{`${displayReceiveAmount} ${currentConvertTo}`}</ConvertValue>
          </ConvertInfoItem>
        </ConvertInfoWrapper>
        <ActionsWrapper>
          <ConnectButton isSmall={true} onClick={this.processConvert}>
            {`${getLocale('binanceWidgetConfirm')} (${currentConvertExpiryTime}s)`}
          </ConnectButton>
          <DismissAction onClick={this.cancelConvert}>
            {getLocale('binanceWidgetCancel')}
          </DismissAction>
        </ActionsWrapper>
      </InvalidWrapper>
    )
  }

  renderConvertView = () => {
    const { accountBalances, convertAssets } = this.props
    const {
      currentConvertAmount,
      currentConvertTo,
      currentConvertFrom,
      convertFromShowing,
      convertToShowing
    } = this.state
    const convertFromAmount = this.formatCryptoBalance(accountBalances[currentConvertFrom])
    const compatibleCurrencies = convertAssets[currentConvertFrom]
    const convertAssetKeys = Object.keys(convertAssets)

    return (
      <>
        <Copy>
          {`${getLocale('binanceWidgetAvailable')} ${convertFromAmount} ${currentConvertFrom}`}
        </Copy>
        <BuyPromptWrapper>
          <FiatInputWrapper>
            <FiatInputField
              type={'text'}
              placeholder={getLocale('binanceWidgetConvertIntent')}
              value={currentConvertAmount}
              onChange={this.setCurrentConvertAmount}
            />
            <FiatDropdown
              className={'asset-dropdown'}
              itemsShowing={convertFromShowing}
              onClick={this.handleConvertFromChange}
            >
              <span>
                {currentConvertFrom}
              </span>
              <CaratDropdown>
                <CaratDownIcon />
              </CaratDropdown>
            </FiatDropdown>
            {
              convertFromShowing
              ? <AssetItems isFiat={true}>
                  {convertAssetKeys.map((asset: string, i: number) => {
                    if (asset === currentConvertFrom) {
                      return null
                    }

                    return (
                      <AssetItem
                        key={`choice-${asset}`}
                        isLast={i === convertAssetKeys.length - 1}
                        onClick={this.setCurrentConvertFrom.bind(this, asset)}
                      >
                        <DropdownIcon>
                          {this.renderIconAsset(asset.toLowerCase())}
                        </DropdownIcon>
                        {asset}
                      </AssetItem>
                    )
                  })}
                </AssetItems>
              : null
            }
          </FiatInputWrapper>
          <AssetDropdown
            className={'asset-dropdown'}
            itemsShowing={convertToShowing}
            onClick={this.handleConvertToChange}
          >
            <AssetDropdownLabel>
              <DropdownIcon>
                {this.renderIconAsset(currentConvertTo.toLowerCase())}
              </DropdownIcon>
              {currentConvertTo}
            </AssetDropdownLabel>
            <CaratDropdown>
              <CaratDownIcon />
            </CaratDropdown>
          </AssetDropdown>
          {
            convertToShowing
            ? <AssetItems>
                {compatibleCurrencies.map((item, i: number) => {
                  if (!item) {
                    console.warn('binance bad asset', item)
                    return null
                  }

                  if (item.assetName === currentConvertTo) {
                    return null
                  }

                  return (
                    <AssetItem
                      key={`choice-${item.assetName}`}
                      isLast={i === (compatibleCurrencies.length - 1)}
                      onClick={this.setCurrentConvertAsset.bind(this, item.assetName)}
                    >
                      <DropdownIcon>
                        {this.renderIconAsset(item.assetName.toLowerCase())}
                      </DropdownIcon>
                      {item.assetName}
                    </AssetItem>
                  )
                })}
              </AssetItems>
            : null
          }
        </BuyPromptWrapper>
        <ActionsWrapper>
          <ActionButton onClick={this.shouldShowConvertPreview}>
            {getLocale('binanceWidgetPreviewConvert')}
          </ActionButton>
        </ActionsWrapper>
      </>
    )
  }

  renderSelectedView = () => {
    const { selectedView } = this.props

    switch (selectedView) {
      case 'deposit':
        return this.renderDepositView()
      case 'summary':
        return this.renderSummaryView()
      case 'convert':
        return this.renderConvertView()
      case 'buy':
        return this.renderBuyView()
      default:
        return this.renderSummaryView()
    }
  }

  renderAccountView = () => {
    const { selectedView } = this.props
    const { currentDepositAsset } = this.state
    const isSummaryView = !selectedView || selectedView === 'summary'
    const hideOverflow = currentDepositAsset && selectedView === 'deposit'

    return (
      <>
        <NavigationBar>
          <NavigationItem
            tabIndex={0}
            isLeading={true}
            isActive={isSummaryView}
            onClick={this.setSelectedView.bind(this, 'summary')}
          >
            {getLocale('binanceWidgetSummary')}
          </NavigationItem>
          <NavigationItem
            tabIndex={0}
            isActive={selectedView === 'deposit'}
            onClick={this.setSelectedView.bind(this, 'deposit')}
          >
            {getLocale('binanceWidgetDepositLabel')}
          </NavigationItem>
          <NavigationItem
            tabIndex={0}
            isActive={selectedView === 'convert'}
            onClick={this.setSelectedView.bind(this, 'convert')}
          >
            {getLocale('binanceWidgetConvert')}
          </NavigationItem>
          <NavigationItem
            tabIndex={0}
            isBuy={true}
            isActive={selectedView === 'buy'}
            onClick={this.setSelectedView.bind(this, 'buy')}
          >
            {getLocale('binanceWidgetBuy')}
          </NavigationItem>
        </NavigationBar>
        {
          selectedView === 'convert' || selectedView === 'buy'
          ? this.renderSelectedView()
          : <SelectedView hideOverflow={!!hideOverflow}>
              {this.renderSelectedView()}
            </SelectedView>
        }
      </>
    )
  }

  renderBuyView = () => {
    const {
      onBuyCrypto,
      userTLD,
      initialAsset,
      initialFiat,
      initialAmount,
      userAuthed,
      getCurrencyList
    } = this.props
    const {
      fiatShowing,
      currenciesShowing
    } = this.state
    const isUS = userTLD === 'us'
    const ButtonComponent: AnyStyledComponent = userAuthed ? ActionButton : ConnectButton
    const currencies = getCurrencyList()

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
              isFiat={true}
              placeholder={getLocale('binanceWidgetBuyDefault')}
              value={initialAmount}
              onChange={this.setInitialAmount}
            />
            <FiatDropdown
              isFiat={true}
              disabled={isUS}
              itemsShowing={fiatShowing}
              className={'asset-dropdown'}
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
                        isLast={i === (this.fiatList.length - 1)}
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
            className={'asset-dropdown'}
            itemsShowing={currenciesShowing}
            onClick={this.toggleCurrenciesShowing}
          >
            <AssetDropdownLabel>
              <DropdownIcon>
                {this.renderIconAsset(initialAsset.toLowerCase())}
              </DropdownIcon>
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
                      <DropdownIcon>
                        {this.renderIconAsset(asset.toLowerCase())}
                      </DropdownIcon>
                      {asset}
                    </AssetItem>
                  )
                })}
              </AssetItems>
            : null
          }
        </BuyPromptWrapper>
        <ActionsWrapper isFirstView={!userAuthed}>
          <ButtonComponent onClick={onBuyCrypto.bind(this, initialAsset, initialAmount, initialFiat)}>
            {`${getLocale('binanceWidgetBuy')} ${initialAsset}`}
          </ButtonComponent>
          {
            !userAuthed && !isUS
            ? <ConnectAction onClick={this.connectBinance}>
                {getLocale('binanceWidgetConnect')}
              </ConnectAction>
            : null
          }
        </ActionsWrapper>
      </>
    )
  }

  renderIndexView () {
    const {
      currentQRAsset,
      insufficientFunds,
      convertFailed,
      convertSuccess,
      showConvertPreview,
      underMinimumConvertAmount
    } = this.state
    const {
      authInvalid,
      disconnectInProgress
    } = this.props

    if (authInvalid) {
      return this.renderAuthInvalid()
    } else if (currentQRAsset) {
      return this.renderQRView()
    } else if (insufficientFunds) {
      return this.renderInsufficientFundsView()
    } else if (underMinimumConvertAmount) {
      return this.renderUnderMinimumConvertView()
    } else if (convertFailed) {
      return this.renderUnableToConvertView()
    } else if (convertSuccess) {
      return this.renderConvertSuccess()
    } else if (showConvertPreview) {
      return this.renderConvertConfirm()
    } else if (disconnectInProgress) {
      return this.renderDisconnectView()
    } else {
      return false
    }
  }

  render () {
    const { showContent, userAuthed } = this.props

    if (!showContent) {
      return this.renderTitleTab()
    }

    return (
      <WidgetWrapper tabIndex={0} userAuthed={userAuthed} onClick={this.unpersistDropdowns}>
        {
          this.renderIndexView()
          ? this.renderIndexView()
          : <>
              {this.renderTitle()}
              {this.renderRoutes()}
            </>
        }
      </WidgetWrapper>
    )
  }
}

export const BinanceWidget = createWidget(Binance)
