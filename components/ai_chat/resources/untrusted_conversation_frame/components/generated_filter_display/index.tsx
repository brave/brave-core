/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import CodeBlock from '../code_block'
import styles from './style.module.scss'

export interface GeneratedFilterData {
  filterType: 'css_selector' | 'scriptlet'
  domain: string
  code: string
  description: string
  targetElements: string[]
  confidence: 'high' | 'medium' | 'low'
  reasoning: string
}

interface GeneratedFilterDisplayProps {
  filter: GeneratedFilterData
}

function ConfidenceBadge(props: { level: 'high' | 'medium' | 'low' }) {
  const { level } = props
  const iconName = level === 'high' ? 'check-circle-filled' :
                   level === 'medium' ? 'warning-circle-filled' :
                   'warning-triangle-filled'

  return (
    <div className={`${styles.confidenceBadge} ${styles[`confidence-${level}`]}`}>
      <Icon name={iconName} />
      <span>{level.charAt(0).toUpperCase() + level.slice(1)} confidence</span>
    </div>
  )
}

export default function GeneratedFilterDisplay(props: GeneratedFilterDisplayProps) {
  const { filter } = props
  const isScriptlet = filter.filterType === 'scriptlet'
  const language = isScriptlet ? 'javascript' : 'css'

  return (
    <div className={styles.container}>
      <div className={styles.header}>
        <div className={styles.headerLeft}>
          <Icon name={isScriptlet ? 'code' : 'design-palette'} />
          <h3 className={styles.title}>
            {isScriptlet ? 'Custom Scriptlet' : 'Cosmetic Filter'}
          </h3>
        </div>
        <ConfidenceBadge level={filter.confidence} />
      </div>

      <div className={styles.description}>
        {filter.description}
      </div>

      <div className={styles.section}>
        <div className={styles.sectionHeader}>
          <Icon name='list' />
          <h4>Target Elements</h4>
        </div>
        <div className={styles.targetElements}>
          {filter.targetElements.map((element, index) => (
            <code key={index} className={styles.targetElement}>{element}</code>
          ))}
        </div>
      </div>

      <div className={styles.section}>
        <div className={styles.sectionHeader}>
          <Icon name='code' />
          <h4>Generated Code</h4>
        </div>
        <CodeBlock.Block code={filter.code} lang={language} />
      </div>

      <div className={styles.section}>
        <div className={styles.sectionHeader}>
          <Icon name='info-outline' />
          <h4>Reasoning</h4>
        </div>
        <div className={styles.reasoning}>
          {filter.reasoning}
        </div>
      </div>

      {isScriptlet && (
        <div className={styles.debugNote}>
          <Icon name='settings' />
          <span>Debug logs will appear in DevTools Console</span>
        </div>
      )}

      <div className={styles.domainInfo}>
        <Icon name='web' />
        <span>
          This filter will apply to: <strong>{filter.domain}</strong>
        </span>
      </div>
    </div>
  )
}
