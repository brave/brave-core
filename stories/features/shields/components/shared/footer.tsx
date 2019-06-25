/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { MainFooter, Link } from '../../../../../src/features/shields'

// Fake data
import { getLocale } from '../../fakeLocale'

interface Props {
  isBlockedListOpen: boolean
  advancedView: boolean
  fakeOnChangeAdvancedView: () => void
}

export default class Footer extends React.PureComponent<Props, {}> {
  render () {
    const { isBlockedListOpen, advancedView, fakeOnChangeAdvancedView } = this.props
    return (
      <MainFooter>
        <Link disabled={isBlockedListOpen} onClick={fakeOnChangeAdvancedView}>
          {advancedView ? getLocale('simpleView') : getLocale('advancedView')}
        </Link>
        <Link
          disabled={isBlockedListOpen}
        >
          {getLocale('changeDefaults')}
        </Link>
      </MainFooter>
    )
  }
}
