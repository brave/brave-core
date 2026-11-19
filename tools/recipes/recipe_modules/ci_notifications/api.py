# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `ci_notifications` module API."""

from __future__ import annotations

from typing import TYPE_CHECKING

from recipe_api import RecipeApi
from recipe_modules.ci_issues import Filing

from .notifications import Notification, compose

if TYPE_CHECKING:
    from recipe_modules import ci_notifications

# The output property the messages are reported in.
OUTPUT_PROPERTY = 'notifications'


class CiNotificationsApi(RecipeApi):
    """Reports the messages a build has for the people following it, for CI
    to send."""

    m: ci_notifications.DEPS

    def report(self, filing: Filing) -> tuple[Notification, ...]:
        """Report the messages for what the build found and filed as its
        `notifications` output property: each with the audience it is for
        and its text, in Slack's markup.

        Args:
            filing: What the build had filed.

        Returns:
            The messages.
        """
        notifications = compose(filing)
        step = self.m.step('notifications', None)
        step.presentation.properties[OUTPUT_PROPERTY] = [
            notification.to_json() for notification in notifications
        ]
        return notifications
