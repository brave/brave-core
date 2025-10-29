// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, fireEvent, waitFor, act } from '@testing-library/react'
import InputBox, { InputBoxProps } from '.'
import { ContentType, UploadedFileType } from '../../../common/mojom'
import { defaultContext } from '../../state/conversation_context'

// Mock the convertFileToUploadedFile function
jest.mock('../../utils/file_utils', () => ({
  convertFileToUploadedFile: jest.fn(),
}))

import { convertFileToUploadedFile } from '../../utils/file_utils'
const mockConvertFileToUploadedFile =
  convertFileToUploadedFile as jest.MockedFunction<
    typeof convertFileToUploadedFile
  >

// Mock URL.createObjectURL for tests that include image files
// This is needed because AttachmentUploadItems calls URL.createObjectURL to create blob URLs for images
Object.defineProperty(URL, 'createObjectURL', {
  writable: true,
  value: jest.fn(() => 'mock-object-url'),
})

const testContext: InputBoxProps['context'] = {
  ...defaultContext,
  // Override specific properties for testing
  isMobile: false,
  hasAcceptedAgreement: true,
  getPluralString: () => Promise.resolve(''),
  tabs: [],
  attachImages: jest.fn(),
  isAIChatAgentProfileFeatureEnabled: false,
  isAIChatAgentProfile: false,
  openAIChatAgentProfile: () => {},
}

