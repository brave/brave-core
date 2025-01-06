/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { NonInteractiveURL } from './basic'

interface ShortenedUrlProps {
    url: string
    maxLength: number
    interactive: boolean
}

interface ShortenedUrlState {
    shortenedUrl: string
}

class ShortenedUrl
    extends React.PureComponent<ShortenedUrlProps, ShortenedUrlState> {
    static defaultProps: Partial<ShortenedUrlProps> = {
        maxLength: 100
    }

    constructor(props: ShortenedUrlProps) {
        super(props)
        this.state = {
            shortenedUrl: this.shortenUrl(props.url, props.maxLength),
        }
    }

    shortenUrl(url: string, maxLength: number): string {
        if (url.length <= maxLength) {
            return url
        }
        const dots = '......'
        const partLength = Math.floor((maxLength - dots.length) / 2)
        const start = url.slice(0, partLength)
        const end = url.slice(-partLength)

        return `${start}${dots}${end}`
    }

    componentDidUpdate(prevProps: ShortenedUrlProps) {
        if (prevProps.url !== this.props.url ||
            prevProps.maxLength !== this.props.maxLength) {
            this.setState({
                shortenedUrl: this.shortenUrl(
                    this.props.url, this.props.maxLength),
            })
        }
    }

    render() {
        const { shortenedUrl } = this.state
        return (
            <NonInteractiveURL>
                {shortenedUrl}
            </NonInteractiveURL>
        )
    }
}

export default ShortenedUrl;
