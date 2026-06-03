// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, Route, RouteProps } from 'react-router'

interface ProtectedRouteProps {
  requirement: boolean
  /** May include search/hash (e.g. restored session after unlock). */
  redirectRoute: string
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
      render={() =>
        requirement ? (
          (children as React.ReactNode)
        ) : (
          <Redirect to={redirectRoute} />
        )
      }
    />
  )
}
