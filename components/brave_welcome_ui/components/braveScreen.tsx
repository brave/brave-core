/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const React = require('react')
const { getLocale } = require('../../common/locale')
const Image = require('brave-ui/v1/image').default
const PushButton = require('brave-ui/v1/pushButton').PushButton
const { Heading, Paragraph } = require('brave-ui')
const braveLogo = require('../../img/welcome/brave_logo.png')

class BraveScreen extends React.PureComponent {
  constructor (props) {
    super(props)
    this.onClickLetsGo = this.onClickLetsGo.bind(this)
  }

  onClickLetsGo () {
    this.props.onGoToFirstSlide()
  }

  render () {
    const { theme } = this.props
    return (
      <section style={theme.content}>
        <Image theme={theme.braveLogo} src={braveLogo} />
        <Heading level={1} theme={theme.title} text={getLocale('welcome')} />
        <Paragraph theme={theme.text} text={getLocale('whatIsBrave')} />
        <PushButton
          color='primary'
          size='large'
          theme={theme.mainButton}
          onClick={this.onClickLetsGo}
        >
          {getLocale('letsGo')}
        </PushButton>
      </section>
    )
  }
}

module.exports = BraveScreen
