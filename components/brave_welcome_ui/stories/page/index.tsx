/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Page, Panel, SlideContent } from '../../components'

// Component groups
import WelcomeBox from './screens/welcomeBox'
import ImportBox from './screens/importBox'
import ShieldsBox from './screens/shieldsBox'
import SearchBox from './screens/searchBox'
import RewardsBox from './screens/rewardsBox'
import FooterBox from './screens/footerBox'

// Images
import { Background, BackgroundContainer } from '../../components/images'

export interface State {
  currentScreen: number
  shouldUpdateElementOverflow: boolean
  fakeChangedSearchEngine: boolean
  fakeBookmarksImported: boolean
  fakeChangedDefaultTheme: boolean
}

export interface Props {
  isDefaultSearchGoogle: boolean
}

export default class WelcomePage extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      currentScreen: 1,
      shouldUpdateElementOverflow: false,
      fakeChangedSearchEngine: false,
      fakeBookmarksImported: false,
      fakeChangedDefaultTheme: false
    }
  }

  get totalScreensSize () {
    return 5
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

  onChangeDefaultSearchEngine = () => {
    this.setState({ fakeChangedSearchEngine: !this.state.fakeChangedSearchEngine })
    console.log('CHANGED DEFAULT SEARCH ENGINE!')
  }

  onClickConfirmDefaultSearchEngine = () => {
    this.setState({ currentScreen: this.state.currentScreen + 1 })
    console.log('CONFIRMED DEFAULT SEARCH ENGINE!')
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

  resetStyleOverflow = () => {
    this.setState({ shouldUpdateElementOverflow: true })
  }

  render () {
    const { currentScreen, shouldUpdateElementOverflow } = this.state
    const { isDefaultSearchGoogle } = this.props
    return (
      <>
        <Page
          onAnimationEnd={this.resetStyleOverflow}
          shouldUpdateElementOverflow={shouldUpdateElementOverflow}
        >
          <Panel>
            <SlideContent>
              <WelcomeBox index={1} currentScreen={currentScreen} onClick={this.onClickLetsGo} />
              <ImportBox index={2} currentScreen={currentScreen} onClick={this.onClickImport} />
              <ShieldsBox index={3} currentScreen={currentScreen} />
              <SearchBox index={4} currentScreen={currentScreen} onClick={this.onClickConfirmDefaultSearchEngine} fakeOnChange={this.onChangeDefaultSearchEngine} isDefaultSearchGoogle={isDefaultSearchGoogle}/>
              <RewardsBox index={5} currentScreen={currentScreen} onClick={this.onClickRewardsGetStarted} />
            </SlideContent>
            <FooterBox
              totalScreensSize={this.totalScreensSize}
              currentScreen={currentScreen}
              onClickSkip={this.onClickSkip}
              onClickSlideBullet={this.onClickSlideBullet}
              onClickNext={this.onClickNext}
              onClickDone={this.onClickDone}
            />
          </Panel>
          <BackgroundContainer>
            <Background />
          </BackgroundContainer>
        </Page>
      </>
    )
  }
}
