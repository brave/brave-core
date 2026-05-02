# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.


class UserValidationError(Exception):
    """Raised for user-facing validation failures.

    The message is printed to stderr as-is by the top-level handler.
    Raising this instead of printing-and-returning keeps command functions
    free of error-reporting boilerplate.
    """
