/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import createWidget from '../widget/index'
import { StyledTitleTab } from '../widgetTitleTab'

import {
  WidgetWrapper,
  Header,
  StyledTitle,
  CryptoDotComIcon,
  StyledTitleText,
  Text,
  Box,
  ListItem,
  ListInfo,
  ListLabel,
  ListIcon,
  Balance,
  ActionButton,
  AssetIconWrapper,
  AssetIcon
} from './style'
// import {
//   SearchIcon,
//   ShowIcon,
//   HideIcon
// } from '../exchangeWidget/shared-assets'
import CryptoDotComLogo from './assets/cryptoDotCom-logo'
// import { CaratLeftIcon, CaratDownIcon } from 'brave-ui/components/icons'

// Utils
import cryptoColors from '../exchangeWidget/colors'
// import { getLocale } from '../../../../common/locale'

interface State {
}

interface Props {
  showContent: boolean
  optInTotal: boolean
  optInBTCPrice: boolean
  tickerPrices: Record<string, string>
  stackPosition: number
  onShowContent: () => void
  onDisableWidget: () => void
  onTotalPriceOptIn: () => void
  onBtcPriceOptIn: () => void
}

class CryptoDotCom extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {}
  }

  componentDidMount () {}

  componentDidUpdate (prevProps: Props) {}

  componentWillUnmount () {}

  renderIconAsset = (key: string, isDetail: boolean = false) => {
    const iconColor = cryptoColors[key] || '#fff'
    const styles = { color: '#000', marginTop: '5px', marginLeft: '5px' }

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

  renderIndexView () {
    const currency = "BTC";
    return <>
      <Box>
        <ListItem hasBorder={false}>
          <ListInfo isAsset={true} position={'left'}>
            <ListIcon>
              {this.renderIconAsset(currency.toLowerCase())}
            </ListIcon>
            <ListLabel>
              {currency}
            </ListLabel>
          </ListInfo>
          <ListInfo position={'right'}>
            <Balance>
              {0.00} {currency}
            </Balance>
          </ListInfo>
        </ListItem>
      </Box>
      <Text>Connect to Crypto.com to view buy and trade crypto, view account balance and upcoming events.</Text>
      <ActionButton>View Top Movers</ActionButton>
    </>
  }

  renderTitle () {
    const { showContent } = this.props;
    return (
      <Header showContent={showContent}>
        <StyledTitle>
          <CryptoDotComIcon>
            <CryptoDotComLogo />
          </CryptoDotComIcon>
          <StyledTitleText>
            {'Crypto.com'}
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
    const { showContent } = this.props

    if (!showContent) {
      return this.renderTitleTab()
    }

    return (
      <WidgetWrapper tabIndex={0}>
        {this.renderTitle()}
        {this.renderIndexView()}
      </WidgetWrapper>
    )
  }
}

export const CryptoDotComWidget = createWidget(CryptoDotCom)
