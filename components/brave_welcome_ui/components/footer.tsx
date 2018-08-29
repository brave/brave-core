/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { PushButton, PushButtonLink } from 'brave-ui/old/v1/pushButton'
import ArrowRight from 'brave-ui/old/v1/icons/arrowRight'
import { Grid, Column } from 'brave-ui/components'
import { Anchor } from 'brave-ui/old'

// Constants
import { theme } from '../constants/theme'

// Utils
import { getLocale } from '../../common/locale'

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
        <Grid columns={3} customStyle={theme.footer}>
          <Column size={1} customStyle={theme.footerColumnLeft}>
            <Anchor
              customStyle={theme.skip}
              text={getLocale('skipWelcomeTour')}
              href='chrome://newtab'
            />
          </Column>
          <Column size={1} customStyle={theme.footerColumnCenter}>
            {children}
          </Column>
          <Column size={1} customStyle={theme.footerColumnRight}>
            {
              pageIndex < totalSecondaryScreensSize - 1
              ? (
                <PushButton
                  customStyle={theme.sideButton}
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
                  customStyle={theme.sideButton}
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
