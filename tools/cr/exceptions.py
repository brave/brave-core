# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Shared exceptions for the Brockit toolchain.

These are brockit exceptions and they are better suited for the type of error
reporting we have with brockit.
"""

from __future__ import annotations

import logging

from terminal import console


class InvalidInputException(Exception):
    """Invalid input exceptions to be handled graciously and terminate."""

    def __init__(self, message):
        super().__init__(message)
        logging.error(message)


class BadOutcomeException(Exception):
    """A failure along the way that results on task termination."""

    def __init__(self, message):
        super().__init__(message)
        logging.warning(message)


class ActionNeededException(Exception):
    """An expected failure along the way that results on task termination."""

    def __init__(self, message: str):
        super().__init__(message)
        console.log(message)
