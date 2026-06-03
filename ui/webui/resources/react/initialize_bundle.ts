// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// ESM facade for the combined React + ReactDOM bundle. The upstream packages
// are CommonJS, so using them directly as webpack entries with
// `output.library.type = 'module'` collapses each onto the bundle's `default`
// export and breaks named imports from consumers. Bundling them together has
// two further wins: react-dom's internal `require('react')` resolves directly
// to React's CJS exports (no ESM boundary in the way), and consumers fetch a
// single file.
//
// Externals in components/webpack/webpack.config.js map `react` and
// `react-dom` to the `React` / `ReactDOM` namespace exports below.
import * as React from 'react'
import * as ReactDOM from 'react-dom'

export { React, ReactDOM }
