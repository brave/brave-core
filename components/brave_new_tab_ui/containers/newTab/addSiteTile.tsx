// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SortableElement, SortableElementProps } from 'react-sortable-hoc'

import { getLocale } from '../../../common/locale'
import { AddSiteTile, AddSiteTileImage, TileTitle } from '../../components/default'
// Icons
import AddSiteTileIcon from '../../components/default/gridSites/assets/add-site-tile'

interface Props {
  showEditTopSite: () => void
  isDragging: boolean
}

class AddSite extends React.PureComponent<Props, {}> {
  render () {
    return (
      <AddSiteTile
        onClick={this.props.showEditTopSite}
        isDragging={this.props.isDragging}
      >
        <AddSiteTileImage>
          <AddSiteTileIcon/>
        </AddSiteTileImage>
        <TileTitle>
          {getLocale('addTopSiteDialogTitle')}
        </TileTitle>
      </AddSiteTile>
    )
  }
}

type TopSiteSortableElementProps = SortableElementProps & Props
export default SortableElement(
  (props: TopSiteSortableElementProps) => <AddSite {...props} />
)
