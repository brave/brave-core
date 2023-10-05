// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UniformTypeIdentifiers
import AVFoundation

// IANA List of Audio types: https://www.iana.org/assignments/media-types/media-types.xhtml#audio
// IANA List of Video types: https://www.iana.org/assignments/media-types/media-types.xhtml#video
// APPLE List of UTI types: https://developer.apple.com/library/archive/documentation/Miscellaneous/Reference/UTIRef/Articles/System-DeclaredUniformTypeIdentifiers.html

public class PlaylistMimeTypeDetector {
  private(set) var mimeType: String?
  private(set) var fileExtension: String?  // When nil, assume `mpg` format.
  
  init(url: URL) {
    let possibleFileExtension = url.pathExtension.lowercased()
    if let supportedExtension = knownFileExtensions.first(where: { $0.lowercased() == possibleFileExtension }) {
      self.fileExtension = supportedExtension
      self.mimeType = mimeTypeMap.first(where: { $0.value == supportedExtension })?.key
    } else if let fileExtension = PlaylistMimeTypeDetector.supportedAVAssetFileExtensions().first(where: { $0.lowercased() == possibleFileExtension }) {
      self.fileExtension = fileExtension
      self.mimeType = UTType(filenameExtension: fileExtension)?.preferredMIMEType
    }
  }
  
  init(mimeType: String) {
    if let fileExtension = mimeTypeMap[mimeType.lowercased()] {
      self.mimeType = mimeType
      self.fileExtension = fileExtension
    } else if let mimeType = PlaylistMimeTypeDetector.supportedAVAssetMimeTypes().first(where: { $0.lowercased() == mimeType.lowercased() }) {
      self.mimeType = mimeType
      self.fileExtension = UTType(mimeType: mimeType)?.preferredFilenameExtension
    }
  }
  
