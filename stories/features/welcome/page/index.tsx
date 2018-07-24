/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Page from '../../../../src/old/page/index'
import { Heading } from '../../../../src/old/headings/index'
import Paragraph from '../../../../src/old/paragraph/index'
import { Grid, Column } from '../../../../src/components/layout/gridList/index'
import Panel from '../../../../src/old/v1/panel/index'
import UnstyledButton from '../../../../src/old/unstyledButton/index'
import { PushButton } from '../../../../src/old/v1/pushButton/index'
import Image from '../../../../src/old/v1/image/index'
import ArrowRight from '../../../../src/old/v1/icons/arrowRight'

import theme from './theme'
import locale from './fakeLocale'

// Images
const braveLogo = require('../../../assets/img/brave_logo.png')
const paymentsImage = require('../../../assets/img/payments.png')
const importImage = require('../../../assets/img/import.png')
const shieldsImage = require('../../../assets/img/shields.png')
const featuresImage = require('../../../assets/img/features.png')
const background = require('../../../assets/img/welcomebg.svg')

// Fonts
import '../../../assets/fonts/muli.css'
import '../../../assets/fonts/poppins.css'

export interface WelcomePageState {
  currentScreen: number
}

class WelcomePage extends React.PureComponent<{}, WelcomePageState> {
  constructor (props: {}) {
    super(props)
    this.onClickNext = this.onClickNext.bind(this)
    this.onSkipWelcomeTour = this.onSkipWelcomeTour.bind(this)
    this.onClickDone = this.onClickDone.bind(this)
    this.state = { currentScreen: 1 }
  }

  get totalScreensSize () {
    return 5
  }

  get firstScreen () {
    return (
      <section style={theme.content}>
        <Image theme={theme.braveLogo} src={braveLogo} />
        <Heading level={1} theme={theme.title} text={locale.welcome} />
        <Paragraph theme={theme.text} text={locale.whatIsBrave} />
        <PushButton color='primary' size='large' theme={theme.mainButton}>{locale.letsGo}</PushButton>
      </section>
    )
  }

  get secondScreen () {
    return (
      <section style={theme.content}>
        <Image theme={theme.paymentsImage} src={paymentsImage} />
        <Heading level={1} theme={theme.title} text={locale.enableBraveRewards} />
        <Paragraph theme={theme.text} text={locale.setupBraveRewards} />
        <PushButton color='primary' size='large' theme={theme.mainButton}>{locale.enableRewards}</PushButton>
      </section>
    )
  }

  get thirdScreen () {
    return (
      <section style={theme.content}>
        <Image theme={theme.importImage} src={importImage} />
        <Heading level={1} theme={theme.title} text={locale.importFromAnotherBrowser} />
        <Paragraph theme={theme.text} text={locale.setupImport} />
        <PushButton color='primary' size='large' theme={theme.mainButton}>{locale.importNow}</PushButton>
      </section>
    )
  }

  get fourthScreen () {
    return (
      <section style={theme.content}>
        <Image theme={theme.shieldsImage} src={shieldsImage} />
        <Heading level={1} theme={theme.title} text={locale.manageShields} />
        <Paragraph theme={theme.text} text={locale.adjustProtectionLevel} />
        <PushButton color='primary' size='large' theme={theme.mainButton}>{locale.shieldSettings}</PushButton>
      </section>
    )
  }

  get fifthScreen () {
    return (
      <section style={theme.content}>
        <Image theme={theme.featuresImage} src={featuresImage} />
        <Heading level={1} theme={theme.title} text={locale.customizePreferences} />
        <Paragraph theme={theme.text} text={locale.configure} />
        <PushButton color='primary' size='large' theme={theme.mainButton}>{locale.preferences}</PushButton>
      </section>
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
      default:
        return this.firstScreen
    }
  }

  get footer () {
    return (
      <footer>
        <Grid columns={3} theme={theme.footer}>
          <Column size={1} theme={theme.footerColumnLeft}>
            <UnstyledButton
              theme={theme.skip}
              text={locale.skipWelcomeTour}
              onClick={this.onSkipWelcomeTour}
            />
          </Column>
          <Column size={1} theme={theme.footerColumnCenter}>
            {
              Array.from({length: this.totalScreensSize}, (v: undefined, k: number) => {
                return (
                  <UnstyledButton
                    theme={
                      this.state.currentScreen === k + 1
                      ? theme.bulletActive
                      : theme.bullet
                    }
                    text='•'
                    key={k}
                    onClick={this.onClickSlideBullet.bind(this, k + 1)}
                  />
                )
              })
            }
          </Column>
          <Column size={1} theme={theme.footerColumnRight}>
          {
            this.state.currentScreen !== this.totalScreensSize
             ? (
              <PushButton
                theme={theme.sideButton}
                color='secondary'
                onClick={this.onClickNext}>
                <span style={{
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'space-evenly'
                }}>
                  {locale.next} <ArrowRight />
                </span>
              </PushButton>
             )
             : (
              <PushButton
                theme={theme.sideButton}
                color='secondary'
                onClick={this.onClickDone}>
                {locale.done}
              </PushButton>
            )
          }
          </Column>
        </Grid>
      </footer>
    )
  }

  onClickSlideBullet (nextScreen: number) {
    this.setState({ currentScreen: nextScreen })
  }

  onClickNext () {
    this.setState({ currentScreen:this.state.currentScreen + 1 })
  }

  onClickDone () {
    // fades out
    // fades in to new tab page
  }

  onSkipWelcomeTour () {
    // fades out
    // fades in to new tab page
  }

  get backgroundPosition () {
    switch (this.state.currentScreen) {
      case 1:
        return '0%'
      case 2:
        return '100%'
      case 3:
        return '200%'
      case 4:
        return '300%'
      case 5:
        return '400%'
      default:
        return '0%'
    }
  }

  render () {
    return (
      <div style={{
        height: '-webkit-fill-available',
        width: '-webkit-fill-available',
        backgroundImage: `url('${background}')`,
        backgroundRepeat: 'repeat-x',
        backgroundSize: 'contain',
        transition: 'background-position-x 0.6s ease-out',
        backgroundPositionX: this.backgroundPosition
      }}>
        <Page theme={theme.welcomePage}>
          <Panel theme={theme.panel}>
            { this.currentScreen }
            { this.footer }
          </Panel>
        </Page>
      </div>
    )
  }
}

export default WelcomePage
