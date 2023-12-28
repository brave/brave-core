# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils

# pylint: disable=undefined-variable, function-redefined


def _add_brave_metrics(options):
    options.ExtendTimelineBasedMetric(['braveNavigationMetric'])
    return options


@override_utils.override_method(MobileCommonSystemHealth)
def CreateCoreTimelineBasedMeasurementOptions(self, original_method):
    return _add_brave_metrics(original_method(self))


@override_utils.override_method(DesktopCommonSystemHealth)
def CreateCoreTimelineBasedMeasurementOptions(self, original_method):
    return _add_brave_metrics(original_method(self))
