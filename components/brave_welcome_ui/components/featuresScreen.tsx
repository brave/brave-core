/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const React = require('react')
const { getLocale } = require('../../common/locale')
const featuresImage = require('../../img/welcome/features.png')
const Image = require('brave-ui/v1/image').default
const PushButtonLink = require('brave-ui/v1/pushButton').PushButtonLink
const { Heading, Paragraph } = require('brave-ui')

class FeaturesScreen extends React.PureComponent {
  render () {
    const { theme } = this.props
    return (
      <section style={theme.content}>
        <Image theme={theme.featuresImage} src={featuresImage} />
        <Heading level={1} theme={theme.title} text={getLocale('customizePreferences')} />
        <Paragraph theme={theme.text} text={getLocale('configure')} />
        <PushButtonLink
          color='primary'
          size='large'
          theme={theme.mainButton}
          href='chrome://settings'
        >
          {getLocale('preferences')}
        </PushButtonLink>
      </section>
    )
  }
}

module.exports = FeaturesScreen
