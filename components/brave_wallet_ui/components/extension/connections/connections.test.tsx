// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from '@testing-library/react'

import { default as BraveCoreThemeProvider } from '../../../../common/BraveCoreThemeProvider'

// Components
import { DomainTextContainer, DomainText } from './connections.style'
import { CreateSiteOrigin } from '../../shared/create-site-origin'

const activeOrigin = {
  originSpec:
    'http://theofficialabsolutelongestdomainnameregisteredontheworldwideweb.'
    + 'international/',
  eTldPlusOne:
    'theofficialabsolutelongestdomainnameregisteredontheworldwideweb.'
    + 'international',
}

describe('Connections', () => {
  const renderComponent = () => {
    const { container } = render(
      <BraveCoreThemeProvider>
        <DomainTextContainer
          data-key='domain-text-container'
          width='100px'
          padding='0px 24px'
        >
          <DomainText
            data-key='domain-text-etldplusone'
            textSize='16px'
            isBold={true}
            textColor='primary'
          >
            {activeOrigin.eTldPlusOne}
          </DomainText>
          <DomainText
            textSize='14px'
            isBold={false}
            textColor='tertiary'
            data-key='domain-text-origin'
          >
            <CreateSiteOrigin
              originSpec={activeOrigin.originSpec}
              eTldPlusOne={activeOrigin.eTldPlusOne}
            />
          </DomainText>
        </DomainTextContainer>
      </BraveCoreThemeProvider>,
    )
    return { container }
  }
  it('should render', async () => {
    const { container } = renderComponent()

    // Test domain text container styles
    const domainContainer = container.querySelector(
      '[data-key="domain-text-container"]',
    )
    expect(domainContainer).toBeInTheDocument()
    expect(domainContainer).toBeVisible()
    expect(domainContainer).toHaveStyle({
      overflow: 'hidden',
      boxSizing: 'border-box',
    })

    // Test domain text etldplusone and styles
    const domainTextetldPlusOne = container.querySelector(
      '[data-key="domain-text-etldplusone"]',
    )
    expect(domainTextetldPlusOne).toBeInTheDocument()
    expect(domainTextetldPlusOne).toBeVisible()
    expect(domainTextetldPlusOne).toHaveStyle({
      wordBreak: 'break-all',
    })

    // Test etldplusone text
    expect(domainTextetldPlusOne).toHaveTextContent(activeOrigin.eTldPlusOne)

    // Test domain text origin and styles
    const domainTextOrigin = container.querySelector(
      '[data-key="domain-text-origin"]',
    )
    expect(domainTextOrigin).toBeInTheDocument()
    expect(domainTextOrigin).toBeVisible()
    expect(domainTextOrigin).toHaveStyle({
      wordBreak: 'break-all',
    })

    // Test origin text
    expect(domainTextOrigin).toHaveTextContent(activeOrigin.originSpec)
  })
})
