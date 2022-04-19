// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { MemoryRouter as Router, Route } from 'react-router'

import { StepsNavigation } from './steps-navigation'

export const Nav = () => {
  return <Router>
    <Route path={'/:currentStep'}>
      {({ match }) => (
        <StepsNavigation
          steps={['1', '2', '3']}
          currentStep={match?.params.currentStep || '1'}
          goBack={() => alert('go back')}
        />
      )}
    </Route>
  </Router>
}

export default Nav
