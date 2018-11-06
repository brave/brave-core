/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Background, Page, Panel, BackgroundContainer } from '../../../../src/features/welcome/'

// Component groups
import WelcomeBox from './screens/welcomeBox'
import ImportBox from './screens/importBox'
import ShieldsBox from './screens/shieldsBox'
import SearchBox from './screens/searchBox'
import RewardsBox from './screens/rewardsBox'
import ThemeBox from './screens/themeBox'
import FooterBox from './screens/footerBox'

// Images
const background = require('../../../assets/img/welcome_bg.svg')

// Fonts
import '../../../assets/fonts/muli.css'
import '../../../assets/fonts/poppins.css'
// import FooterBox from './screens/footer';

export interface State {
  currentScreen: number
  fakeChangedSearchEngine: boolean
  fakeBookmarksImported: boolean
  fakeChangedDefaultTheme: boolean
}

export default class WelcomePage extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = {
      currentScreen: 1,
      fakeChangedSearchEngine: false,
      fakeBookmarksImported: false,
      fakeChangedDefaultTheme: false
    }
  }

  get totalScreensSize () {
    return 6
  }

  onClickLetsGo = () => {
    this.setState({ currentScreen: this.state.currentScreen + 1 })
  }

  onClickSlideBullet = (nextScreen: number) => {
    this.setState({ currentScreen: nextScreen })
  }

  onClickNext = () => {
    this.setState({ currentScreen: this.state.currentScreen + 1 })
  }

  onClickDone = () => {
    // fades out
    // fades in to new tab page
  }

  onClickSkip = () => {
    // fades out
    // fades in to new tab page
  }

  onClickImport = () => {
    this.setState({ fakeBookmarksImported: !this.state.fakeBookmarksImported })
    // prob not the correct syntax for this behavior, but a reminder that
    // clicking this button executes functionality and then auto proceed to next screen
    this.setState({ currentScreen: this.state.currentScreen + 1 })
    console.log('IMPORTED!')
  }

  onClickConfirmDefaultSearchEngine = () => {
    this.setState({ fakeChangedSearchEngine: !this.state.fakeChangedSearchEngine })
    this.setState({ currentScreen: this.state.currentScreen + 1 })
    console.log('CHANGED DEFAULT SEARCH ENGINE!')
  }

  onClickChooseYourTheme = () => {
    this.setState({ fakeChangedDefaultTheme: !this.state.fakeChangedDefaultTheme })
    this.setState({ currentScreen: this.state.currentScreen + 1 })
    console.log('NEW THEME CHOOSED')
  }

  onClickRewardsGetStarted = () => {
    console.log('SENT TO REWARDS PAGE')
  }

  get backgroundPosition () {
    switch (this.state.currentScreen) {
      case 1:
        return '100%'
      case 2:
        return '200%'
      case 3:
        return '300%'
      case 4:
        return '400%'
      case 5:
        return '500%'
      case 6:
        return '600%'
      default:
        return '100%'
    }
  }

  render () {
    const { currentScreen } = this.state
    return (
      <>
        <BackgroundContainer>
          <Background background={{ image: background, position: `-${currentScreen}0%` }} />
        </BackgroundContainer>
        <Page>
          <Panel>
            <div
              style={{
                maxWidth: 'inherit',
                minHeight: '540px',
                display: 'flex',
                justifyContent: 'center',
                alignItems: 'center'
              }}
            >
              <WelcomeBox index={1} currentScreen={currentScreen} onClick={this.onClickLetsGo} />
              <ImportBox index={2} currentScreen={currentScreen} onClick={this.onClickImport} />
              <SearchBox index={3} currentScreen={currentScreen} onClick={this.onClickConfirmDefaultSearchEngine} />
              <ThemeBox index={4} currentScreen={currentScreen} onClick={this.onClickChooseYourTheme} />
              <ShieldsBox index={5} currentScreen={currentScreen} />
              <RewardsBox index={6} currentScreen={currentScreen} onClick={this.onClickRewardsGetStarted} />
            </div>
            <FooterBox
              totalScreensSize={this.totalScreensSize}
              currentScreen={currentScreen}
              onClickSkip={this.onClickSkip}
              onClickSlideBullet={this.onClickSlideBullet}
              onClickNext={this.onClickNext}
              onClickDone={this.onClickDone}
            />
          </Panel>
        </Page>
      </>
    )
  }
}
