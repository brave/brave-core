/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// NOLINT(build/header_guard)
// no-include-guard-because-multiply-included

TOR_EVENT(ADDRMAP)
TOR_EVENT(AUTHDIR_NEWDESCS)
TOR_EVENT(BUILDTIMEOUT_SET)
TOR_EVENT(BW)
TOR_EVENT(CELL_STATS)
TOR_EVENT(CIRC)
TOR_EVENT(CIRC_BW)
TOR_EVENT(CIRC_MINOR)
TOR_EVENT(CLIENTS_SEEN)
TOR_EVENT(CONF_CHANGED)
TOR_EVENT(CONN_BW)
TOR_EVENT(DEBUG)
TOR_EVENT(DESCCHANGED)
TOR_EVENT(ERR)
TOR_EVENT(GUARD)
TOR_EVENT(HS_DESC)
// TOR_EVENT(HS_DESC_CONTENT)   // omitted because uses data replies
TOR_EVENT(INFO)
TOR_EVENT(NETWORK_LIVENESS)
// TOR_EVENT(NEWCONSENSUS)      // omitted because uses data replies
TOR_EVENT(NEWDESC)
TOR_EVENT(NOTICE)
// TOR_EVENT(NS)                // omitted because uses data replies
TOR_EVENT(ORCONN)
TOR_EVENT(SIGNAL)
TOR_EVENT(STATUS_CLIENT)
TOR_EVENT(STATUS_GENERAL)
TOR_EVENT(STATUS_SERVER)
TOR_EVENT(STREAM)
TOR_EVENT(STREAM_BW)
TOR_EVENT(TB_EMPTY)
TOR_EVENT(TRANSPORT_LAUNCHED)
TOR_EVENT(WARN)
