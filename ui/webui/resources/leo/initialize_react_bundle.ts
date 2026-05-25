// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Each `@brave/leo/react/<name>` module is exported under a name produced by
// uppercasing the first letter and preserving the rest (e.g. navigationItem ->
// NavigationItem). The externals function in components/webpack/webpack.config.js
// uses the same transform, so consumer imports of `@brave/leo/react/<name>` are
// rewritten to `(leo-react.bundle.js).<Name>`.
export * as Alert from '@brave/leo/react/alert'
export * as AlertCenter from '@brave/leo/react/alertCenter'
export * as Button from '@brave/leo/react/button'
export * as ButtonMenu from '@brave/leo/react/buttonMenu'
export * as Checkbox from '@brave/leo/react/checkbox'
export * as Collapse from '@brave/leo/react/collapse'
export * as Dialog from '@brave/leo/react/dialog'
export * as Dropdown from '@brave/leo/react/dropdown'
export * as Floating from '@brave/leo/react/floating'
export * as Icon from '@brave/leo/react/icon'
export * as Input from '@brave/leo/react/input'
export * as Label from '@brave/leo/react/label'
export * as Navdots from '@brave/leo/react/navdots'
export * as Navigation from '@brave/leo/react/navigation'
export * as NavigationItem from '@brave/leo/react/navigationItem'
export * as NavigationMenu from '@brave/leo/react/navigationMenu'
export * as ProgressBar from '@brave/leo/react/progressBar'
export * as ProgressRing from '@brave/leo/react/progressRing'
export * as RadioButton from '@brave/leo/react/radioButton'
export * as SegmentedControl from '@brave/leo/react/segmentedControl'
export * as SegmentedControlItem from '@brave/leo/react/segmentedControlItem'
export * as TabItem from '@brave/leo/react/tabItem'
export * as Tabs from '@brave/leo/react/tabs'
export * as Textarea from '@brave/leo/react/textarea'
export * as Toggle from '@brave/leo/react/toggle'
export * as Tooltip from '@brave/leo/react/tooltip'
