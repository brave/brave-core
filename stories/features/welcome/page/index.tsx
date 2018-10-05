/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Button from '../../../../src/components/buttonsIndicators/button'
import Select from '../../../../src/components/formControls/select'
import { ArrowRightIcon } from '../../../../src/components/icons'
import * as Welcome from '../../../../src/features/welcome/'

import locale from './fakeLocale'

// Images
const braveLogo = require('../../../assets/img/lion_logo.svg')
const paymentsImage = require('../../../assets/img/welcome_rewards.svg')
const importImage = require('../../../assets/img/welcome_import.svg')
const shieldsImage = require('../../../assets/img/welcome_shields.svg')
const themeImage = require('../../../assets/img/welcome_theme.svg')
const searchImage = require('../../../assets/img/welcome_search.svg')
const background = require('../../../assets/img/welcome_bg.svg')

// Fonts
import '../../../assets/fonts/muli.css'
import '../../../assets/fonts/poppins.css'

export interface WelcomePageState {
  currentScreen: number
  fakeChangedSearchEngine: boolean
  fakeBookmarksImported: boolean
  fakeChangedDefaultTheme: boolean
}

class WelcomePage extends React.PureComponent<{}, WelcomePageState> {
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

  get firstScreen () {
    return (
      <Welcome.Content zIndex={1} active={this.state.currentScreen === 1}>
        <Welcome.BraveImage src={braveLogo} />
        <Welcome.Title>{locale.welcome}</Welcome.Title>
        <Welcome.Paragraph>{locale.whatIsBrave}</Welcome.Paragraph>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.letsGo}
          onClick={this.onClickLetsGo}
          icon={{ position: 'after', image: <ArrowRightIcon /> }}
        />
      </Welcome.Content>
    )
  }

  get secondScreen () {
    return (
      <Welcome.Content zIndex={2} active={this.state.currentScreen === 2}>
        <Welcome.ImportImage src={importImage} />
        <Welcome.Title>{locale.importFromAnotherBrowser}</Welcome.Title>
        <Welcome.Paragraph>{locale.setupImport}</Welcome.Paragraph>
        <Welcome.SelectGrid>
          <Select
            type='light'
            value='chrome'
          >
            <div data-value='firefox'>Firefox</div>
            <div data-value='safari'>Safari</div>
            <div data-value='chrome'>Chrome</div>
            <div data-value='html'>Bookmark HTML file</div>
          </Select>
          <Button
            level='primary'
            type={this.state.fakeBookmarksImported ? 'accent' : 'accent'}
            size='large'
            text={this.state.fakeBookmarksImported ? locale.imported : locale.import}
            onClick={this.onClickImport}
          />
        </Welcome.SelectGrid>
      </Welcome.Content>
    )
  }

  get thirdScreen () {
    return (
      <Welcome.Content zIndex={3} active={this.state.currentScreen === 3}>
        <Welcome.SearchImage src={searchImage} />
        <Welcome.Title>{locale.setDefaultSearchEngine}</Welcome.Title>
        <Welcome.Paragraph>{locale.chooseSearchEngine}</Welcome.Paragraph>
        <Welcome.SelectGrid>
          <Select id='searchEngine' type='light' value='duckduckgo'>
            <div data-value='firefox'>Firefox</div>
            <div data-value='google'>Google</div>
            <div data-value='duckduckgo'>DuckDuckGo</div>
            <div data-value='bing'>Bing</div>
            <div data-value='yahoo'>Yahoo!</div>
            <div data-value='ask'>Ask</div>
            <div data-value='aol'>AOL</div>
          </Select>
          <Button
            level='primary'
            type={this.state.fakeChangedSearchEngine ? 'default' : 'accent'}
            size='large'
            text={this.state.fakeChangedSearchEngine ? locale.confirmed : locale.confirm}
            onClick={this.onClickConfirmDefaultSearchEngine}
          />
        </Welcome.SelectGrid>
      </Welcome.Content>
    )
  }

  get fourthScreen () {
    return (
      <Welcome.Content zIndex={4} active={this.state.currentScreen === 4}>
        <Welcome.ThemeImage src={themeImage} />
        <Welcome.Title>{locale.chooseYourTheme}</Welcome.Title>
        <Welcome.Paragraph>{locale.findToolbarTheme}</Welcome.Paragraph>
        <Welcome.SelectGrid>
          <Select id='theme' type='light' value='default'>
            <div data-value='default'>Default Theme</div>
            <div data-value='dark'>Dark Theme</div>
          </Select>
          <Button
            level='primary'
            type={this.state.fakeChangedDefaultTheme ? 'accent' : 'accent'}
            size='large'
            text={this.state.fakeChangedDefaultTheme ? locale.confirmed : locale.confirm}
            onClick={this.onClickChooseYourTheme}
          />
        </Welcome.SelectGrid>
      </Welcome.Content>
    )
  }

  get fifthScreen () {
    return (
      <Welcome.Content zIndex={5} active={this.state.currentScreen === 5}>
        <Welcome.ShieldsImage src={shieldsImage} />
        <Welcome.Title>{locale.protectYourPrivacy}</Welcome.Title>
        <Welcome.Paragraph>{locale.adjustProtectionLevel}</Welcome.Paragraph>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.showMeWhere}
        />
      </Welcome.Content>
    )
  }

  get sixthScreen () {
    return (
      <Welcome.Content zIndex={6} active={this.state.currentScreen === 6}>
        <Welcome.PaymentsImage src={paymentsImage} />
        <Welcome.Title>{locale.enableBraveRewards}</Welcome.Title>
        <Welcome.Paragraph>{locale.setupBraveRewards}</Welcome.Paragraph>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.getStarted}
        />
      </Welcome.Content>
    )
  }

  get currentScreen () {
    switch (this.state.currentScreen) {
      case 1:
        return this.firstScreen
      case 2:
        return this.secondScreen
      case 3:
        return this.thirdScreen
      case 4:
        return this.fourthScreen
      case 5:
        return this.fifthScreen
      case 6:
        return this.sixthScreen
      default:
        return this.firstScreen
    }
  }

  get footer () {
    return (
      <Welcome.Footer>
        <Welcome.FooterLeftColumn>
          <Welcome.SkipButton onClick={this.onSkipWelcomeTour}>
            {locale.skipWelcomeTour}
          </Welcome.SkipButton>
        </Welcome.FooterLeftColumn>
        <Welcome.FooterMiddleColumn>
          {
            Array.from({ length: this.totalScreensSize }, (v: undefined, k: number) => {
              return (
                <Welcome.Bullet
                  active={this.state.currentScreen === k + 1}
                  key={k}
                  onClick={this.onClickSlideBullet.bind(this, k + 1)}
                >
                  &bull;
                </Welcome.Bullet>
              )
            })}
        </Welcome.FooterMiddleColumn>
        <Welcome.FooterRightColumn>
          {
            this.state.currentScreen !== this.totalScreensSize &&
            // don't show the next button in the first screen
            this.state.currentScreen !== 1
              ? (
                <Button
                  level='secondary'
                  type='default'
                  size='medium'
                  onClick={this.onClickNext}
                  text={locale.next}
                  icon={{ position: 'after', image: <ArrowRightIcon /> }}
                />
              )
              : this.state.currentScreen !== 1 && (
                <Button
                  level='secondary'
                  type='default'
                  size='medium'
                  onClick={this.onClickDone}
                  text={locale.done}
                />
            )
          }
        </Welcome.FooterRightColumn>
      </Welcome.Footer>
    )
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

  onSkipWelcomeTour = () => {
    // fades out
    // fades in to new tab page
  }

  onClickImport = () => {
    this.setState({ fakeBookmarksImported: !this.state.fakeBookmarksImported })
    // prob not the correct syntax for this behavior, but a reminder that clicking this button executes functionality and then auto proceed to next screen
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
    return (
      <>
        <Welcome.Background
          background={{
            image: background,
            position: this.backgroundPosition
          }}
        />
        <Welcome.Page>
          <Welcome.Panel>
            <div
              style={{
                maxWidth: 'inherit',
                minHeight: '540px',
                display: 'flex',
                justifyContent: 'center',
                alignItems: 'center'
              }}
            >
              {this.firstScreen}
              {this.secondScreen}
              {this.thirdScreen}
              {this.fourthScreen}
              {this.fifthScreen}
              {this.sixthScreen}
            </div>
            {this.footer}
          </Welcome.Panel>
        </Welcome.Page>
      </>
    )
  }
}

export default WelcomePage
