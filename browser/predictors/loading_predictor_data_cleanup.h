/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PREDICTORS_LOADING_PREDICTOR_DATA_CLEANUP_H_
#define BRAVE_BROWSER_PREDICTORS_LOADING_PREDICTOR_DATA_CLEANUP_H_

namespace sql {
class Database;
}  // namespace sql

namespace predictors {

// Drops what the loading predictor learned while it was still running. Brave
// disables it (see IsLoadingPredictorEnabled()), so its data is never read or
// written again, but a profile that collected some before it was turned off
// would otherwise keep it indefinitely -- the predictor's own history deletion
// cleanup no longer runs either. Tables the loading predictor does not own are
// left alone, including the omnibox predictor's, which shares the database.
//
// Does nothing when `is_loading_predictor_enabled` is true, or once the tables
// hold no rows, which is the steady state because upstream recreates them
// empty right after this runs.
//
// `db` must be open, and this must be called on its sequence.
void MaybeClearLoadingPredictorData(sql::Database* db,
                                    bool is_loading_predictor_enabled);

}  // namespace predictors

#endif  // BRAVE_BROWSER_PREDICTORS_LOADING_PREDICTOR_DATA_CLEANUP_H_
