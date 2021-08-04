/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

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
  InvalidCopy,
  TradeInfoWrapper,
  TradeInfoItem,
  TradeItemLabel,
  TradeValue,
  StyledParty,
  SmallButton
} from './style'
import {
  SearchIcon,
  QRIcon,
  ShowIcon,
  HideIcon
} from '../exchangeWidget/shared-assets'
import GeminiLogo from './assets/gemini-logo'
import { CaratLeftIcon, CaratDownIcon } from 'brave-ui/components/icons'

import * as S from '../../../widgets/shared/styles'
import IconAsset from '../../../widgets/shared/iconAsset'

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
  tradeError: string
  currentTradeId: string
  currentTradeFee: string
  currentTradePrice: string
  currentTradeTotalPrice: string
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
  authInvalid: boolean
  stackPosition: number
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
  onDismissAuthInvalid: () => void
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
      tradeError: '',
      currentTradeId: '',
      currentTradeFee: '',
      currentTradePrice: '',
      currentTradeTotalPrice: '',
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
      showTradePreview,
      tradeSuccess
    } = this.state
    const { authInvalid, disconnectInProgress } = this.props

    if (authInvalid) {
      return this.renderAuthInvalid()
    } else if (currentQRAsset) {
      return this.renderQRView()
    } else if (disconnectInProgress) {
      return this.renderDisconnectView()
    } else if (insufficientFunds) {
      return this.renderInsufficientFundsView()
    } else if (tradeFailed) {
      return this.renderUnableToTradeView()
    } else if (tradeSuccess) {
      return this.renderTradeSuccess()
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
      await navigator.clipboard.writeText(address)
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

  formatCryptoBalance = (balance: string, precision: number = 3) => {
    if (!balance) {
      return '0'
    }

    return parseFloat(balance).toFixed(precision)
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

    if ('USD' in accountBalances) {
      USDValue += parseFloat(accountBalances['USD'])
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

  renderAuthInvalid = () => {
    const { onDismissAuthInvalid } = this.props

    return (
      <InvalidWrapper>
        <InvalidTitle>
          {getLocale('geminiWidgetAuthInvalid')}
        </InvalidTitle>
        <InvalidCopy>
          {getLocale('geminiWidgetAuthInvalidCopy')}
        </InvalidCopy>
        <GenButton onClick={onDismissAuthInvalid}>
          {getLocale('geminiWidgetDone')}
        </GenButton>
      </InvalidWrapper>
    )
  }

  renderDisconnectView = () => {
    return (
      <DisconnectWrapper>
        <DisconnectTitle>
          {getLocale('geminiWidgetDisconnectTitle')}
        </DisconnectTitle>
        <DisconnectCopy>
          {getLocale('geminiWidgetDisconnectText')}
        </DisconnectCopy>
        <DisconnectButton onClick={this.finishDisconnect}>
          {getLocale('geminiWidgetDisconnectButton')}
        </DisconnectButton>
        <DismissAction onClick={this.cancelDisconnect}>
          {getLocale('geminiWidgetCancelText')}
        </DismissAction>
      </DisconnectWrapper>
    )
  }

  renderAuthView () {
    return (
      <>
        <IntroTitle>
          {getLocale('geminiWidgetConnectTitle')}
        </IntroTitle>
        <Copy>
          {getLocale('geminiWidgetConnectCopy')}
        </Copy>
        <ActionsWrapper isAuth={true}>
          <ConnectButton onClick={this.connectGemini}>
            {getLocale('geminiWidgetConnectButton')}
          </ConnectButton>
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
          {getLocale('geminiWidgetDone')}
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
    const balanceKeys = Object.keys(accountBalances)

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
        {balanceKeys.length !== 0
        ?
        <>
          {balanceKeys.map((asset: string) => {
            const assetAccountBalance = accountBalances[asset]
            const assetBalance = this.formatCryptoBalance(assetAccountBalance)
            return (
              <S.ListItem key={`list-${asset}`} isFlex={true} $p={10}>
                <S.FlexItem $mr={10} $w={25} $h={25}>
                  <IconAsset iconKey={asset.toLowerCase()} />
                </S.FlexItem>
                <S.FlexItem>
                  <S.Text>{geminiData.currencyNames[asset]}</S.Text>
                </S.FlexItem>
                <S.FlexItem textAlign='right' flex={1}>
                  <S.Balance hideBalance={hideBalance}>
                    <S.Text lineHeight={1.15}>{assetBalance} {asset}</S.Text>
                  </S.Balance>
                </S.FlexItem>
              </S.ListItem>
            )
          })}
        </>
        : <S.Balance hideBalance={hideBalance}>
            <S.Text lineHeight={1.15} $p={12}>{getLocale('geminiWidgetSummaryNoBalance')}</S.Text>
          </S.Balance>
        }
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
      tradeError: '',
      currentTradeId: '',
      currentTradeFee: '',
      currentTradePrice: '',
      currentTradeTotalPrice: '',
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
      currentTradeTotalPrice: '',
      currentTradeQuantity: '',
      currentTradeExpiryTime: 60,
      currentTradeQuantityLive: '',
      currentTradeAsset: 'BAT'
    })
  }

  processTrade = () => {
    const {
      currentTradeId,
      currentTradeAsset,
      currentTradeFee: fee,
      currentTradePrice: price,
      currentTradeMode: side,
      currentTradeQuantityLive: quantity
    } = this.state
    const quoteId = parseInt(currentTradeId, 10)
    const symbol = `${currentTradeAsset}usd`.toUpperCase()
    chrome.gemini.executeOrder(symbol, side, quantity, price, fee, quoteId, (success: boolean) => {
      if (success) {
        this.setState({ tradeSuccess: true })
        setTimeout(() => {
          this.props.onUpdateActions()
        }, 1000)
      } else {
        this.setTradeFailed()
      }
    })
  }

  finishTrade = () => {
    this.cancelTrade()
    this.props.onSetSelectedView('balance')
  }

  setTradeFailed = (message: string = '') => {
    this.setState({
      tradeFailed: true,
      tradeError: message
    })
    clearInterval(this.tradeTimer)
  }

  shouldShowTradePreview = () => {
    const {
      currentTradeMode,
      currentTradeAsset,
      currentTradeQuantity
    } = this.state
    const { accountBalances } = this.props

    if (!currentTradeQuantity || isNaN(parseFloat(currentTradeQuantity))) {
      return
    }

    const compare = currentTradeMode === 'buy'
      ? (accountBalances['USD'] || '0')
      : (accountBalances[currentTradeAsset] || '0')

    if (parseFloat(currentTradeQuantity) >= parseFloat(compare)) {
      this.setState({
        insufficientFunds: true
      })
      return
    }

    chrome.gemini.getOrderQuote(currentTradeMode, `${currentTradeAsset}usd`, currentTradeQuantity, (quote: any, error: string) => {
      if (!quote.id || !quote.quantity || !quote.fee || !quote.price || !quote.totalPrice) {
        this.setTradeFailed(error)
        return
      }

      this.setState({
        currentTradeId: quote.id,
        currentTradeFee: quote.fee,
        currentTradePrice: quote.price,
        currentTradeTotalPrice: quote.totalPrice,
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
          {getLocale('geminiWidgetFailedTrade')}
        </InvalidTitle>
        <InvalidCopy>
          {getLocale('geminiWidgetInsufficientFunds')}
        </InvalidCopy>
        <GenButton onClick={this.retryTrade}>
          {getLocale('geminiWidgetRetry')}
        </GenButton>
      </InvalidWrapper>
    )
  }

  renderUnableToTradeView = () => {
    const { tradeError } = this.state
    const errorMessage = tradeError || getLocale('geminiWidgetError')

    return (
      <InvalidWrapper>
        <InvalidTitle>
          {getLocale('geminiWidgetFailedTrade')}
        </InvalidTitle>
        <InvalidCopy>
          {errorMessage}
        </InvalidCopy>
        <GenButton onClick={this.retryTrade}>
          {getLocale('geminiWidgetRetry')}
        </GenButton>
      </InvalidWrapper>
    )
  }

  renderTradeSuccess = () => {
    const {
      currentTradeAsset,
      currentTradeQuantityLive,
      currentTradeMode
    } = this.state
    const quantity = this.formatCryptoBalance(currentTradeQuantityLive)
    const actionLabel = currentTradeMode === 'buy' ? 'geminiWidgetBought' : 'geminiWidgetSold'

    return (
      <InvalidWrapper>
        <StyledParty>
          🎉
        </StyledParty>
        <InvalidTitle>
          {`${getLocale(actionLabel)} ${quantity} ${currentTradeAsset}!`}
        </InvalidTitle>
        <SmallButton onClick={this.finishTrade}>
          {getLocale('geminiWidgetContinue')}
        </SmallButton>
      </InvalidWrapper>
    )
  }

  renderTradeConfirm = () => {
    const {
      currentTradeFee,
      currentTradeAsset,
      currentTradeQuantityLive,
      currentTradeMode,
      currentTradeExpiryTime,
      currentTradePrice,
      currentTradeTotalPrice
    } = this.state
    const isBuy = currentTradeMode === 'buy'
    const tradeLabel = isBuy ? 'geminiWidgetBuying' : 'geminiWidgetSelling'
    const quantity = this.formatCryptoBalance(currentTradeQuantityLive)
    const fee = this.formatCryptoBalance(currentTradeFee)
    const total = this.formatCryptoBalance(currentTradeTotalPrice, 2)
    const totalLabel = isBuy ? 'geminiWidgetTotalPrice' : 'geminiWidgetTotalAmount'

    return (
      <InvalidWrapper>
        <InvalidTitle>
          {getLocale('geminiWidgetConfirmTrade')}
        </InvalidTitle>
        <TradeInfoWrapper>
          <TradeInfoItem>
            <TradeItemLabel>{getLocale(tradeLabel)}</TradeItemLabel>
            <TradeValue>{`${quantity} ${currentTradeAsset}`}</TradeValue>
          </TradeInfoItem>
          <TradeInfoItem>
            <TradeItemLabel>{getLocale('geminiWidgetUnitPrice')}</TradeItemLabel>
            <TradeValue>{`${currentTradePrice} USD/${currentTradeAsset}`}</TradeValue>
          </TradeInfoItem>
          <TradeInfoItem>
            <TradeItemLabel>{getLocale('geminiWidgetFee')}</TradeItemLabel>
            <TradeValue>{`${fee} USD`}</TradeValue>
          </TradeInfoItem>
          <TradeInfoItem isLast={true}>
            <TradeItemLabel>{getLocale(totalLabel)}</TradeItemLabel>
            <TradeValue>{`${total} USD`}</TradeValue>
          </TradeInfoItem>
        </TradeInfoWrapper>
        <ActionsWrapper>
          <SmallButton onClick={this.processTrade}>
            {`${getLocale('geminiWidgetConfirm')} (${currentTradeExpiryTime}s)`}
          </SmallButton>
          <DismissAction onClick={this.cancelTrade}>
            {getLocale('geminiWidgetCancel')}
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
          {`${getLocale('geminiWidgetAvailable')} ${availableAmount} ${availableLabel}`}
        </Copy>
        <TradeSwitchWrapper>
          <TradeSwitch
            onClick={this.toggleCurrentTradeMode}
            isActive={currentTradeMode === 'buy'}
          >
            {getLocale('geminiWidgetBuy')}
          </TradeSwitch>
          <TradeSwitch
            onClick={this.toggleCurrentTradeMode}
            isActive={currentTradeMode === 'sell'}
          >
            {getLocale('geminiWidgetSell')}
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
              {
                currentTradeMode === 'buy'
                ? 'USD'
                : currentTradeAsset
              }
            </Dropdown>
          </InputWrapper>
          <AssetDropdown
            itemsShowing={false}
            className={'asset-dropdown'}
            onClick={this.handleTradeAssetChange}
          >
            <DropdownIcon>
              <IconAsset iconKey={currentTradeAsset.toLowerCase()} size={15} />
            </DropdownIcon>
            <AssetDropdownLabel>
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
                        <IconAsset iconKey={asset.toLowerCase()} size={15} />
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
            {getLocale('geminiWidgetGetQuote')}
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
              <IconAsset iconKey={currentDepositAsset.toLowerCase()} />
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
                    {getLocale('geminiWidgetUnavailable')}
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
                    {`${currentDepositAsset} ${getLocale('geminiWidgetDepositAddress')}`}
                  </DetailLabel>
                  <DetailInfo>
                    {currentDepositAddress}
                  </DetailInfo>
                </MemoInfo>
                <CopyButton onClick={this.copyToClipboard.bind(this, currentDepositAddress)}>
                  {getLocale('geminiWidgetCopy')}
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
            placeholder={getLocale('geminiWidgetSearch')}
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
            <S.ListItem
              key={`list-${asset}`}
              isFlex={true}
              justify='flex-start'
              $p={10}
              onClick={this.setCurrentDepositAsset.bind(this, asset)}
            >
              <S.FlexItem $mr={10} $w={25} $h={25}>
                <IconAsset iconKey={lowerAsset} />
              </S.FlexItem>
              <S.FlexItem>
                <S.Text $fontSize={12}>
                  {`${asset} (${cleanName})`}
                </S.Text>
              </S.FlexItem>
            </S.ListItem>
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
              {getLocale('geminiWidgetDepositLabel')}
            </NavigationItem>
            <NavigationItem
              tabIndex={0}
              isActive={selectedView === 'trade'}
              onClick={this.setSelectedView.bind(this, 'trade')}
            >
              {getLocale('geminiWidgetTradeLabel')}
            </NavigationItem>
            <NavigationItem
              tabIndex={0}
              isActive={isBalanceView}
              onClick={this.setSelectedView.bind(this, 'balance')}
            >
              {getLocale('geminiWidgetBalanceLabel')}
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
    const { onShowContent, stackPosition } = this.props

    return (
      <StyledTitleTab onClick={onShowContent} stackPosition={stackPosition}>
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
