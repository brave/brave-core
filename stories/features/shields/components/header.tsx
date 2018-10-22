/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Header,
  HeaderToggle,
  Description,
  SiteCard,
  EnabledText,
  DisabledText,
  Label,
  Highlight,
  UnHighlight,
  Toggle,
  ShieldIcon
} from '../../../../src/features/shields'

// Fake data
import locale from '../fakeLocale'
import data from '../fakeData'

interface Props {
  enabled: boolean
  sitename: string
  favicon: string
  fakeOnChange: () => void
}

export default class ShieldsHeader extends React.PureComponent<Props, {}> {
  render () {
    const { fakeOnChange, enabled, sitename, favicon } = this.props
    return (
      <Header id='braveShieldsHeader' enabled={enabled}>
        <HeaderToggle enabled={enabled}>
          <Label size='medium'>
            {locale.shields} <Highlight enabled={enabled}> {enabled ? locale.up : locale.down}
            </Highlight>
            <UnHighlight> {locale.forThisSite}</UnHighlight>
          </Label>
          <Toggle id='mainToggle' checked={enabled} onChange={fakeOnChange} size='large' />
        </HeaderToggle>
        {
          enabled
            ? <Description enabled={true}>{locale.enabledMessage}</Description>
            : null
        }
        <SiteCard>
          <EnabledText>
            <img src={favicon} />
            <Label size='large'>{sitename}</Label></EnabledText>
            {
              enabled
              ? (
                <EnabledText>
                  <Highlight enabled={true} size='large'>{data.totalBlocked}</Highlight>
                  <Label size='medium'>{locale.totalBlocked}</Label>
                </EnabledText>
              )
              : (
                <DisabledText>
                  <ShieldIcon />
                  <Description enabled={false}>{locale.disabledMessage}</Description>
                </DisabledText>
              )
            }
        </SiteCard>
      </Header>
    )
  }
}
