// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, act } from '@testing-library/react'
import AttachmentButtonMenu from '.'

jest.mock('$web-common/uploadFile', () => ({
  pickFiles: jest.fn(),
}))

// eslint-disable-next-line import/first
import { pickFiles } from '$web-common/uploadFile'
const mockPickFiles = pickFiles as jest.MockedFunction<typeof pickFiles>

const defaultProps = {
  attachFiles: jest.fn(),
  getScreenshots: jest.fn(),
  conversationHistory: [],
  associatedContentInfo: [],
  setAttachmentsDialog: jest.fn(),
  associateDefaultContent: undefined,
  unassociatedTabs: [],
  isMobile: false,
  conversationStarted: false,
}

// Mirrors the accept constant in the component — must match its preprocessor block exactly.
const expectedAccept: string | undefined =
  // <if expr="is_android">
  'image/*,application/pdf'
// <else>
undefined
// </if>

describe('attachment button menu', () => {
  beforeEach(() => {
    mockPickFiles.mockResolvedValue([])
  })

  it('upload menu item passes correct accept to pickFiles', async () => {
    const { container } = render(<AttachmentButtonMenu {...defaultProps} />)

    const menuItems = container.querySelectorAll<HTMLElement>('leo-menu-item')
    await act(async () => {
      menuItems[0].click()
    })

    expect(mockPickFiles).toHaveBeenCalledWith({
      multiple: true,
      accept: expectedAccept,
    })
  })

  it('camera menu item passes correct accept to pickFiles', async () => {
    const { container } = render(
      <AttachmentButtonMenu
        {...defaultProps}
        isMobile
      />,
    )

    // With isMobile=true and no associated content, menu order is:
    // [0] upload, [1] camera, [2] bookmarks, [3] history
    const menuItems = container.querySelectorAll<HTMLElement>('leo-menu-item')
    await act(async () => {
      menuItems[1].click()
    })

    expect(mockPickFiles).toHaveBeenCalledWith({
      capture: 'environment',
      accept: expectedAccept,
    })
  })
})
