# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import logging

from rich.logging import RichHandler

from terminal import console


class IncendiaryErrorHandler(RichHandler):
    """ A custom handler that adds emojis to error messages.
    """

    def emit(self, record: logging.LogRecord) -> None:
        if record.levelno == logging.ERROR:
            record.msg = f'¯\\_(ツ)_/¯\n🔥🔥 {record.msg}'
        elif record.levelno == logging.DEBUG:
            # Debug messages should be printed as normal logs, otherwise the
            # formatting goes all over the place with the status bar.
            console.log(f'[dim]{record.getMessage()}[/]', _stack_offset=8)
            return

        super().emit(record)
