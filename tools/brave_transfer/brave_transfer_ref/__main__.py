# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Command line for the Brave Transfer reference peer. See ../README.md."""

import argparse
import asyncio
import json
import os
import sys
import time
from pathlib import Path

from . import PROTOCOL_VERSION, bip39, proto
from .discovery import Advertiser, browse
from .keys import ChainKeys, epoch_for, fingerprint, x25519_public
from .peer import (ACCEPT_ALL, ACCEPT_FIRST, DECLINE, INITIATOR, RESPONDER,
                   Peer, PeerConfig, item_from_path)
from .session import OutgoingItem


def _keys(args) -> ChainKeys:
    if args.sync_code:
        return ChainKeys.from_sync_code(args.sync_code)
    if args.seed:
        return ChainKeys.from_seed(bytes.fromhex(args.seed))
    sys.exit('one of --sync-code or --seed is required')


def _add_key_args(p):
    p.add_argument('--sync-code', help='24-word Brave Sync code')
    p.add_argument('--seed', help='32-byte seed as hex, instead of a code')


def cmd_new_code(args):
    print(bip39.bytes32_to_mnemonic(os.urandom(32)))


def cmd_tags(args):
    keys = _keys(args)
    now = args.time if args.time is not None else time.time()
    epoch = epoch_for(now)
    print(json.dumps({
        'epoch': epoch,
        'tags': {
            str(e): keys.tag(e).hex()
            for e in (epoch - 1, epoch, epoch + 1)
        },
    }, indent=2))


def _config(args) -> PeerConfig:
    items = [
        OutgoingItem.from_bytes(proto.ITEM_KIND_TEXT, 'text.txt',
                                'text/plain', t.encode())
        for t in args.send_text
    ] + [
        OutgoingItem.from_bytes(proto.ITEM_KIND_URL, u, 'text/uri-list',
                                u.encode()) for u in args.send_url
    ] + [item_from_path(Path(f)) for f in args.send_file]
    static = bytes.fromhex(args.static) if args.static else os.urandom(32)
    info = proto.PeerInfo(protocol_version=PROTOCOL_VERSION,
                          device_name=args.name,
                          device_type=proto.DEVICE_TYPE_DESKTOP
                          if args.role == INITIATOR else proto.DEVICE_TYPE_PHONE,
                          sync_device_guid=args.guid,
                          capabilities=proto.CAPABILITY_RECEIVE
                          | proto.CAPABILITY_SEND)
    receive_dir = Path(args.receive_dir) if args.receive_dir else None
    if receive_dir:
        receive_dir.mkdir(parents=True, exist_ok=True)
    print(f'local fingerprint: {fingerprint(x25519_public(static))}',
          file=sys.stderr)
    return PeerConfig(role=args.role,
                      keys=_keys(args),
                      info=info,
                      static_private=static,
                      items=items,
                      accept=args.accept,
                      expect_offers=args.expect_offers,
                      receive_dir=receive_dir,
                      allowed_guids=set(args.allow_guid)
                      if args.allow_guid else None)


async def _run(args):
    config = _config(args)
    peer = Peer(config)
    if args.exec:
        proc = await asyncio.create_subprocess_exec(
            *args.exec, stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE)
        try:
            report = await peer.run(proc.stdout, proc.stdin)
        finally:
            code = await proc.wait()
        if code:
            raise SystemExit(f'peer process exited with {code}')
    elif args.connect or args.discover:
        if args.discover:
            host, port = await browse(config.keys, config.clock)
            print(f'found peer at {host}:{port}', file=sys.stderr)
        else:
            host, _, port = args.connect.rpartition(':')
        reader, writer = await asyncio.open_connection(host, int(port))
        report = await peer.run(reader, writer)
        writer.close()
    else:
        done = asyncio.get_running_loop().create_future()

        async def on_client(reader, writer):
            try:
                done.set_result(await peer.run(reader, writer))
            except Exception as e:  # pylint: disable=broad-except
                done.set_exception(e)
            finally:
                writer.close()

        server = await asyncio.start_server(on_client, args.host, args.listen)
        port = server.sockets[0].getsockname()[1]
        print(f'listening on {args.host}:{port}', file=sys.stderr)
        advertiser = None
        if args.advertise:
            advertiser = Advertiser(config.keys, port, config.clock)
            await advertiser.start()
        try:
            report = await done
        finally:
            server.close()
            if advertiser:
                await advertiser.stop()
    text = json.dumps(report.to_json(), indent=2)
    if args.report:
        Path(args.report).write_text(text + '\n')
    else:
        print(text)


def main(argv=None):
    parser = argparse.ArgumentParser(prog='brave_transfer_ref')
    sub = parser.add_subparsers(dest='command', required=True)

    p = sub.add_parser('new-code', help='print a random 24-word sync code')
    p.set_defaults(func=cmd_new_code)

    p = sub.add_parser('tags', help='print discovery tags around now')
    _add_key_args(p)
    p.add_argument('--time', type=float, help='unix time instead of now')
    p.set_defaults(func=cmd_tags)

    p = sub.add_parser('run', help='act as one device')
    _add_key_args(p)
    p.add_argument('--role', choices=(INITIATOR, RESPONDER), required=True)
    transport = p.add_mutually_exclusive_group()
    transport.add_argument('--connect', metavar='HOST:PORT')
    transport.add_argument('--discover', action='store_true',
                           help='browse mDNS for a matching responder')
    transport.add_argument('--exec', nargs=argparse.REMAINDER,
                           help='run a peer process and talk over its stdio')
    p.add_argument('--listen', type=int, default=0, metavar='PORT')
    p.add_argument('--host', default='0.0.0.0')
    p.add_argument('--advertise', action='store_true',
                   help='publish the listening port over mDNS')
    p.add_argument('--static', help='static X25519 private key as hex')
    p.add_argument('--name', default='Reference peer')
    p.add_argument('--guid', default='')
    p.add_argument('--allow-guid', action='append', default=[])
    p.add_argument('--send-text', action='append', default=[])
    p.add_argument('--send-url', action='append', default=[])
    p.add_argument('--send-file', action='append', default=[])
    p.add_argument('--accept', choices=(ACCEPT_ALL, ACCEPT_FIRST, DECLINE),
                   default=ACCEPT_ALL)
    p.add_argument('--expect-offers', type=int, default=0)
    p.add_argument('--receive-dir')
    p.add_argument('--report', help='write the JSON report here')
    p.set_defaults(func=lambda a: asyncio.run(_run(a)))

    args = parser.parse_args(argv)
    args.func(args)


if __name__ == '__main__':
    main()
