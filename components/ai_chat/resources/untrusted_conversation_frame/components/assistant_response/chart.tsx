// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import styles from './chart.module.scss'

interface ChartProps {
  artifact: Mojom.ToolArtifact
}

export default function Chart({ artifact }: ChartProps) {
  if (artifact.type !== Mojom.CHART_ARTIFACT_TYPE) {
    return null
  }

  return (
    <iframe
      src={`chart_display.html?${encodeURIComponent(artifact.contentJson)}`}
      className={styles.chartIframe}
      sandbox='allow-scripts allow-same-origin'
    />
  )
}
