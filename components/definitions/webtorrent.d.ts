/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 /// <reference path="../../node_modules/@types/webtorrent/index.d.ts" />

import { Server } from 'http'

interface CreateServerOpts {
  origin: string
  hostname: string
}

declare namespace WebTorrent {
  interface Torrent extends NodeJS.EventEmitter {
    createServer (opts?: CreateServerOpts): Server
  }
}

export = WebTorrent
