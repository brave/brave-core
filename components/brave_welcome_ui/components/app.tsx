/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
const { bindActionCreators } = require('redux')
const { connect } = require('react-redux')
const welcomeActions = require('../actions/welcome_actions')

// Components
const { UnstyledButton } = require('brave-ui')
const Panel = require('brave-ui/v1/panel').default

// Theme
const theme = require('../theme')

// Screens
const BraveScreen = require('./braveScreen')
const RewardsScreen = require('./rewardsScreen')
const ImportScreen = require('./importScreen')
const ShieldsScreen = require('./shiedsScreen')
const FeaturesScreen = require('./featuresScreen')
const Footer = require('./footer')

// Images
const background = require('../../img/welcome/welcomebg.svg')

// Fonts
require('../../fonts/muli.css')
require('../../fonts/poppins.css')

class WelcomePage extends React.Component {
  constructor (props) {
    super(props)
    this.onImportNowClicked = this.onImportNowClicked.bind(this)
    this.onGoToFirstSlide = this.onGoToSlide.bind(this, 1)
    this.onClickNext = this.onClickNext.bind(this)
  }

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
        return <BraveScreen theme={theme} onGoToFirstSlide={this.onGoToFirstSlide} />
      case 1:
        return <RewardsScreen theme={theme} />
      case 2:
        return <ImportScreen theme={theme} onImportNowClicked={this.onImportNowClicked} />
      case 3:
        return <ShieldsScreen theme={theme} />
      case 4:
        return <FeaturesScreen theme={theme} />
      default:
        return <BraveScreen theme={theme} />
    }
  }

  get slideBullets () {
    return Array.from({length: this.totalSecondaryScreensSize}, (v, k) => {
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

  onGoToSlide (nextPage) {
    this.actions.goToPageRequested(nextPage)
  }

  onClickNext () {
    this.actions.goToPageRequested(this.pageIndex + 1)
  }

  onImportNowClicked () {
    this.actions.importNowRequested()
  }

  render () {
    return (
      <div style={this.backgroundStyle}>
        <Panel theme={theme.panel}>
          { this.activeScreen }
          <Footer
            theme={theme}
            pageIndex={this.pageIndex}
            totalSecondaryScreensSize={this.totalSecondaryScreensSize}
            onClickNext={this.onClickNext}
          >
            { this.slideBullets }
          </Footer>
        </Panel>
      </div>
    )
  }
}

const mapStateToProps = (state) => ({
  welcomeData: state.welcomeData
})

const mapDispatchToProps = (dispatch) => ({
  actions: bindActionCreators(welcomeActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(WelcomePage)
