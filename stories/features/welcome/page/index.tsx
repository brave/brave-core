/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Button from '../../../../src/components/buttonsIndicators/button'
import { ArrowRightIcon } from '../../../../src/components/icons'
import * as Welcome from '../../../../src/features/welcome/'

import locale from './fakeLocale'

// Images
const braveLogo = require('../../../assets/img/lion_logo.svg')
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
    this.state = { currentScreen: 1 }
  }

  get totalScreensSize () {
    return 5
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
        <Welcome.PaymentsImage src={paymentsImage} />
        <Welcome.Title>{locale.enableBraveRewards}</Welcome.Title>
        <Welcome.Paragraph>{locale.setupBraveRewards}</Welcome.Paragraph>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.enableRewards}
        />
      </Welcome.Content>
    )
  }

  get thirdScreen () {
    return (
      <Welcome.Content>
        <Welcome.ImportImage src={importImage} />
        <Welcome.Title>{locale.importFromAnotherBrowser}</Welcome.Title>
        <Welcome.Paragraph>{locale.setupImport}</Welcome.Paragraph>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.importNow}
        />
      </Welcome.Content>
    )
  }

  get fourthScreen () {
    return (
      <Welcome.Content>
        <Welcome.ShieldsImage src={shieldsImage} />
        <Welcome.Title>{locale.manageShields}</Welcome.Title>
        <Welcome.Paragraph>{locale.adjustProtectionLevel}</Welcome.Paragraph>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.shieldSettings}
        />
      </Welcome.Content>
    )
  }

  get fifthScreen () {
    return (
      <Welcome.Content>
        <Welcome.FeaturesImage src={featuresImage} />
        <Welcome.Title>{locale.customizePreferences}</Welcome.Title>
        <Welcome.Paragraph>{locale.configure}</Welcome.Paragraph>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.preferences}
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
            this.state.currentScreen !== this.totalScreensSize
              ? (
                <Button
                  level='secondary'
                  type='default'
                  onClick={this.onClickNext}
                  text={locale.next}
                  icon={{ position: 'after', image: <ArrowRightIcon /> }}
                />
              )
              : (
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
