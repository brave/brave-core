/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Label,
  Resources,
  ResourcesHeader,
  ResourcesSiteInfo,
  ResourcesList,
  CloseButton,
  CloseIcon,
  ResourcesLabelGridStatic,
  ResourcesLabelGridDynamic,
  EmptyButton,
  Stat,
  ShowLessIcon,
  ResourcesLabelDynamic,
} from '../../../../../src/features/shields'

interface Props {
  sitename: string
  data: string | number
  title: string
  favicon: string
  onToggle: (event: React.MouseEvent<HTMLButtonElement>) => void
  dynamic?: boolean
}

const dynamicHeader = (
  title: string,
  onToggle: (event: React.MouseEvent<HTMLButtonElement>) => void
) => {
  return (
    <ResourcesLabelGridDynamic>
      <EmptyButton onClick={onToggle}><ShowLessIcon /></EmptyButton>
      <ResourcesLabelDynamic>{title}</ResourcesLabelDynamic>
    </ResourcesLabelGridDynamic>
  )
}

const staticHeader = (
  data: string | number,
  title: string,
  onToggle: (event: React.MouseEvent<HTMLButtonElement>) => void
) => {
  return (
    <ResourcesLabelGridStatic>
      <EmptyButton onClick={onToggle}><ShowLessIcon /></EmptyButton>
      <Stat>{data}</Stat>
      <ResourcesLabelDynamic>{title}</ResourcesLabelDynamic>
    </ResourcesLabelGridStatic>
  )
}

export default class BlockedResources extends React.PureComponent<Props, {}> {
  render () {
    const { sitename, data, title, favicon, onToggle, dynamic, children } = this.props
    return (
      <Resources>
        <ResourcesHeader>
          <ResourcesSiteInfo>
            <img src={favicon} />
            <Label size='large'>{sitename}</Label>
          </ResourcesSiteInfo>
          <CloseButton onClick={onToggle}><CloseIcon /></CloseButton>
        </ResourcesHeader>
        {dynamic ? dynamicHeader(title, onToggle) : staticHeader(data, title, onToggle)}
        <ResourcesList>{children}</ResourcesList>
      </Resources>
    )
  }
}
