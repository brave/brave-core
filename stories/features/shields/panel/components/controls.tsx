/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Grid, Column } from '../../../../../src/components/layout/gridList/index'
import SelectOption from '../../../../../src/old/selectOption/index'
import SwitchButton from '../../../../../src/old/switchButton/index'
import ContentToggleArrow from '../../../../../src/old/contentToggleArrow/index'
import BoxedContent from '../../../../../src/old/boxedContent/index'
import locale from '../fakeLocale'
import theme from '../theme'

class BraveShieldsControls extends React.PureComponent {
  render () {
    return (
      <Grid
        id='braveShieldsControls'
        theme={theme.braveShieldsControls}>
        <Column>
          <ContentToggleArrow
            withSeparator={true}
            defaultOpen={true}
            summary={locale.shieldsControlsAdvancedControls}
            onClick={() => console.log('fired onClick')}>
            <BoxedContent theme={theme.braveShieldsControlsContent}>
              <SelectOption
                disabled={false}
                titleName={locale.shieldsControlsAdControl}>
                {/* TODO needs "show brave ads" */}
                <option value='allow'>{locale.shieldsControlsAdControlOptionBlockAds}</option>
                <option value='block'>{locale.shieldsControlsAdControlOptionAllowAdsTracking}</option>
              </SelectOption>
              <SelectOption
                disabled={false}
                titleName={locale.shieldsControlsCookieControl}>
                <option value='SOME'>{locale.shieldsControlsCookieOptionBlock3p}</option>
                <option value='SOME'>{locale.shieldsControlsCookieOptionAllowAll}</option>
                <option value='SOME'>{locale.shieldsControlsCookieOptionBlockAll}</option>
              </SelectOption>

              <SelectOption
                disabled={false}
                titleName={locale.shieldsControlsFingerprintingProtection}>
                <option value='block_third_party'>{locale.shieldsControlsFingerprintingOptionBlock3p}</option>
                <option value='block'>{locale.shieldsControlsFingerprintingOptionBlockAll}</option>
                <option value='allow'>{locale.shieldsControlsFingerprintingOptionAllowAll}</option>
              </SelectOption>
              <Grid theme={theme.braveShieldsControlsSwitches}>
                <Column>
                  <SwitchButton
                    id='httpsEverywhere'
                    disabled={false}
                    rightText={locale.shieldsControlsHttpsEverywhereSwitch}
                    checked={true}
                    onChange={() => console.log('httpsE')} />
                </Column>
                <Column>
                  <SwitchButton
                    id='blockScripts'
                    disabled={false}
                    rightText={locale.shieldsControlsBlockScriptsSwitch}
                    checked={true}
                    onChange={() => console.log('block JS')} />
                </Column>
                <Column>
                  <SwitchButton
                    id='blockPhishingMalware'
                    checked={true}
                    disabled={false}
                    rightText={locale.shieldsControlsBlockPhishingMalwareSwitch}
                    onChange={() => console.log('block Pishing/Malware')} />
                </Column>
              </Grid>
            </BoxedContent>
          </ContentToggleArrow>
        </Column>
      </Grid>
    )
  }
}

export default BraveShieldsControls
