# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""DNS-SD advertising and browsing (PROTOCOL.md §3.3), using python-zeroconf.

Only needed for testing against real devices on a LAN; the protocol tests
use loopback TCP or pipes.
"""

import asyncio
import os
import socket

from .keys import EPOCH_SECONDS, ChainKeys, epoch_for

SERVICE_TYPE = '_brave-xfer._tcp.local.'
TXT_VERSION = '1'


def txt_record(keys: ChainKeys, unix_seconds: float) -> dict:
    return {'v': TXT_VERSION, 'c': keys.tag(epoch_for(unix_seconds)).hex()}


def matches(keys: ChainKeys, properties: dict, unix_seconds: float) -> bool:
    props = {(k.decode() if isinstance(k, bytes) else k):
             (v.decode() if isinstance(v, bytes) else v)
             for k, v in properties.items() if v is not None}
    if props.get('v') != TXT_VERSION:
        return False
    try:
        tag = bytes.fromhex(props.get('c', ''))
    except ValueError:
        return False
    return tag in keys.accepted_tags(unix_seconds)


def _local_address() -> str:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        try:
            s.connect(('192.0.2.1', 9))
            return s.getsockname()[0]
        except OSError:
            return '127.0.0.1'


class Advertiser:
    """Publishes the responder's listening port with a rotating tag."""

    def __init__(self, keys: ChainKeys, port: int, clock):
        from zeroconf import ServiceInfo
        from zeroconf.asyncio import AsyncZeroconf
        self.keys, self.port, self.clock = keys, port, clock
        self.zc = AsyncZeroconf()
        self.name = f'{os.urandom(6).hex()}.{SERVICE_TYPE}'
        self._service_info = ServiceInfo
        self.info = None
        self._task = None

    def _make_info(self):
        return self._service_info(
            SERVICE_TYPE,
            self.name,
            addresses=[socket.inet_aton(_local_address())],
            port=self.port,
            properties=txt_record(self.keys, self.clock()))

    async def start(self):
        self.info = self._make_info()
        await self.zc.async_register_service(self.info)
        self._task = asyncio.ensure_future(self._rotate())

    async def _rotate(self):
        while True:
            now = self.clock()
            await asyncio.sleep(EPOCH_SECONDS - now % EPOCH_SECONDS + 1)
            self.info = self._make_info()
            await self.zc.async_update_service(self.info)

    async def stop(self):
        if self._task:
            self._task.cancel()
        if self.info:
            await self.zc.async_unregister_service(self.info)
        await self.zc.async_close()


async def browse(keys: ChainKeys, clock, timeout: float = 30.0):
    """Returns (host, port) of the first service whose tag matches."""
    from zeroconf import ServiceStateChange
    from zeroconf.asyncio import (AsyncServiceBrowser, AsyncServiceInfo,
                                  AsyncZeroconf)
    zc = AsyncZeroconf()
    found = asyncio.get_running_loop().create_future()
    pending = set()

    async def resolve(name):
        info = AsyncServiceInfo(SERVICE_TYPE, name)
        if not await info.async_request(zc.zeroconf, 3000):
            return
        if matches(keys, info.properties, clock()) and info.addresses \
                and not found.done():
            found.set_result((socket.inet_ntoa(info.addresses[0]), info.port))

    def on_change(zeroconf, service_type, name, state_change):
        if state_change in (ServiceStateChange.Added,
                            ServiceStateChange.Updated):
            pending.add(asyncio.ensure_future(resolve(name)))

    browser = AsyncServiceBrowser(zc.zeroconf, [SERVICE_TYPE],
                                  handlers=[on_change])
    try:
        return await asyncio.wait_for(found, timeout)
    finally:
        await browser.async_cancel()
        for task in pending:
            task.cancel()
        await zc.async_close()
