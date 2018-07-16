/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const React = require('react')
const { getLocale } = require('../../common/locale')
const PushButton = require('brave-ui/v1/pushButton').PushButton
const PushButtonLink = require('brave-ui/v1/pushButton').PushButtonLink
const ArrowRight = require('brave-ui/v1/icons/arrowRight').default
const { Grid, Column, Anchor } = require('brave-ui')

class Footer extends React.PureComponent {
  render () {
    const {
      theme,
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
                    <span style={{
                      display: 'flex',
                      alignItems: 'center',
                      justifyContent: 'space-evenly'
                    }}>
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

module.exports = Footer
