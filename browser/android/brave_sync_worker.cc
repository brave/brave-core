/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/brave_sync_worker.h"

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "brave/build/android/jni_headers/BraveSyncWorker_jni.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"

namespace chrome {
namespace android {

#define DB_FILE_NAME      "brave_sync_db"

BraveSyncWorker::BraveSyncWorker(JNIEnv* env, jobject obj):
  weak_java_shields_config_(env, obj) {
}

BraveSyncWorker::~BraveSyncWorker() {
}

}  // namespace android
}  // namespace chrome