  init(data: Data) {
    // Assume mpg by default. If it can't play, it will fail anyway..
    // AVPlayer REQUIRES that you give a file extension no matter what and will refuse to determine the extension for you without an
    // AVResourceLoaderDelegate :S
    
    if findHeader(offset: 0, data: data, header: [0x1A, 0x45, 0xDF, 0xA3]) {
      mimeType = "video/webm"
      fileExtension = "webm"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0x1A, 0x45, 0xDF, 0xA3]) {
      mimeType = "video/matroska"
      fileExtension = "mkv"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0x4F, 0x67, 0x67, 0x53]) {
      mimeType = "application/ogg"
      fileExtension = "ogg"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0x52, 0x49, 0x46, 0x46]) && findHeader(offset: 8, data: data, header: [0x57, 0x41, 0x56, 0x45]) {
      mimeType = "audio/x-wav"
      fileExtension = "wav"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0xFF, 0xFB]) || findHeader(offset: 0, data: data, header: [0x49, 0x44, 0x33]) {
      mimeType = "audio/mpeg"
      fileExtension = "mp4"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0x66, 0x4C, 0x61, 0x43]) {
      mimeType = "audio/flac"
      fileExtension = "flac"
      return
    }
    
    if findHeader(offset: 4, data: data, header: [0x66, 0x74, 0x79, 0x70, 0x4D, 0x53, 0x4E, 0x56]) || findHeader(offset: 4, data: data, header: [0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6F, 0x6D]) || findHeader(offset: 4, data: data, header: [0x66, 0x74, 0x79, 0x70, 0x6D, 0x70, 0x34, 0x32]) || findHeader(offset: 0, data: data, header: [0x33, 0x67, 0x70, 0x35]) {
      mimeType = "video/mp4"
      fileExtension = "mp4"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0x00, 0x00, 0x00, 0x1C, 0x66, 0x74, 0x79, 0x70, 0x4D, 0x34, 0x56]) {
      mimeType = "video/x-m4v"
      fileExtension = "m4v"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0x00, 0x00, 0x00, 0x14, 0x66, 0x74, 0x79, 0x70]) {
      mimeType = "video/quicktime"
      fileExtension = "mov"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0x52, 0x49, 0x46, 0x46]) && findHeader(offset: 8, data: data, header: [0x41, 0x56, 0x49]) {
      mimeType = "video/x-msvideo"
      fileExtension = "avi"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9]) {
      mimeType = "video/x-ms-wmv"
      fileExtension = "wmv"
      return
    }
    
    // Maybe
    if findHeader(offset: 0, data: data, header: [0x00, 0x00, 0x01]) {
      mimeType = "video/mpeg"
      fileExtension = "mpg"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0x49, 0x44, 0x33]) || findHeader(offset: 0, data: data, header: [0xFF, 0xFB]) {
      mimeType = "audio/mpeg"
      fileExtension = "mp3"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0x4D, 0x34, 0x41, 0x20]) || findHeader(offset: 4, data: data, header: [0x66, 0x74, 0x79, 0x70, 0x4D, 0x34, 0x41]) {
      mimeType = "audio/m4a"
      fileExtension = "m4a"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0x23, 0x21, 0x41, 0x4D, 0x52, 0x0A]) {
      mimeType = "audio/amr"
      fileExtension = "amr"
      return
    }
    
    if findHeader(offset: 0, data: data, header: [0x46, 0x4C, 0x56, 0x01]) {
      mimeType = "video/x-flv"
      fileExtension = "flv"
      return
    }
    
    mimeType = "application/x-mpegURL"  // application/vnd.apple.mpegurl
    fileExtension = nil
  }
  
  private func findHeader(offset: Int, data: Data, header: [UInt8]) -> Bool {
    if offset < 0 || data.count < offset + header.count {
      return false
    }
    
    return [UInt8](data[offset..<(offset + header.count)]) == header
  }
  
  /// Converts a list of AVFileType to a list of file extensions
  private static func supportedAVAssetFileExtensions() -> [String] {
    let types = AVURLAsset.audiovisualTypes()
    return types.compactMap({ UTType($0.rawValue)?.preferredFilenameExtension }).filter({ !$0.isEmpty })
  }
  
  /// Converts a list of AVFileType to a list of mime-types
  private static func supportedAVAssetMimeTypes() -> [String] {
    let types = AVURLAsset.audiovisualTypes()
    return types.compactMap({ UTType($0.rawValue)?.preferredMIMEType }).filter({ !$0.isEmpty })
  }
  
  private let knownFileExtensions = [
    "mov",
    "qt",
    "mp4",
    "m4v",
    "m4a",
    "m4b",  // DRM protected
    "m4p",  // DRM protected
    "3gp",
    "3gpp",
    "sdv",
    "3g2",
    "3gp2",
    "caf",
    "wav",
    "wave",
    "bwf",
    "aif",
    "aiff",
    "aifc",
    "cdda",
    "amr",
    "mp3",
    "au",
    "snd",
    "ac3",
    "eac3",
    "flac",
    "aac",
    "mp2",
    "pls",
    "avi",
    "webm",
    "ogg",
    "mpg",
    "mpg4",
    "mpeg",
    "mpg3",
    "wma",
    "wmv",
    "swf",
    "flv",
    "mng",
    "asx",
    "asf",
    "mkv",
  ]
  
  private let mimeTypeMap = [
    "audio/x-wav": "wav",
    "audio/vnd.wave": "wav",
    "audio/aacp": "aacp",
    "audio/mpeg3": "mp3",
    "audio/mp3": "mp3",
    "audio/x-caf": "caf",
    "audio/mpeg": "mp3",  // mpg3
    "audio/x-mpeg3": "mp3",
    "audio/wav": "wav",
    "audio/flac": "flac",
    "audio/x-flac": "flac",
    "audio/mp4": "mp4",
    "audio/x-mpg": "mp3",  // maybe mpg3
    "audio/scpls": "pls",
    "audio/x-aiff": "aiff",
    "audio/usac": "eac3",  // Extended AC3
    "audio/x-mpeg": "mp3",
    "audio/wave": "wav",
    "audio/x-m4r": "m4r",
    "audio/x-mp3": "mp3",
    "audio/amr": "amr",
    "audio/aiff": "aiff",
    "audio/3gpp2": "3gp2",
    "audio/aac": "aac",
    "audio/mpg": "mp3",  // mpg3
    "audio/mpegurl": "mpg",  // actually .m3u8, .m3u HLS stream
    "audio/x-m4b": "m4b",
    "audio/x-m4p": "m4p",
    "audio/x-scpls": "pls",
    "audio/x-mpegurl": "mpg",  // actually .m3u8, .m3u HLS stream
    "audio/x-aac": "aac",
    "audio/3gpp": "3gp",
    "audio/basic": "au",
    "audio/au": "au",
    "audio/snd": "snd",
    "audio/x-m4a": "m4a",
    "audio/x-realaudio": "ra",
    "video/3gpp2": "3gp2",
    "video/quicktime": "mov",
    "video/mp4": "mp4",
    "video/mp4v": "mp4",
    "video/mpg": "mpg",
    "video/mpeg": "mpeg",
    "video/x-mpg": "mpg",
    "video/x-mpeg": "mpeg",
    "video/avi": "avi",
    "video/x-m4v": "m4v",
    "video/mp2t": "ts",
    "application/vnd.apple.mpegurl": "mpg",  // actually .m3u8, .m3u HLS stream
    "video/3gpp": "3gp",
    "text/vtt": "vtt",  // Subtitles format
    "application/mp4": "mp4",
    "application/x-mpegurl": "mpg",  // actually .m3u8, .m3u HLS stream
    "video/webm": "webm",
    "application/ogg": "ogg",
    "video/msvideo": "avi",
    "video/x-msvideo": "avi",
    "video/x-ms-wmv": "wmv",
    "video/x-ms-wma": "wma",
    "application/x-shockwave-flash": "swf",
    "video/x-flv": "flv",
    "video/x-mng": "mng",
    "video/x-ms-asx": "asx",
    "video/x-ms-asf": "asf",
    "video/matroska": "mkv",
  ]
}
