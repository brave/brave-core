/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const React = require('react')
const { getLocale } = require('../../common/locale')
const rewardsImage = require('../../img/welcome/rewards.png')
const Image = require('brave-ui/v1/image').default
const PushButtonLink = require('brave-ui/v1/pushButton').PushButtonLink
const { Heading, Paragraph } = require('brave-ui')

class RewardsScreen extends React.PureComponent {
  render () {
    const { theme } = this.props
    return (
      <section style={theme.content}>
        <Image theme={theme.paymentsImage} src={rewardsImage} />
        <Heading level={1} theme={theme.title} text={getLocale('enableBraveRewards')} />
        <Paragraph theme={theme.text} text={getLocale('setupBraveRewards')} />
        <PushButtonLink
          color='primary'
          size='large'
          theme={theme.mainButton}
          href='chrome://rewards'
        >
          {getLocale('enableRewards')}
        </PushButtonLink>
      </section>
    )
  }
}

module.exports = RewardsScreen
