/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/brave_sync_worker.h"

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "brave/build/android/jni_headers/BraveSyncWorker_jni.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"

namespace chrome {
namespace android {

#define DB_FILE_NAME      "brave_sync_db"

leveldb::DB* g_level_db = nullptr;
static std::mutex* g_pLevel_db_init_mutex = new std::mutex();

BraveSyncWorker::BraveSyncWorker(JNIEnv* env, jobject obj):
  weak_java_shields_config_(env, obj) {
}

BraveSyncWorker::~BraveSyncWorker() {
}

void CreateOpenDatabase() {
    if (!g_pLevel_db_init_mutex) {
        return;
    }
    std::lock_guard<std::mutex> guard(*g_pLevel_db_init_mutex);

    if (nullptr == g_level_db) {
        base::FilePath app_data_path;
        base::PathService::Get(base::DIR_ANDROID_APP_DATA, &app_data_path);
        base::FilePath dbFilePath = app_data_path.Append(DB_FILE_NAME);

        leveldb::Options options;
        options.create_if_missing = true;
        leveldb::Status status = leveldb::DB::Open(options,
            dbFilePath.value().c_str(), &g_level_db);
        if (!status.ok() || !g_level_db) {
            if (g_level_db) {
                delete g_level_db;
                g_level_db = nullptr;
            }

            LOG(ERROR) << "sync level db open error " << DB_FILE_NAME;

            return;
        }
    }
}

base::android::ScopedJavaLocalRef<jstring>
    JNI_BraveSyncWorker_GetLocalIdByObjectId(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& jcaller,
        const base::android::JavaParamRef<jstring>& objectId) {
    CreateOpenDatabase();
    if (nullptr == g_level_db) {
        return base::android::ConvertUTF8ToJavaString(env, "");
    }

    std::string value;
    g_level_db->Get(leveldb::ReadOptions(),
        base::android::ConvertJavaStringToUTF8(objectId), &value);

    return base::android::ConvertUTF8ToJavaString(env, value);
}

base::android::ScopedJavaLocalRef<jstring>
    JNI_BraveSyncWorker_GetObjectIdByLocalId(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& localId) {
    CreateOpenDatabase();
    if (nullptr == g_level_db) {
        return base::android::ConvertUTF8ToJavaString(env, "");
    }

    std::string value;
    g_level_db->Get(leveldb::ReadOptions(),
        base::android::ConvertJavaStringToUTF8(localId), &value);

    return base::android::ConvertUTF8ToJavaString(env, value);
}

void JNI_BraveSyncWorker_SaveObjectId(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller,
      const base::android::JavaParamRef<jstring>& localId,
      const base::android::JavaParamRef<jstring>& objectIdJSON,
      const base::android::JavaParamRef<jstring>& objectId) {
    CreateOpenDatabase();
    if (nullptr == g_level_db) {
        return;
    }
    std::string strLocalId = base::android::ConvertJavaStringToUTF8(localId);

    g_level_db->Put(leveldb::WriteOptions(), strLocalId,
        base::android::ConvertJavaStringToUTF8(objectIdJSON));
    std::string strObjectId = base::android::ConvertJavaStringToUTF8(objectId);
    if (0 != strObjectId.size()) {
        g_level_db->Put(leveldb::WriteOptions(), strObjectId, strLocalId);
    }
}

void JNI_BraveSyncWorker_DeleteByLocalId(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& localId) {
    CreateOpenDatabase();
    if (nullptr == g_level_db) {
        return;
    }

    std::string strLocalId = base::android::ConvertJavaStringToUTF8(localId);
    std::string value;
    g_level_db->Get(leveldb::ReadOptions(), strLocalId, &value);

    g_level_db->Delete(leveldb::WriteOptions(), strLocalId);
    g_level_db->Delete(leveldb::WriteOptions(), value);
}

static void JNI_BraveSyncWorker_Clear(JNIEnv* env,
        const base::android::JavaParamRef<jobject>& obj) {
    if (g_level_db) {
        delete g_level_db;
        g_level_db = nullptr;
    }
    if (g_pLevel_db_init_mutex) {
        delete g_pLevel_db_init_mutex;
        g_pLevel_db_init_mutex = nullptr;
    }
}

static void JNI_BraveSyncWorker_ResetSync(JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& key) {
    CreateOpenDatabase();
    if (nullptr == g_level_db) {
        return;
    }
    g_level_db->Delete(leveldb::WriteOptions(),
        base::android::ConvertJavaStringToUTF8(key));
}

}  // namespace android
}  // namespace chrome
