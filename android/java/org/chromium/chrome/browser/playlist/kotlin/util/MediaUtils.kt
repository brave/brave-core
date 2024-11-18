/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist.util

import android.content.Context
import android.net.Uri
import android.util.Log
import com.brave.playlist.util.ConstantUtils.HLS_FILE_EXTENSION
import java.io.File
import java.io.IOException
import java.io.InputStream


object MediaUtils {
    private val TAG: String = "Playlist/"+this::class.java.simpleName
    @JvmStatic
    @Suppress("unused")
    fun getFileSizeFromUri(context: Context, uri: Uri): Long {
        var fileSize = 0L
        var inputStream: InputStream? = null
        try {
            inputStream = context.contentResolver.openInputStream(uri)
            if (inputStream != null) {
                val bytes = ByteArray(1024)
                var read: Int
                while (inputStream.read(bytes).also { read = it } >= 0) {
                    fileSize += read.toLong()
                }
            }
        } catch (ex: Exception) {
            Log.e(TAG, ::getFileSizeFromUri.name + " : "+  ex.message.toString())
        } finally {
            if (inputStream != null) {
                try {
                    inputStream.close()
                } catch (ex: IOException) {
                    Log.e(TAG, ::getFileSizeFromUri.name + " : "+  ex.message.toString())
                }
            }
        }
        return fileSize
    }

    @JvmStatic
    @Suppress("unused")
    fun writeToFile(data: ByteArray?, filePath: String) {
        try {
            val file = File(filePath)
            data?.let { file.appendBytes(it) }
        } catch (ex:Exception) {
            Log.e(TAG, ::writeToFile.name + " : "+  ex.message.toString())
        }
    }

    @JvmStatic
    @Suppress("unused")
    fun isFileExist(filePath: String) : Boolean {
        return try {
            val file = File(filePath)
            file.exists()
        } catch (ex:Exception) {
            Log.e(TAG, ::writeToFile.name + " : "+  ex.message.toString())
            false
        }
    }

    @JvmStatic
    fun isHlsFile(mediaPath: String): Boolean {
        val extension: String = mediaPath
            .substring(mediaPath.lastIndexOf("."))
        return extension == HLS_FILE_EXTENSION
    }
}
