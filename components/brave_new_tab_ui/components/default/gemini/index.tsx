/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
const clipboardCopy = require('clipboard-copy')

import createWidget from '../widget/index'
import { StyledTitleTab } from '../widgetTitleTab'

import {
  WidgetWrapper,
  IntroTitle,
  Copy,
  ActionsWrapper,
  ConnectButton,
  DismissAction,
  Header,
  StyledTitle,
  GeminiIcon,
  StyledTitleText,
  NavigationBar,
  NavigationItem,
  SelectedView,
  ListItem,
  ListIcon,
  ListImg,
  ListLabel,
  AssetIconWrapper,
  AssetIcon,
  SearchInput,
  DetailIcons,
  AssetTicker,
  AssetLabel,
  AssetQR,
  DetailArea,
  MemoArea,
  MemoInfo,
  SmallNoticeWrapper,
  QRImage,
  DetailLabel,
  DetailInfo,
  CopyButton,
  BackArrow,
  GenButton,
  DetailIconWrapper,
  AccountSummary,
  ListInfo,
  TradeLabel,
  Balance,
  BlurIcon,
  TradeWrapper,
  InputWrapper,
  AssetDropdown,
  AssetItems,
  CaratDropdown,
  DropdownIcon,
  AssetItem,
  ActionButton,
  AmountInputField,
  Dropdown,
  AssetDropdownLabel,
  TradeSwitchWrapper,
  TradeSwitch,
  DisconnectButton,
  DisconnectWrapper,
  DisconnectTitle,
  DisconnectCopy,
  InvalidWrapper,
  InvalidTitle,
  InvalidCopy
} from './style'
import {
  SearchIcon,
  QRIcon,
  ShowIcon,
  HideIcon
} from '../exchangeWidget/shared-assets'
import GeminiLogo from './assets/gemini-logo'
import { CaratLeftIcon, CaratDownIcon } from 'brave-ui/components/icons'

// Utils
import geminiData from './data'
import cryptoColors from '../exchangeWidget/colors'
import { getLocale } from '../../../../common/locale'

interface State {
  currentDepositSearch: string
  currentDepositAsset: string
  currentQRAsset: string
  currentTradeMode: string
  currentTradeQuantity: string
  currentTradeAsset: string
  tradeDropdownShowing: boolean
  insufficientFunds: boolean
  showTradePreview: boolean
  tradeSuccess: boolean
  tradeFailed: boolean
  currentTradeId: string
  currentTradeFee: string
  currentTradePrice: string
  currentTradeQuantityLive: string
  currentTradeExpiryTime: number
}

interface Props {
  hideBalance: boolean
  showContent: boolean
  userAuthed: boolean
  authInProgress: boolean
  geminiClientUrl: string
  selectedView: string
  assetAddresses: Record<string, string>
  assetAddressQRCodes: Record<string, string>
  accountBalances: Record<string, string>
  tickerPrices: Record<string, string>
  disconnectInProgress: boolean
  onShowContent: () => void
  onDisableWidget: () => void
  onValidAuthCode: () => void
  onConnectGemini: () => void
  onUpdateActions: () => void
  onSetSelectedView: (view: string) => void
  onGeminiClientUrl: (url: string) => void
  onSetHideBalance: (hide: boolean) => void
  onDisconnectGemini: () => void
  onCancelDisconnect: () => void
}

class Gemini extends React.PureComponent<Props, State> {
  private refreshInterval: any
  private tradeTimer: any

  constructor (props: Props) {
    super(props)
    this.state = {
      currentDepositSearch: '',
      currentDepositAsset: '',
      currentQRAsset: '',
      currentTradeAsset: 'BAT',
      currentTradeQuantity: '',
      currentTradeMode: 'buy',
      tradeDropdownShowing: false,
      insufficientFunds: false,
      showTradePreview: false,
      tradeSuccess: false,
      tradeFailed: false,
      currentTradeId: '',
      currentTradeFee: '',
      currentTradePrice: '',
      currentTradeQuantityLive: '',
      currentTradeExpiryTime: 60
    }
  }

