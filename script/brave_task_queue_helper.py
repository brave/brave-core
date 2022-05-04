#!/usr/bin/env vpython3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import sys

def limit_pool_size(task_queue):
    if sys.platform == 'darwin':
        task_queue._pool_size = min(task_queue._pool_size, 4) # pylint: disable=protected-access

