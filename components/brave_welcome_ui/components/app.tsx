/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Feature-specific components
import { Page, Panel, SlideContent } from 'brave-ui/features/welcome'

// Component groups
import WelcomeBox from './screens/welcomeBox'
import ImportBox from './screens/importBox'
import RewardsBox from './screens/rewardsBox'
import SearchBox from './screens/searchBox'
import ShieldsBox from './screens/shieldsBox'
import ThemeBox from './screens/themeBox'
import FooterBox from './screens/footerBox'

// Images
import { Background, BackgroundContainer } from 'brave-ui/features/welcome/images'

// Utils
import * as welcomeActions from '../actions/welcome_actions'

// Assets
import '../../fonts/muli.css'
import '../../fonts/poppins.css'
import 'emptykit.css'

interface Props {
  welcomeData: Welcome.State
  actions: any
}

export interface State {
  currentScreen: number
}

const totalScreensSize = 6

export class WelcomePage extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      currentScreen: 1
    }
  }

  get currentScreen () {
    return this.state.currentScreen
  }

  onClickLetsGo = () => {
    this.setState({ currentScreen: this.state.currentScreen + 1 })
  }

  onClickImport = (sourceBrowserProfileIndex: number) => {
    this.props.actions.importBrowserProfileRequested(sourceBrowserProfileIndex)
    this.setState({ currentScreen: this.state.currentScreen + 1 })
  }

  onClickChooseYourTheme = () => {
    this.props.actions.goToTabRequested('chrome://settings/appearance', '_blank')
  }

  onClickRewardsGetStarted = () => {
    this.props.actions.goToTabRequested('chrome://rewards', '_blank')
  }

  onClickSlideBullet = (nextScreen: number) => {
    this.setState({ currentScreen: nextScreen })
  }

  onClickNext = () => {
    this.setState({ currentScreen: this.state.currentScreen + 1 })
  }

  onClickDone = () => {
    this.props.actions.goToTabRequested('chrome://newtab', '_self')
  }

  onClickSkip = () => {
    this.props.actions.goToTabRequested('chrome://newtab', '_self')
  }

  render () {
    const { welcomeData, actions } = this.props
    const { currentScreen } = this.state
    return (
      <>
        <BackgroundContainer>
          <Background position={`-${currentScreen}0%`} style={{ backfaceVisibility: 'hidden' }} />
        </BackgroundContainer>
        <Page id='welcomePage'>
          <Panel>
            <SlideContent>
              <WelcomeBox index={1} currentScreen={this.currentScreen} onClick={this.onClickLetsGo} />
              <ImportBox
                index={2}
                currentScreen={this.currentScreen}
                onClick={this.onClickImport}
                browserProfiles={welcomeData.browserProfiles}
              />
              <SearchBox
                index={3}
                currentScreen={this.currentScreen}
                onClick={this.onClickNext}
                changeDefaultSearchProvider={actions.changeDefaultSearchProvider}
                searchProviders={welcomeData.searchProviders}
              />
              <ThemeBox index={4} currentScreen={this.currentScreen} onClick={this.onClickChooseYourTheme} />
              <ShieldsBox index={5} currentScreen={this.currentScreen} />
              <RewardsBox index={6} currentScreen={this.currentScreen} onClick={this.onClickRewardsGetStarted} />
            </SlideContent>
            <FooterBox
              totalScreensSize={totalScreensSize}
              currentScreen={this.currentScreen}
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
