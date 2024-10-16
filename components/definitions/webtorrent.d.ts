/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import { Server } from 'http'
 import { Instance as ParseTorrent } from "parse-torrent"

interface CreateServerOpts {
  origin: string
  hostname: string
}

interface Config {
  tracker?:
    | boolean
    | { wrtc: boolean }
    | undefined;
}

declare module 'webtorrent' {
  interface TorrentFile extends NodeJS.EventEmitter {
    readonly name: string;
    readonly path: string;
    readonly length: number;

    getBlob(callback: (err: string | Error | undefined, blob?: Blob) => void): void;
  }

  interface Torrent extends NodeJS.EventEmitter {
      readonly infoHash: string
      readonly name: string;
      readonly files: TorrentFile[];

      createServer (opts?: CreateServerOpts): Server

      destroy(opts?: { destroyStore?: boolean | undefined }, cb?: (err: Error | string) => void): void;

      on(event: "infoHash" | "metadata" | "ready" | "done", callback: () => void): this;

      on(event: "warning" | "error", callback: (err: Error | string) => void): this;

      on(event: "download" | "upload", callback: (bytes: number) => void): this;

      on(event: "wire", callback: (wire: any, addr?: string) => void): this;

      on(event: "noPeers", callback: (announceType: "tracker" | "dht") => void): this;
  }

  export default class WebTorrent {
    readonly torrents: Torrent[]

    constructor (opts?: Config)
    add(torrent: string | Buffer | File | ParseTorrent, cb?: (torrent: Torrent) => any): Torrent
    destroy(callback?: (err: Error | string) => void): void
  }
}
