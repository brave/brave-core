/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import shieldsLightTheme from '../theme/shields-light'
import shieldsDarkTheme from '../theme/shields-dark'
import { withThemesProvider } from 'storybook-addon-styled-component-theme'
// @ts-ignore
import { withKnobs } from '@storybook/addon-knobs/react'
import ReadOnlyView from '../containers/readOnlyView'
import SimpleView from '../containers/simpleView'
import AdvancedView from '../containers/advancedView'
import NoScript from '../containers/advancedView/overlays/noScriptOverlay'
import StaticList from '../containers/advancedView/overlays/staticOverlay'
import {
  getShieldsPanelTabData,
  getPersistentData,
  fakeBlockedResources,
  fakeNoScriptInfo
} from './data/storybookState'
import { ShieldsPanelActionTypes } from '../types/actions/shieldsPanelActions'

const doNothing = (event?: any): any => undefined

// Themes
const themes = [shieldsLightTheme, shieldsDarkTheme]

storiesOf('Shields', module)
.addDecorator(withThemesProvider(themes))
.addDecorator(withKnobs)
  .add('Read-only View', () => {
    return (
      <ReadOnlyView
        shieldsPanelTabData={getShieldsPanelTabData}
        toggleReadOnlyView={doNothing()}
      />
    )
  })
  .add('Simple View', () => {
    return (
      <SimpleView
        shieldsPanelTabData={getShieldsPanelTabData}
        toggleReadOnlyView={doNothing()}
        toggleAdvancedView={doNothing()}
        actions={{} as ShieldsPanelActionTypes}
        persistentData={getPersistentData}
      />
    )
  })
  .add('Advanced View', () => {
    return (
      <AdvancedView
        shieldsPanelTabData={getShieldsPanelTabData}
        toggleAdvancedView={doNothing()}
        actions={{} as ShieldsPanelActionTypes}
        persistentData={getPersistentData}
      />
    )
  })
  .add('Scripts blocked list (NoScript)', () => {
    return (
      <NoScript
        favicon=''
        hostname={getShieldsPanelTabData.hostname}
        noScriptInfo={fakeNoScriptInfo}
        onClose={doNothing}
        actions={{} as ShieldsPanelActionTypes}
      />
    )
  })
  .add('Static resources blocked list', () => {
    return (
      <StaticList
        favicon=''
        hostname={getShieldsPanelTabData.hostname}
        stats={99}
        name='Static List Example'
        list={fakeBlockedResources}
        onClose={doNothing}
      />
    )
  })
