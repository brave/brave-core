/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Link, Navigation, IconLink, PhotoName } from '../../../../src/features/newTab/default'

// Icons
import { SettingsIcon, FolderIcon, HistoryIcon } from '../../../../src/components/icons'

export default class FooterInfo extends React.PureComponent<{}, {}> {
  render () {
    return (
      <>
      <div>
        <PhotoName>Paris: The Roof</PhotoName>
        <Link
          href='https://www.photoserge.com/'
          rel='noopener'
          target='_blank'
        >
          Serge Ramelli
        </Link>
      </div>
        <Navigation>
          <IconLink><SettingsIcon /></IconLink>
          <IconLink><FolderIcon /></IconLink>
          <IconLink><HistoryIcon /></IconLink>
        </Navigation>
      </>
    )
  }
}