  componentDidMount () {
    const { userAuthed, authInProgress } = this.props

    if (userAuthed) {
      this.props.onUpdateActions()
    }

    if (authInProgress) {
      this.checkForOauthCode()
    }

    this.getClientURL()
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

  componentWillUnmount () {
    clearInterval(this.refreshInterval)
  }

  checkSetRefreshInterval = () => {
    if (!this.refreshInterval) {
      this.refreshInterval = setInterval(() => {
        this.props.onUpdateActions()
      }, 30000)
    }
  }

  clearIntervals = () => {
    clearInterval(this.refreshInterval)
  }

  getClientURL = () => {
    chrome.gemini.getClientUrl((clientUrl: string) => {
      this.props.onGeminiClientUrl(clientUrl)
    })
  }

  checkForOauthCode = () => {
    const params = window.location.search
    const urlParams = new URLSearchParams(params)
    const geminiAuth = urlParams.get('geminiAuth')

    if (geminiAuth) {
      chrome.gemini.getAccessToken((success: boolean) => {
        if (success) {
          this.props.onValidAuthCode()
        }
      })
    }
  }

  connectGemini = () => {
    window.open(this.props.geminiClientUrl, '_self', 'noopener')
    this.props.onConnectGemini()
  }

  cancelDisconnect = () => {
    this.props.onCancelDisconnect()
  }

  finishDisconnect = () => {
    this.clearIntervals()
    chrome.gemini.revokeToken(() => {
      this.props.onDisconnectGemini()
      this.cancelDisconnect()
    })
  }

  renderIndexView () {
    const {
      currentQRAsset,
      insufficientFunds,
      tradeFailed,
      showTradePreview
    } = this.state
    const { disconnectInProgress } = this.props

    if (currentQRAsset) {
      return this.renderQRView()
    } else if (disconnectInProgress) {
      return this.renderDisconnectView()
    } else if (insufficientFunds) {
      return this.renderInsufficientFundsView()
    } else if (tradeFailed) {
      return this.renderUnableToTradeView()
    } else if (showTradePreview) {
      return this.renderTradeConfirm()
    }

    return false
  }

  setSelectedView (view: string) {
    this.props.onSetSelectedView(view)
  }

  setCurrentDepositSearch = ({ target }: any) => {
    this.setState({
      currentDepositSearch: target.value
    })
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

  copyToClipboard = async (address: string) => {
    try {
      await clipboardCopy(address)
    } catch (e) {
      console.log(`Could not copy address ${e.toString()}`)
    }
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

  onSetHideBalance = () => {
    this.props.onSetHideBalance(
      !this.props.hideBalance
    )
  }

  formatCryptoBalance = (balance: string) => {
    if (!balance) {
      return '0'
    }

    return parseFloat(balance).toFixed(3)
  }

  getAccountUSDValue = () => {
    const { accountBalances, tickerPrices } = this.props
    let USDValue = 0.00

    for (let ticker in tickerPrices) {
      if (!(ticker in accountBalances)) {
        continue
      }

      const price = parseFloat(tickerPrices[ticker])
      const assetBalance = parseFloat(accountBalances[ticker])

      USDValue += price * assetBalance
    }

    return USDValue.toFixed(2)
  }

  renderSmallIconAsset = (key: string, isDetail: boolean = false) => {
    const iconColor = cryptoColors[key] || '#fff'

    return (
      <AssetIcon
        isDetail={isDetail}
        style={{ color: iconColor }}
        className={`crypto-icon icon-${key}`}
      />
    )
  }

  renderIconAsset = (key: string, isDetail: boolean = false) => {
    const iconColor = cryptoColors[key] || '#fff'
    const styles = { color: '#000' }

    if (this.props.selectedView === 'balance') {
      styles['marginTop'] = '5px'
      styles['marginLeft'] = '5px'
    }

    return (
      <AssetIconWrapper style={{ background: iconColor }}>
        <AssetIcon
          isDetail={isDetail}
          style={styles}
          className={`crypto-icon icon-${key}`}
        />
      </AssetIconWrapper>
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

  renderAuthView () {
    const { onDisableWidget } = this.props

    return (
      <>
        <IntroTitle>
          {'Purchase and trade with Gemini'}
        </IntroTitle>
        <Copy>
          {'Enable a Gemini connection to view your Gemini account balance and trade crypto.'}
        </Copy>
        <ActionsWrapper>
          {
            <>
              <ConnectButton onClick={this.connectGemini}>
                {getLocale('Connect to Gemini')}
              </ConnectButton>
              <DismissAction onClick={onDisableWidget}>
                {'No thank you'}
              </DismissAction>
            </>
          }
        </ActionsWrapper>
      </>
    )
  }

  renderSelectedView () {
    const { selectedView } = this.props

    switch (selectedView) {
      case 'deposit':
        return this.renderDepositView()
      case 'balance':
        return this.renderBalanceView()
      case 'trade':
        return this.renderTradeView()
      default:
        return null
    }
  }

  renderQRView () {
    const { assetAddressQRCodes } = this.props
    const imageSrc = assetAddressQRCodes[this.state.currentQRAsset]

    return (
      <SmallNoticeWrapper>
        <QRImage src={imageSrc} />
        <GenButton onClick={this.cancelQR}>
          {getLocale('binanceWidgetDone')}
        </GenButton>
      </SmallNoticeWrapper>
    )
  }

  renderBalanceView () {
    const {
      hideBalance,
      accountBalances
    } = this.props
    const accountUSDValue = this.getAccountUSDValue()

    return (
      <>
        <AccountSummary>
          <ListInfo position={'left'}>
            <TradeLabel>
              <Balance isSummary={true} hideBalance={hideBalance}>
                {`$${accountUSDValue}`}
              </Balance>
            </TradeLabel>
          </ListInfo>
          <ListInfo position={'right'} isSummary={true}>
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
        </AccountSummary>
        {geminiData.currencies.map((asset: string) => {
          const assetAccountBalance = accountBalances[asset] || '0'
          const assetBalance = this.formatCryptoBalance(assetAccountBalance)

          return (
            <ListItem key={`list-${asset}`}>
              <ListInfo isAsset={true} position={'left'}>
                <ListIcon>
                  {this.renderIconAsset(asset.toLowerCase())}
                </ListIcon>
                <ListLabel>
                  {geminiData.currencyNames[asset]}
                </ListLabel>
              </ListInfo>
              <ListInfo position={'right'}>
                <Balance isSummary={false} hideBalance={hideBalance}>
                  {assetBalance} {asset}
                </Balance>
              </ListInfo>
            </ListItem>
          )
        })}
      </>
    )
  }

  setCurrentTradeQuantity = ({ target }: any) => {
    this.setState({ currentTradeQuantity: target.value })
  }

  setCurrentTradeAsset (asset: string) {
    this.setState({
      currentTradeAsset: asset,
      tradeDropdownShowing: false
    })
  }

  handleTradeAssetChange = () => {
    this.setState({
      tradeDropdownShowing: !this.state.tradeDropdownShowing
    })
  }

  disableTradeAssetDropdown = () => {
    /*
    this.setState({
      tradeDropdownShowing: false
    })
    */
  }

  toggleCurrentTradeMode = () => {
    const { currentTradeMode } = this.state
    const newMode = currentTradeMode === 'buy' ? 'sell' : 'buy'
    this.setState({
      currentTradeMode: newMode
    })
  }

  retryTrade = () => {
    clearInterval(this.tradeTimer)
    this.setState({
      insufficientFunds: false,
      showTradePreview: false,
      tradeSuccess: false,
      tradeFailed: false,
      currentTradeId: '',
      currentTradeFee: '',
      currentTradePrice: '',
      currentTradeQuantity: '',
      currentTradeExpiryTime: 60
    })
  }

  cancelTrade = () => {
    clearInterval(this.tradeTimer)
    this.setState({
      insufficientFunds: false,
      showTradePreview: false,
      tradeSuccess: false,
      tradeFailed: false,
      currentTradeId: '',
      currentTradeFee: '',
      currentTradePrice: '',
      currentTradeQuantity: '',
      currentTradeExpiryTime: 60,
      currentTradeQuantityLive: '',
      currentTradeAsset: 'BAT'
    })
  }

  shouldShowTradePreview () {
    const {
      currentTradeMode,
      currentTradeAsset,
      currentTradeQuantity
    } = this.state
    const { accountBalances } = this.props
    const compare = currentTradeMode === 'buy'
      ? (accountBalances['USD'] || '0')
      : (accountBalances[currentTradeAsset] || '0')

    if (parseFloat(currentTradeQuantity) >= parseFloat(compare)) {
      this.setState({
        insufficientFunds: true
      })
      return
    }

    chrome.gemini.getOrderQuote(currentTradeMode, `${currentTradeAsset}usd`, currentTradeQuantity, (quote: any) => {
      if (!quote.id || !quote.quantity || !quote.fee || !quote.price) {
        this.setState({
          tradeFailed: true
        })
        return
      }

      this.setState({
        currentTradeId: quote.id,
        currentTradeFee: quote.fee,
        currentTradePrice: quote.price,
        currentTradeQuantityLive: quote.quantity,
        showTradePreview: true
      })

      this.tradeTimer = setInterval(() => {
        const { currentTradeExpiryTime } = this.state

        if (currentTradeExpiryTime - 1 === 0) {
          clearInterval(this.tradeTimer)
          this.cancelTrade()
          return
        }

        this.setState({
          currentTradeExpiryTime: (currentTradeExpiryTime - 1)
        })
      }, 1000)
    })
  }

  renderInsufficientFundsView = () => {
    return (
      <InvalidWrapper>
        <InvalidTitle>
          {'Unable to perform trade'}
        </InvalidTitle>
        <InvalidCopy>
          {getLocale('binanceWidgetInsufficientFunds')}
        </InvalidCopy>
        <GenButton onClick={this.retryTrade}>
          {getLocale('binanceWidgetRetry')}
        </GenButton>
      </InvalidWrapper>
    )
  }

  renderUnableToTradeView = () => {
    return (
      <InvalidWrapper>
        <InvalidTitle>
          {'Unable to perform trade'}
        </InvalidTitle>
        <InvalidCopy>
          {'Something went wrong'}
        </InvalidCopy>
        <GenButton onClick={this.retryTrade}>
          {getLocale('binanceWidgetRetry')}
        </GenButton>
      </InvalidWrapper>
    )
  }

  renderTradeConfirm = () => {
    const {
      currentTradeQuantityLive,
      currentTradeFee,
      currentTradeExpiryTime
    } = this.state
    console.log(currentTradeExpiryTime, currentTradeFee, currentTradeQuantityLive)

    return (
      <InvalidWrapper>
        <InvalidTitle>
          {getLocale('binanceWidgetConfirmConversion')}
        </InvalidTitle>
        <ActionsWrapper>
          <ConnectButton isSmall={true}>
            {`${getLocale('binanceWidgetConfirm')} (${currentTradeExpiryTime}s)`}
          </ConnectButton>
          <DismissAction onClick={this.cancelTrade}>
            {getLocale('binanceWidgetCancel')}
          </DismissAction>
        </ActionsWrapper>
      </InvalidWrapper>
    )
  }

  renderTradeView () {
    const {
      currentTradeMode,
      currentTradeAsset,
      currentTradeQuantity,
      tradeDropdownShowing
    } = this.state
    const { accountBalances } = this.props
    const currentAssetBalance = this.formatCryptoBalance(accountBalances[currentTradeAsset] || '0')
    const accountUSDBalance = accountBalances['USD'] || '0.00'
    const availableAmount = currentTradeMode === 'buy' ? accountUSDBalance : currentAssetBalance
    const availableLabel = currentTradeMode === 'buy' ? 'USD' : currentTradeAsset

    return (
      <>
        <Copy>
          {`${getLocale('binanceWidgetAvailable')} ${availableAmount} ${availableLabel}`}
        </Copy>
        <TradeSwitchWrapper>
          <TradeSwitch
            onClick={this.toggleCurrentTradeMode}
            isActive={currentTradeMode === 'buy'}
          >
            {'Buy'}
          </TradeSwitch>
          <TradeSwitch
            onClick={this.toggleCurrentTradeMode}
            isActive={currentTradeMode === 'sell'}
          >
            {'Sell'}
          </TradeSwitch>
        </TradeSwitchWrapper>
        <TradeWrapper>
          <InputWrapper>
            <AmountInputField
              type={'text'}
              placeholder={`I want to ${currentTradeMode}...`}
              value={currentTradeQuantity}
              onChange={this.setCurrentTradeQuantity}
            />
            <Dropdown
              disabled={false}
              itemsShowing={false}
              className={'asset-dropdown'}
            >
              <CaratDropdown hide={true}>
                <CaratDownIcon />
              </CaratDropdown>
            </Dropdown>
          </InputWrapper>
          <AssetDropdown
            itemsShowing={false}
            className={'asset-dropdown'}
            onClick={this.handleTradeAssetChange}
          >
            <AssetDropdownLabel>
              <DropdownIcon>
                {this.renderSmallIconAsset(currentTradeAsset.toLowerCase())}
              </DropdownIcon>
              {currentTradeAsset}
            </AssetDropdownLabel>
            <CaratDropdown>
              <CaratDownIcon />
            </CaratDropdown>
          </AssetDropdown>
          {
            tradeDropdownShowing
            ? <AssetItems>
                {geminiData.currencies.map((asset: string, i: number) => {
                  if (asset === currentTradeAsset) {
                    return null
                  }

                  return (
                    <AssetItem
                      key={`choice-${asset}`}
                      isLast={i === geminiData.currencies.length - 1}
                      onClick={this.setCurrentTradeAsset.bind(this, asset)}
                    >
                      <DropdownIcon>
                        {this.renderSmallIconAsset(asset.toLowerCase())}
                      </DropdownIcon>
                      {asset}
                    </AssetItem>
                  )
                })}
              </AssetItems>
            : null
          }
        </TradeWrapper>
        <ActionsWrapper>
          <ActionButton onClick={this.shouldShowTradePreview}>
            {'Get a quote'}
          </ActionButton>
        </ActionsWrapper>
      </>
    )
  }

  renderCurrentDepositAsset = () => {
    const { currentDepositAsset } = this.state
    const { assetAddresses } = this.props
    const currentDepositAddress = assetAddresses[currentDepositAsset] || ''

    return (
      <>
        <ListItem>
          <DetailIcons>
            <BackArrow>
              <CaratLeftIcon
                onClick={this.setCurrentDepositAsset.bind(this, '')}
              />
            </BackArrow>
            <DetailIconWrapper>
              {this.renderIconAsset(currentDepositAsset.toLowerCase(), true)}
            </DetailIconWrapper>
          </DetailIcons>
          <AssetTicker>
            {currentDepositAsset}
          </AssetTicker>
          <AssetLabel>
            {geminiData.currencies[currentDepositAsset]}
          </AssetLabel>
          {
            currentDepositAddress
            ? <AssetQR onClick={this.setQR.bind(this, currentDepositAsset)}>
                <img style={{ width: '25px', marginRight: '5px' }} src={QRIcon} />
              </AssetQR>
            : null
          }
        </ListItem>
        <DetailArea>
          {
            !currentDepositAddress
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
            currentDepositAddress
            ? <MemoArea>
                <MemoInfo>
                  <DetailLabel>
                    {`${currentDepositAsset} ${getLocale('binanceWidgetDepositAddress')}`}
                  </DetailLabel>
                  <DetailInfo>
                    {currentDepositAddress}
                  </DetailInfo>
                </MemoInfo>
                <CopyButton onClick={this.copyToClipboard.bind(this, currentDepositAddress)}>
                  {getLocale('binanceWidgetCopy')}
                </CopyButton>
              </MemoArea>
            : null
          }
        </DetailArea>
      </>
    )
  }

  renderDepositView () {
    const { currentDepositSearch, currentDepositAsset } = this.state

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
        {geminiData.currencies.map((asset: string) => {
          const cleanName = geminiData.currencyNames[asset]
          const lowerAsset = asset.toLowerCase()
          const lowerName = cleanName.toLowerCase()
          const lowerSearch = currentDepositSearch.toLowerCase()

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
                {`${asset} (${cleanName})`}
              </ListLabel>
            </ListItem>
          )
        })}
      </>
    )
  }

  renderAccountView () {
    const { selectedView } = this.props
    const { currentDepositAsset } = this.state
    const isBalanceView = !selectedView || selectedView === 'balance'
    const hideOverflow = currentDepositAsset && selectedView === 'deposit'

    return (
      <>
        <NavigationBar>
            <NavigationItem
              tabIndex={0}
              isActive={selectedView === 'deposit'}
              onClick={this.setSelectedView.bind(this, 'deposit')}
            >
              {getLocale('binanceWidgetDepositLabel')}
            </NavigationItem>
            <NavigationItem
              tabIndex={0}
              isActive={selectedView === 'trade'}
              onClick={this.setSelectedView.bind(this, 'trade')}
            >
              {'Trade'}
            </NavigationItem>
            <NavigationItem
              tabIndex={0}
              isLast={true}
              isActive={isBalanceView}
              onClick={this.setSelectedView.bind(this, 'balance')}
            >
              {'Balance'}
            </NavigationItem>
        </NavigationBar>
        {
          selectedView === 'trade'
          ? this.renderSelectedView()
          : <SelectedView hideOverflow={!!hideOverflow}>
              {this.renderSelectedView()}
            </SelectedView>
        }
      </>
    )
  }

  renderRoutes () {
    const { userAuthed } = this.props

    if (userAuthed) {
      return this.renderAccountView()
    }

    return this.renderAuthView()
  }

  renderTitle () {
    return (
      <Header>
        <StyledTitle>
          <GeminiIcon>
            <GeminiLogo />
          </GeminiIcon>
          <StyledTitleText>
            {'Gemini'}
          </StyledTitleText>
        </StyledTitle>
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

  render () {
    const { showContent, userAuthed } = this.props

    if (!showContent) {
      return this.renderTitleTab()
    }

    return (
      <WidgetWrapper tabIndex={0} userAuthed={userAuthed} onClick={this.disableTradeAssetDropdown}>
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

export const GeminiWidget = createWidget(Gemini)
