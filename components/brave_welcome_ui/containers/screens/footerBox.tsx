/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Footer,
  FooterLeftColumn,
  FooterMiddleColumn,
  FooterRightColumn,
  SkipButton,
  FooterButton,
  Bullet
} from '../../components'

// Shared components
import { ArrowRightIcon } from 'brave-ui/components/icons'

// Utils
import { getLocale } from '../../../common/locale'

interface Props {
  currentScreen: number
  totalScreensSize: number
  onClickSkip: () => void
  onClickNext: () => void
  onClickSlideBullet: (args: any) => any
  onClickDone: () => void
}

export default class FooterBox extends React.PureComponent<Props, {}> {
  render () {
    const {
      currentScreen,
      totalScreensSize,
      onClickSkip,
      onClickNext,
      onClickSlideBullet,
      onClickDone
    } = this.props
    return (
      <Footer>
        <FooterLeftColumn>
          <SkipButton onClick={onClickSkip}>{getLocale('skipWelcomeTour')}</SkipButton>
        </FooterLeftColumn>
        <FooterMiddleColumn>
          {Array.from({ length: totalScreensSize }, (v: undefined, k: number) => (
            <Bullet
              active={currentScreen === k + 1}
              key={k}
              onClick={onClickSlideBullet.bind(this, k + 1)}
            >
              <svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'>
                <circle cx='8' cy='8' r='8' fill='currentColor' fillRule='evenodd' />
              </svg>
            </Bullet>
          ))}
        </FooterMiddleColumn>
        <FooterRightColumn>
          {
            currentScreen !== totalScreensSize &&
            // don't show the next button in the first screen
            currentScreen !== 1
              ? (
                <FooterButton
                  level='secondary'
                  type='default'
                  size='medium'
                  onClick={onClickNext}
                  text={getLocale('next')}
                  icon={{ position: 'after', image: <ArrowRightIcon /> }}
                />
              )
              : currentScreen !== 1 && (
                <FooterButton
                  level='secondary'
                  type='default'
                  size='medium'
                  onClick={onClickDone}
                  text={getLocale('done')}
                />
            )
          }
        </FooterRightColumn>
      </Footer>
    )
  }
}
