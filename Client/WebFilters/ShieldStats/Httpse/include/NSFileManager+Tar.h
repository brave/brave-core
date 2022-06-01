//
//  NSFileManager+Tar.h
//  Tar
//
//  Created by Mathieu Hausherr Octo Technology on 25/11/11.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#import <Foundation/Foundation.h>

/**
 *  Block to follow the extracting of the archive
 *
 *  @param float Percentage of the archive extracted
 */
typedef void(^NSFileManagerTarProgressBlock)(float);

@interface NSFileManager (Tar)

/**
 *  Extract the tar archive into the directory at the url.
 *
 *  @param url           URL of the destination directory
 *  @param tarData       Tar archive
 *  @param error         Error occuring during the extraction of the archive
 *  @param progressBlock Block to follow the percentage of archive extracted
 *
 *  @return `YES` if the archive has been successfully extracted
 */
- (BOOL)createFilesAndDirectoriesAtURL:(NSURL *)url
                           withTarData:(NSData *)tarData
                                 error:(NSError **)error
                              progress:(NSFileManagerTarProgressBlock)progressBlock;
/**
 *  Extract the tar archive into the given directory.
 *
 *  @param path          Path of the destination directory
 *  @param tarData       Tar archive
 *  @param error         Error occuring during the extraction of the archive
 *  @param progressBlock Block to follow the percent of archive extracted
 *
 *  @return `YES` if the archive has been successfully extracted
 */
- (BOOL)createFilesAndDirectoriesAtPath:(NSString *)path
                            withTarData:(NSData *)tarData
                                  error:(NSError **)error
                               progress:(NSFileManagerTarProgressBlock)progressBlock;
/**
 *  Extract the tar archive into the directory at the url.
 *
 *  @param path          Path of the destination directory
 *  @param tarPath       Path of the archive
 *  @param error         Error occuring during the extraction of the archive
 *  @param progressBlock Block to follow the percent of archive extracted
 *
 *  @return `YES` if the archive has been successfully extracted
 */
- (BOOL)createFilesAndDirectoriesAtPath:(NSString *)path
                            withTarPath:(NSString *)tarPath
                                  error:(NSError **)error
                               progress:(NSFileManagerTarProgressBlock)progressBlock;

@end

/**
 *  Error corresponding particularly to Light-Untar library.
 */
extern NSString * const NSFileManagerLightUntarErrorDomain;
