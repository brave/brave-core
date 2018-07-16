/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '../../common/locale'
import { PushButton, PushButtonLink } from 'brave-ui/v1/pushButton'
import ArrowRight from 'brave-ui/v1/icons/arrowRight'
import { Grid, Column, Anchor } from 'brave-ui'
import { theme } from '../theme'

interface Props {
  pageIndex: number,
  totalSecondaryScreensSize: number,
  onClickNext: (e: any) => void
}

export default class Footer extends React.PureComponent<Props, {}> {
  render () {
    const {
      pageIndex,
      totalSecondaryScreensSize,
      onClickNext,
      children
    } = this.props

    return (
      <footer>
        <Grid columns={3} theme={theme.footer}>
          <Column size={1} theme={theme.footerColumnLeft}>
            <Anchor
              theme={theme.skip}
              text={getLocale('skipWelcomeTour')}
              href='chrome://newtab'
            />
          </Column>
          <Column size={1} theme={theme.footerColumnCenter}>
            {children}
          </Column>
          <Column size={1} theme={theme.footerColumnRight}>
            {
              pageIndex < totalSecondaryScreensSize - 1
              ? (
                <PushButton
                  theme={theme.sideButton}
                  color='secondary'
                  onClick={onClickNext}
                >
                  <span
                    style={{
                      display: 'flex',
                      alignItems: 'center',
                      justifyContent: 'space-evenly'
                    }}
                  >
                    {getLocale('next')} <ArrowRight />
                  </span>
                </PushButton>
              )
              : (
                <PushButtonLink
                  theme={theme.sideButton}
                  color='secondary'
                  href='chrome://newtab'
                >
                  {getLocale('done')}
                </PushButtonLink>
              )
            }
          </Column>
        </Grid>
      </footer>
    )
  }
}
