/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Header,
  MainToggleFlex,
  Description,
  SiteInfoCard,
  DisabledTextGrid,
  Label,
  Highlight,
  UnHighlight,
  Toggle,
  ShieldIcon,
  MainSiteInfoGrid,
  ShieldIconFlex
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
        <MainToggleFlex enabled={enabled}>
          <Label size='medium'>
            {locale.shields} <Highlight enabled={enabled}> {enabled ? locale.up : locale.down}
            </Highlight>
            <UnHighlight> {locale.forThisSite}</UnHighlight>
          </Label>
          <Toggle id='mainToggle' checked={enabled} onChange={fakeOnChange} size='large' />
        </MainToggleFlex>
        {
          enabled
            ? <Description enabled={true}>{locale.enabledMessage}</Description>
            : null
        }
        <SiteInfoCard>
          <MainSiteInfoGrid>
            <img src={favicon} />
            <Label size='large'>{sitename}</Label>
          </MainSiteInfoGrid>
            {
              enabled
              ? (
                <MainSiteInfoGrid>
                  <Highlight enabled={true} size='large'>{data.totalBlocked}</Highlight>
                  <Label size='medium'>{locale.totalBlocked}</Label>
                </MainSiteInfoGrid>
              )
              : (
                <DisabledTextGrid>
                  <ShieldIconFlex>
                    <ShieldIcon />
                  </ShieldIconFlex>
                  <Description enabled={false}>{locale.disabledMessage}</Description>
                </DisabledTextGrid>
              )
            }
        </SiteInfoCard>
      </Header>
    )
  }
}
