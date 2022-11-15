/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from 'brave-ui/helpers'
import {
  StyledExcludedText,
  StyledRestore
} from './style'

export interface Props {
  numExcludedSites: number
  onRestore?: () => void
  showText?: boolean
}

export default class RestoreSites extends React.PureComponent<Props, {}> {
  getRestoreText () {
    return `(${getLocale('clearExcludeList')})`
  }

  getExclusionText (numSites: number) {
    return `${getLocale('excludedSitesText')} ${numSites}`
  }

  render () {
    const { numExcludedSites, onRestore, showText } = this.props

    return (
      <StyledExcludedText>
        {
          showText
          ? this.getExclusionText(numExcludedSites)
          : null
        }
        <StyledRestore onClick={onRestore}>
          {this.getRestoreText()}
        </StyledRestore>
      </StyledExcludedText>
    )
  }
}