describe('input box', () => {
  it('associated content is rendered in input box when not associated with a turn', () => {
    const { container } = render(
      <InputBox
        context={{
          ...testContext,
          associatedContentInfo: [
            {
              contentId: 1,
              contentType: ContentType.PageContent,
              contentUsedPercentage: 0.5,
              title: 'Associated Content',
              url: { url: 'https://example.com' },
              uuid: '1234',
              conversationTurnUuid: undefined,
            },
          ],
        }}
        conversationStarted={false}
      />,
    )

    expect(
      screen.getByText('Associated Content', { selector: '.title' }),
    ).toBeInTheDocument()
    expect(
      container.querySelector('img[src*="//favicon2"]'),
    ).toBeInTheDocument()
  })

  it('associated content is not rendered in input box when there is no associated content', () => {
    const { container } = render(
      <InputBox
        context={{
          ...testContext,
          associatedContentInfo: [],
        }}
        conversationStarted={false}
      />,
    )

    expect(screen.queryByText('Associated Content')).not.toBeInTheDocument()
    expect(
      container.querySelector('img[src*="//favicon2"]'),
    ).not.toBeInTheDocument()
  })

  it('associated content is not rendered in input box after being associated with a turn', () => {
    const { container } = render(
      <InputBox
        context={{
          ...testContext,
          associatedContentInfo: [
            {
              contentId: 1,
              contentType: ContentType.PageContent,
              contentUsedPercentage: 0.5,
              title: 'Associated Content',
              url: { url: 'https://example.com' },
              uuid: '1234',
              conversationTurnUuid: 'turn-1',
            },
          ],
        }}
        conversationStarted
      />,
    )

    expect(screen.queryByText('Associated Content')).not.toBeInTheDocument()
    expect(
      container.querySelector('img[src*="//favicon2"]'),
    ).not.toBeInTheDocument()
  })

  it('send button is disabled when the input text is empty', () => {
    const { container } = render(
      <InputBox
        context={{
          ...testContext,
          inputText: [''],
        }}
        conversationStarted={false}
      />,
    )

    const sendButton = container.querySelector('.sendButtonDisabled')
    expect(sendButton).toBeInTheDocument()
    expect(sendButton).toHaveClass('sendButtonDisabled')
  })

  it('send button is enabled when the input text is not empty', () => {
    const { container } = render(
      <InputBox
        context={{
          ...testContext,
          inputText: ['test'],
        }}
        conversationStarted={false}
      />,
    )

    const sendButton = container.querySelector('.button')
    expect(sendButton).toBeInTheDocument()
    expect(sendButton).not.toHaveClass('sendButtonDisabled')
  })

  it('streaming button is shown while generating', () => {
    const { container } = render(
      <InputBox
        context={{ ...testContext, isGenerating: true }}
        conversationStarted={false}
      />,
    )

    const streamingButton = container.querySelector('.streamingButton')
    expect(streamingButton).toBeInTheDocument()
    expect(streamingButton).toHaveClass('streamingButton')
  })

  type ContentAgentParams = {
    isAgentProfileFeatureEnabled: boolean
    isAgentProfile: boolean
    isConversationStarted: boolean
  }

  const contentAgentParams: ContentAgentParams[] = [
    {
      isAgentProfileFeatureEnabled: false,
      isAgentProfile: false,
      isConversationStarted: false,
    },
    {
      isAgentProfileFeatureEnabled: false,
      isAgentProfile: false,
      isConversationStarted: true,
    },
    {
      isAgentProfileFeatureEnabled: false,
      isAgentProfile: true,
      isConversationStarted: false,
    },
    {
      isAgentProfileFeatureEnabled: false,
      isAgentProfile: true,
      isConversationStarted: true,
    },
    {
      isAgentProfileFeatureEnabled: true,
      isAgentProfile: false,
      isConversationStarted: false,
    },
    {
      isAgentProfileFeatureEnabled: true,
      isAgentProfile: false,
      isConversationStarted: true,
    },
    {
      isAgentProfileFeatureEnabled: true,
      isAgentProfile: true,
      isConversationStarted: false,
    },
    {
      isAgentProfileFeatureEnabled: true,
      isAgentProfile: true,
      isConversationStarted: true,
    },
  ]

  it.each(contentAgentParams)(
    'Content Agent button is shown only if the feature is enabled',
    (params: ContentAgentParams) => {
      render(
        <InputBox
          context={{
            ...testContext,
            isAIChatAgentProfileFeatureEnabled:
              params.isAgentProfileFeatureEnabled,
            isAIChatAgentProfile: params.isAgentProfile,
          }}
          conversationStarted={params.isConversationStarted}
        />,
      )

      const contentAgentLaunchButton = screen.queryByTitle(
        'Open Leo AI Content Agent Window',
      )
      const contentAgentTooltip = screen.queryByTestId('agent-profile-tooltip')

      if (params.isAgentProfileFeatureEnabled && params.isAgentProfile) {
        expect(contentAgentLaunchButton).not.toBeInTheDocument()
        expect(contentAgentTooltip).toBeInTheDocument()
      } else if (
        params.isAgentProfileFeatureEnabled
        && !params.isAgentProfile
      ) {
        expect(contentAgentLaunchButton).toBeInTheDocument()
        expect(contentAgentTooltip).not.toBeInTheDocument()
      } else {
        expect(contentAgentLaunchButton).not.toBeInTheDocument()
        expect(contentAgentTooltip).not.toBeInTheDocument()
      }
    },
  )

  it('documents show up in attachment wrapper', () => {
    const { container } = render(
      <InputBox
        context={{
          ...testContext,
          pendingMessageFiles: [
            {
              filename: 'test.pdf',
              data: new ArrayBuffer(0),
              type: UploadedFileType.kPdf,
              filesize: BigInt(1024),
            },
          ],
        }}
        conversationStarted={false}
      />,
    )

    expect(screen.getByText('test.pdf')).toBeInTheDocument()
    const attachmentWrapper = container.querySelector('.attachmentWrapper')
    expect(attachmentWrapper).toBeInTheDocument()
  })

  it('attachments are shown if only documents are available', () => {
    const { container } = render(
      <InputBox
        context={{
          ...testContext,
          pendingMessageFiles: [
            {
              filename: 'document1.pdf',
              data: new ArrayBuffer(0),
              type: UploadedFileType.kPdf,
              filesize: BigInt(2048),
            },
            {
              filename: 'document2.pdf',
              data: new ArrayBuffer(0),
              type: UploadedFileType.kPdf,
              filesize: BigInt(1536),
            },
          ],
        }}
        conversationStarted={false}
      />,
    )

    const attachmentWrapper = container.querySelector('.attachmentWrapper')
    expect(attachmentWrapper).toBeInTheDocument()
    expect(screen.getByText('document1.pdf')).toBeInTheDocument()
    expect(screen.getByText('document2.pdf')).toBeInTheDocument()
    // Check that both documents are rendered as attachment items
    const attachmentItems = container.querySelectorAll('.itemWrapper')
    expect(attachmentItems.length).toBeGreaterThanOrEqual(2)
  })

  it('combinations of associated content, images and documents show up', () => {
    const { container } = render(
      <InputBox
        context={{
          ...testContext,
          associatedContentInfo: [
            {
              contentId: 1,
              contentType: ContentType.PageContent,
              contentUsedPercentage: 0.5,
              title: 'Page Content',
              url: { url: 'https://example.com' },
              uuid: '1234',
            },
          ],
          pendingMessageFiles: [
            {
              filename: 'image.jpg',
              data: new ArrayBuffer(0),
              type: UploadedFileType.kImage,
              filesize: BigInt(1024),
            },
            {
              filename: 'document.pdf',
              data: new ArrayBuffer(0),
              type: UploadedFileType.kPdf,
              filesize: BigInt(2048),
            },
          ],
        }}
        conversationStarted={false}
      />,
    )

    const attachmentWrapper = container.querySelector('.attachmentWrapper')
    expect(attachmentWrapper).toBeInTheDocument()

    // Associated content
    expect(screen.getByText('Page Content')).toBeInTheDocument()
    expect(
      container.querySelector('img[src*="//favicon2"]'),
    ).toBeInTheDocument()

    // Image file
    expect(screen.getByText('image.jpg')).toBeInTheDocument()
    expect(container.querySelector('img.image')).toBeInTheDocument()

    // Document file
    expect(screen.getByText('document.pdf')).toBeInTheDocument()

    // Check that we have multiple attachment items (page content + files)
    const attachmentItems = container.querySelectorAll('.itemWrapper')
    expect(attachmentItems.length).toBeGreaterThanOrEqual(3)
  })

  describe('paste event handling', () => {
    const createMockFile = (name: string, type: string, size = 1024) => {
      const file = new File(['mock content'], name, { type })
      Object.defineProperty(file, 'size', { value: size })
      return file
    }

    beforeEach(() => {
      jest.clearAllMocks()

      mockConvertFileToUploadedFile.mockImplementation((file: File) => {
        return Promise.resolve({
          filename: file.name,
          filesize: file.size,
          data: Array.from(new Uint8Array(8)), // Mock data array
          type: UploadedFileType.kImage,
        })
      })
    })

    it('filters image files and calls attachImages on paste', async () => {
      const mockAttachImages = jest.fn()
      const { container } = render(
        <InputBox
          context={{
            ...testContext,
            attachImages: mockAttachImages,
          }}
          conversationStarted={false}
        />,
      )

      const textarea = container.querySelector('textarea')!
      const imageFile = createMockFile('test.png', 'image/png')
      const textFile = createMockFile('test.txt', 'text/plain')

      await act(async () => {
        fireEvent.paste(textarea, {
          clipboardData: {
            files: [imageFile, textFile],
            items: [imageFile, textFile].map((file) => ({
              kind: 'file' as const,
              type: file.type,
              getAsFile: () => file,
            })),
            types: ['Files'],
            getData: jest.fn(),
            setData: jest.fn(),
          },
          preventDefault: jest.fn(),
        })
      })

      await waitFor(() => {
        expect(mockAttachImages).toHaveBeenCalledWith([
          expect.objectContaining({
            filename: 'test.png',
            filesize: 1024,
            data: expect.any(Array),
            type: UploadedFileType.kImage,
          }),
        ])
      })
    })
  })

  it(
    'Content Agent warning is shown if the conversation has not started'
      + ' and isAIChatAgentProfile is true',
    () => {
      const { container } = render(
        <InputBox
          context={{
            ...testContext,
            isAIChatAgentProfileFeatureEnabled: true,
            isAIChatAgentProfile: true,
          }}
          conversationStarted={false}
        />,
      )
      expect(
        container.querySelector('.contentAgentWarning'),
      ).toBeInTheDocument()
    },
  )

  it(
    'Content Agent warning is not shown after the conversation has started'
      + ' and isAIChatAgentProfile is true',
    () => {
      const { container } = render(
        <InputBox
          context={{
            ...testContext,
            isAIChatAgentProfileFeatureEnabled: true,
            isAIChatAgentProfile: true,
          }}
          conversationStarted={true}
        />,
      )
      expect(
        container.querySelector('.contentAgentWarning'),
      ).not.toBeInTheDocument()
    },
  )

  it(
    'Content Agent warning is not shown if isAIChatAgentProfile'
      + 'is not true',
    () => {
      const { container } = render(
        <InputBox
          context={{
            ...testContext,
          }}
          conversationStarted={true}
        />,
      )
      expect(
        container.querySelector('.contentAgentWarning'),
      ).not.toBeInTheDocument()
    },
  )
})
