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
  DetailIconWrapper
} from './style'
import {
  SearchIcon,
  QRIcon
} from '../exchangeWidget/shared-assets'
import GeminiLogo from './assets/gemini-logo'
import { CaratLeftIcon } from 'brave-ui/components/icons'

// Utils
import geminiData from './data'
import cryptoColors from '../exchangeWidget/colors'
import { getLocale } from '../../../../common/locale'

interface State {
  currentDepositSearch: string
  currentDepositAsset: string
  currentQRAsset: string
}

interface Props {
  showContent: boolean
  userAuthed: boolean
  authInProgress: boolean
  geminiClientUrl: string
  selectedView: string
  assetAddresses: Record<string, string>
  assetAddressQRCodes: Record<string, string>
  onShowContent: () => void
  onDisableWidget: () => void
  onValidAuthCode: () => void
  onConnectGemini: () => void
  onUpdateActions: () => void
  onSetSelectedView: (view: string) => void
  onGeminiClientUrl: (url: string) => void
}

class Gemini extends React.PureComponent<Props, State> {
  private refreshInterval: any

  constructor (props: Props) {
    super(props)
    this.state = {
      currentDepositSearch: '',
      currentDepositAsset: '',
      currentQRAsset: ''
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
        this.props.onValidAuthCode()
      })
    }
  }

  connectGemini = () => {
    window.open(this.props.geminiClientUrl, '_self', 'noopener')
    this.props.onConnectGemini()
  }

  renderIndexView () {
    const { currentQRAsset } = this.state

    if (currentQRAsset) {
      this.renderQRView()
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
        return null
      case 'trade':
        return null
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
      <WidgetWrapper tabIndex={0} userAuthed={userAuthed}>
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
