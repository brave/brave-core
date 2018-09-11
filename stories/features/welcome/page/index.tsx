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
const paymentsImage = require('../../../assets/img/payments.png')
const importImage = require('../../../assets/img/import.png')
const shieldsImage = require('../../../assets/img/shields.png')
const themeImage = require('../../../assets/img/theme.png')
const searchImage = require('../../../assets/img/search.png')
const background = require('../../../assets/img/welcomebg.svg')

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
      <Welcome.Content>
        <Welcome.BraveImage src={braveLogo} />
        <Welcome.Title>{locale.welcome}</Welcome.Title>
        <Welcome.Paragraph>{locale.whatIsBrave}</Welcome.Paragraph>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.letsGo}
        />
      </Welcome.Content>
    )
  }

  get secondScreen () {
    return (
      <Welcome.Content>
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
            type={this.state.fakeBookmarksImported ? 'default' : 'accent'}
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
      <Welcome.Content>
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
      <Welcome.Content>
        <Welcome.ThemeImage src={themeImage} />
        <Welcome.Title>{locale.chooseYourTheme}</Welcome.Title>
        <Welcome.Paragraph>{locale.findToolbarTheme}</Welcome.Paragraph>
        <Welcome.SelectGrid>
          <Select id='theme' type='light' value='default'>
            <div data-value='default'>Default Theme</div>
            <div data-value='dark'>Dark Theme</div>
            <div data-value='night'>Night Theme</div>
          </Select>
          <Button
            level='primary'
            type={this.state.fakeChangedDefaultTheme ? 'default' : 'accent'}
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
      <Welcome.Content>
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
      <Welcome.Content>
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
                  onClick={this.onClickNext}
                  text={locale.next}
                  icon={{ position: 'after', image: <ArrowRightIcon /> }}
                />
              )
              : this.state.currentScreen !== 1 && (
                <Button
                  level='secondary'
                  type='default'
                  onClick={this.onClickDone}
                  text={locale.done}
                />
            )
          }
        </Welcome.FooterRightColumn>
      </Welcome.Footer>
    )
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
    console.log('IMPORTED!')
  }

  onClickConfirmDefaultSearchEngine = () => {
    this.setState({ fakeChangedSearchEngine: !this.state.fakeChangedSearchEngine })
    console.log('CHANGED DEFAULT SEARCH ENGINE!')
  }

  onClickChooseYourTheme = () => {
    this.setState({ fakeChangedDefaultTheme: !this.state.fakeChangedDefaultTheme })
    console.log('NEW THEME CHOOSED')
  }

  get backgroundPosition () {
    switch (this.state.currentScreen) {
      case 1:
        return '0%'
      case 2:
        return '50%'
      case 3:
        return '100%'
      case 4:
        return '150%'
      case 5:
        return '200%'
      case 6:
        return '250%'
      default:
        return '0%'
    }
  }

  render () {
    return (
      <Welcome.Background
        background={{
          image: background,
          position: this.backgroundPosition
        }}
      >
          <Welcome.Panel>
            {this.currentScreen}
            {this.footer}
          </Welcome.Panel>
      </Welcome.Background>
    )
  }
}

export default WelcomePage
