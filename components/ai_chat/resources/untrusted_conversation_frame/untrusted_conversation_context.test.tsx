// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from '@testing-library/react'
import MockContext from './mock_untrusted_conversation_context'

const TestComponent = ({ visualContentUsedPercentage, contentUsedPercentage }) => {
  return (
    <MockContext 
      visualContentUsedPercentage={visualContentUsedPercentage}
      contentUsedPercentage={contentUsedPercentage}
    >
      <div>
        <span data-testid="visual-percentage">
          {visualContentUsedPercentage ?? 'undefined'}
        </span>
        <span data-testid="content-percentage">
          {contentUsedPercentage ?? 'undefined'}
        </span>
      </div>
    </MockContext>
  )
}

describe('UntrustedConversationContext visualContentUsedPercentage', () => {
  test('should provide default visualContentUsedPercentage as undefined', () => {
    const { getByTestId } = render(<TestComponent />)

    expect(getByTestId('visual-percentage')).toHaveTextContent('undefined')
  })

  test('should provide visualContentUsedPercentage when set in API state', () => {
    const { getByTestId } = render(<TestComponent visualContentUsedPercentage={75} />)

    expect(getByTestId('visual-percentage')).toHaveTextContent('75')
  })

  test('should handle visualContentUsedPercentage of 0', () => {
    const { getByTestId } = render(<TestComponent visualContentUsedPercentage={0} />)

    expect(getByTestId('visual-percentage')).toHaveTextContent('0')
  })

  test('should handle visualContentUsedPercentage of 100', () => {
    const { getByTestId } = render(<TestComponent visualContentUsedPercentage={100} />)

    expect(getByTestId('visual-percentage')).toHaveTextContent('100')
  })

  test('should handle null visualContentUsedPercentage', () => {
    const { getByTestId } = render(<TestComponent visualContentUsedPercentage={null} />)

    expect(getByTestId('visual-percentage')).toHaveTextContent('undefined')
  })

  test('should distinguish between contentUsedPercentage and visualContentUsedPercentage', () => {
    const { getByTestId } = render(
      <TestComponent 
        contentUsedPercentage={80} 
        visualContentUsedPercentage={60} 
      />
    )

    expect(getByTestId('content-percentage')).toHaveTextContent('80')
    expect(getByTestId('visual-percentage')).toHaveTextContent('60')
  })
})