/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { MainFooterLinkFlex, LinkIcon } from '../../../../src/features/shields'

// Fake data
import locale from '../fakeLocale'

export default class ShieldsFooter extends React.PureComponent<{}, {}> {
  render () {
    return (
      <MainFooterLinkFlex href='chrome://settings' rel='noreferrer noopener' target='_blank'>
        <span>{locale.editDefaults}</span>
        <LinkIcon />
      </MainFooterLinkFlex>
    )
  }
}
