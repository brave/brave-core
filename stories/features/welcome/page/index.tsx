/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Button from '../../../../src/components/buttonsIndicators/button'

import { Title, Text } from '../../../../src/features/welcome/text'
import * as Image from '../../../../src/features/welcome/image'
import { SkipButton, Bullet } from '../../../../src/features/welcome/button'
import { WelcomePanel, WaveBackground } from '../../../../src/features/welcome/page'

import {
  Content,
  Footer,
  FooterLeftColumn,
  FooterMiddleColumn,
  FooterRightColumn
} from '../../../../src/features/welcome/wrappers'

import { ArrowRightIcon } from '../../../../src/components/icons'

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
      <Content>
        <Image.Brave src={braveLogo} />
        <Title>{locale.welcome}</Title>
        <Text>{locale.whatIsBrave}</Text>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.letsGo}
        />
      </Content>
    )
  }

  get secondScreen () {
    return (
      <Content>
        <Image.Payments src={paymentsImage} />
        <Title>{locale.enableBraveRewards}</Title>
        <Text>{locale.setupBraveRewards}</Text>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.enableRewards}
        />
      </Content>
    )
  }

  get thirdScreen () {
    return (
      <Content>
        <Image.Import src={importImage} />
        <Title>{locale.importFromAnotherBrowser}</Title>
        <Text>{locale.setupImport}</Text>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.importNow}
        />
      </Content>
    )
  }

  get fourthScreen () {
    return (
      <Content>
        <Image.Shields src={shieldsImage} />
        <Title>{locale.manageShields}</Title>
        <Text>{locale.adjustProtectionLevel}</Text>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.shieldSettings}
        />
      </Content>
    )
  }

  get fifthScreen () {
    return (
      <Content>
        <Image.Features src={featuresImage} />
        <Title>{locale.customizePreferences}</Title>
        <Text>{locale.configure}</Text>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.preferences}
        />
      </Content>
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
      <Footer>
        <FooterLeftColumn>
          <SkipButton onClick={this.onSkipWelcomeTour}>
            {locale.skipWelcomeTour}
          </SkipButton>
        </FooterLeftColumn>
        <FooterMiddleColumn>
          {
            Array.from({ length: this.totalScreensSize }, (v: undefined, k: number) => {
              return (
                <Bullet
                  active={this.state.currentScreen === k + 1}
                  key={k}
                  onClick={this.onClickSlideBullet.bind(this, k + 1)}
                >
                  &bull;
                </Bullet>
              )
            })}
        </FooterMiddleColumn>
        <FooterRightColumn>
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
        </FooterRightColumn>
      </Footer>
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
      <WaveBackground
        background={{
          image: background,
          position: this.backgroundPosition
        }}
      >
          <WelcomePanel>
            {this.currentScreen}
            {this.footer}
          </WelcomePanel>
      </WaveBackground>
    )
  }
}

export default WelcomePage
