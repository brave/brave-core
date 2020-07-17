// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import * as Background from '../../common/Background'

// TODO: we possibly don't need this file
// actual today background def can go in common/background/today
Background.send(Background.MessageType.indicatingOpen)


// TODO: use overloading so we don't have to export each one
// https://stackoverflow.com/posts/49690749