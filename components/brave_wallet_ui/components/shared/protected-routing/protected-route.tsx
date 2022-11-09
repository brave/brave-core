// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, Route, RouteProps } from 'react-router'

import { WalletRoutes } from '../../../constants/types'

interface ProtectedRouteProps {
  requirement: boolean
  redirectRoute: WalletRoutes
}

export const ProtectedRoute = ({
  children,
  redirectRoute,
  requirement,
  ...routeProps
}: ProtectedRouteProps & RouteProps) => {
  return (
    <Route
      {...routeProps}
      render={({ location }) =>
        requirement ? (
          children
        ) : (
          <Redirect
            to={{
              pathname: redirectRoute,
              state: { from: location }
            }}
          />
        )
      }
    />
  )
}

export default ProtectedRoute
