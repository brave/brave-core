/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import { UnstyledButton } from 'brave-ui'
import Panel from 'brave-ui/v1/panel'

// Components
import BraveScreen from './braveScreen'
import RewardsScreen from './rewardsScreen'
import ImportScreen from './importScreen'
import ShieldsScreen from './shieldsScreen'
import FeaturesScreen from './featuresScreen'
import Footer from './footer'

// Constants
import { theme } from '../constants/theme'

// Utils
import * as welcomeActions from '../actions/welcome_actions'

// Assets
const background = require('../../img/welcome/welcomebg.svg')
require('../../fonts/muli.css')
require('../../fonts/poppins.css')

interface Props {
  welcomeData: Welcome.State
  actions: any
}

export class WelcomePage extends React.Component<Props, {}> {
  get pageIndex () {
    return this.props.welcomeData.pageIndex
  }

  get actions () {
    return this.props.actions
  }

  get totalSecondaryScreensSize () {
    return [
      BraveScreen,
      RewardsScreen,
      ImportScreen,
      ShieldsScreen,
      FeaturesScreen
    ].length
  }

  get activeScreen () {
    switch (this.pageIndex) {
      case 0:
        return <BraveScreen onGoToFirstSlide={this.onGoToFirstSlide} />
      case 1:
        return <RewardsScreen />
      case 2:
        return <ImportScreen onImportNowClicked={this.onImportNowClicked} />
      case 3:
        return <ShieldsScreen />
      case 4:
        return <FeaturesScreen />
      default:
        return <BraveScreen onGoToFirstSlide={this.onGoToFirstSlide} />
    }
  }

  get slideBullets () {
    return Array.from({ length: this.totalSecondaryScreensSize }, (v, k) => {
      return (
        <UnstyledButton
          theme={
            this.pageIndex === k
              ? theme.bulletActive
              : theme.bullet
          }
          text='•'
          key={k}
          onClick={this.onGoToSlide.bind(this, k)}
        />
      )
    })
  }

  get backgroundStyle () {
    return {
      fontFamily: '"Poppins", sans-serif',
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center',
      height: '-webkit-fill-available',
      width: '-webkit-fill-available',
      transition: 'background-position-x 0.6s ease-out',
      backgroundRepeat: 'repeat-x',
      backgroundImage: `url('${background}')`,
      backgroundPositionX: this.backgroundPosition
    }
  }

  get backgroundPosition () {
    const { welcomeData } = this.props
    switch (welcomeData.pageIndex) {
      case 0:
        return '0%'
      case 1:
        return '100%'
      case 2:
        return '200%'
      case 3:
        return '300%'
      case 4:
        return '400%'
      default:
        return '0%'
    }
  }

  onGoToFirstSlide = () => {
    this.onGoToSlide(1)
  }

  onGoToSlide = (nextPage: number) => {
    this.actions.goToPageRequested(nextPage)
  }

  onClickNext = () => {
    this.actions.goToPageRequested(this.pageIndex + 1)
  }

  onImportNowClicked = () => {
    this.actions.importNowRequested()
  }

  render () {
    return (
      <div id='welcomePage' style={this.backgroundStyle}>
        <Panel theme={theme.panel}>
          {this.activeScreen}
          <Footer
            pageIndex={this.pageIndex}
            totalSecondaryScreensSize={this.totalSecondaryScreensSize}
            onClickNext={this.onClickNext}
          >
            {this.slideBullets}
          </Footer>
        </Panel>
      </div>
    )
  }
}

export const mapStateToProps = (state: Welcome.ApplicationState) => ({
  welcomeData: state.welcomeData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(welcomeActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(WelcomePage)
