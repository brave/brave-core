/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const React = require('react')
const { getLocale } = require('../../common/locale')
const importImage = require('../../img/welcome/import.png')
const Image = require('brave-ui/v1/image').default
const PushButton = require('brave-ui/v1/pushButton').PushButton
const { Heading, Paragraph } = require('brave-ui')

class ImportScreen extends React.PureComponent {
  constructor (props) {
    super(props)
    this.onClickImportNow = this.onClickImportNow.bind(this)
  }

  onClickImportNow () {
    this.props.onImportNowClicked()
  }

  render () {
    const { theme } = this.props
    return (
      <section style={theme.content}>
        <Image theme={theme.importImage} src={importImage} />
        <Heading level={1} theme={theme.title} text={getLocale('importFromAnotherBrowser')} />
        <Paragraph theme={theme.text} text={getLocale('setupImport')} />
        <PushButton
          color='primary'
          size='large'
          theme={theme.mainButton}
          onClick={this.onClickImportNow}
        >
          {getLocale('importNow')}
        </PushButton>
      </section>
    )
  }
}

module.exports = ImportScreen
