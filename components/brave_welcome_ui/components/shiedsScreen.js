/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const React = require('react')
const { getLocale } = require('../../common/locale')
const shieldsImage = require('../../img/welcome/shields.png')
const Image = require('brave-ui/v1/image').default
const { Heading, Paragraph } = require('brave-ui')

class ShieldsScreen extends React.PureComponent {
  render () {
    const { theme } = this.props
    return (
      <section style={theme.content}>
        <Image theme={theme.shieldsImage} src={shieldsImage} />
        <Heading level={1} theme={theme.title} text={getLocale('manageShields')} />
        <Paragraph theme={theme.text} text={getLocale('adjustProtectionLevel')} />
      </section>
    )
  }
}

module.exports = ShieldsScreen
