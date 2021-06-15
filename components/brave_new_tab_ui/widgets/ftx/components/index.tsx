// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ThemeProvider, ThemeConsumer } from 'styled-components'
import { CaratLeftIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../common/locale'
import Loading from '../../../components/loading'
import createWidget from '../../../components/default/widget/index'
import { StyledTitleTab } from '../../../components/default/widgetTitleTab'
import {
  BackArrow,
  BasicBox,
  WidgetIcon,
  Header,
  OptionButton,
  StyledTitle,
  StyledTitleText,
  WidgetWrapper
} from '../../shared/styles'
import * as FTXActions from '../ftx_actions'
import { FTXState, ViewType } from '../ftx_state'
import ftxLogo from './ftx-logo.png'
import customizeTheme from './theme'
import Convert from './convert'
import AssetDetail from './assetDetail'
import PreOptIn from './preOptIn'
import Markets from './markets'
import Summary from './summary'

// Utils
interface State {
  hideBalance: boolean
}

interface Props {
  ftx: FTXState
  actions: typeof FTXActions
  widgetTitle: string
  showContent: boolean
  stackPosition: number
  onShowContent: () => void
}

class FTX extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      hideBalance: true
    }
  }

  componentDidMount () {
    if (this.props.showContent && !this.props.ftx.hasInitialized) {
      this.props.actions.initialize()
    }
  }

  componentDidUpdate (prevProps: Props) {
    const isNewlyShown = this.props.showContent && !prevProps.showContent
    if (isNewlyShown && !this.props.ftx.hasInitialized) {
      this.props.actions.initialize()
    }
  }

  toggleBalanceVisibility = () => {
    this.setState({
      hideBalance: !this.state.hideBalance
    })
  }

  renderView () {
    const selectedAsset = this.props.ftx.assetDetail?.currencyName
    const { currentView } = this.props.ftx
    if (selectedAsset) {
      return <AssetDetail ftx={this.props.ftx} actions={this.props.actions} />
    } else if (currentView === ViewType.Convert) {
      return <Convert ftx={this.props.ftx} actions={this.props.actions} />
    } else if (currentView === ViewType.Summary) {
      return (
        <Summary
          ftx={this.props.ftx}
          actions={this.props.actions}
          hideBalance={this.state.hideBalance}
          onToggleBalanceVisibility={this.toggleBalanceVisibility}
        />
      )
    } else {
      return <Markets ftx={this.props.ftx} actions={this.props.actions} />
    }
  }

  setView = (view: ViewType) => {
    this.props.actions.openView(view)
  }

  renderContent () {
    const { currentView, isConnected, hasInitialized } = this.props.ftx
    if (!hasInitialized) {
      return <BasicBox $height={250}><Loading /></BasicBox>
    }
    if (!isConnected && this.props.ftx.currentView === ViewType.OptIn) {
      return <PreOptIn ftx={this.props.ftx} actions={this.props.actions} />
    }
    return (
      <>
        {isConnected &&
        <BasicBox isFlex={true} $mb={10} $gap={10} justify='start'>
          <OptionButton isSelected={currentView === ViewType.Markets} onClick={this.setView.bind(null, ViewType.Markets)}>{getLocale('ftxMarkets')}</OptionButton>
          <OptionButton isSelected={currentView === ViewType.Convert} onClick={this.setView.bind(null, ViewType.Convert)}>{getLocale('ftxConvert')}</OptionButton>
          <OptionButton isSelected={currentView === ViewType.Summary} onClick={this.setView.bind(null, ViewType.Summary)}>{getLocale('ftxSummary')}</OptionButton>
        </BasicBox>
        }
        {this.renderView()}
      </>
    )
  }

  renderTitle () {
    const { showContent, widgetTitle } = this.props
    // Only show back arrow to go back to opt-in view
    const shouldShowBackArrow = showContent &&
      this.props.ftx.currentView === ViewType.Markets &&
      !this.props.ftx.isConnected

    return (
      <Header showContent={showContent}>
        <StyledTitle>
          <WidgetIcon>
            <img src={ftxLogo} alt='FTX logo'/>
          </WidgetIcon>
          <StyledTitleText>
            {widgetTitle}
          </StyledTitleText>
          {shouldShowBackArrow &&
            <BackArrow marketView={true}>
              <CaratLeftIcon
                onClick={this.props.actions.preOptInViewMarkets.bind(undefined, { hide: true })}
              />
            </BackArrow>
          }
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
      <ThemeConsumer>
      {theme =>
        <ThemeProvider theme={customizeTheme(theme)}>
          <WidgetWrapper tabIndex={0}>
            {this.renderTitle()}
            {this.renderContent()}
          </WidgetWrapper>
        </ThemeProvider>
      }
      </ThemeConsumer>
    )
  }
}

export const FTXWidget = createWidget(FTX)
