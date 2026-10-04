// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BackgroundTasks
import Foundation
import OSLog
import OrderedCollections
import Strings
import Web

@MainActor
public class DownloadBackgroundTaskScheduler {
  private var task: BGTask?
  private let taskIdentifier: String
  private let taskScheduler: BGTaskScheduler
  private var isTaskBeingScheduled: Bool = false
  private var downloads: OrderedSet<Download> = []

  @available(iOS 26.0, *)
  public init(
    taskIdentifier: String,
    taskScheduler: BGTaskScheduler = .shared
  ) {
    self.taskIdentifier = taskIdentifier
    self.taskScheduler = taskScheduler

    taskScheduler.register(
      forTaskWithIdentifier: taskIdentifier,
      using: .main
    ) { [weak self] task in
      guard let self, let task = task as? BGContinuedProcessingTask else { return }
      MainActor.assumeIsolated {
        self.taskSchedulerLaunchedTask(task)
      }
    }
  }

  @available(iOS 26.0, *)
  public func addDownload(_ download: Download) {
    downloads.append(download)
    updateActiveTaskProgress()
    submitTaskRequestForDownloads()
  }

  @available(iOS 26.0, *)
  public func removeDownload(_ download: Download) {
    downloads.remove(download)
    updateActiveTaskProgress()
    if downloads.isEmpty {
      if let task {
        task.setTaskCompleted(success: true)
        self.task = nil
      } else if isTaskBeingScheduled {
        taskScheduler.cancel(taskRequestWithIdentifier: taskIdentifier)
        isTaskBeingScheduled = false
      }
    }
  }

  @available(iOS 26.0, *)
  public func updateActiveTaskProgress() {
    guard let task = task as? BGContinuedProcessingTask, !downloads.isEmpty else { return }
    let progress = currentProgress
    task.progress.totalUnitCount = progress.totalUnitCount
    task.progress.completedUnitCount = progress.completedUnitCount
    task.updateTitle(progress.title, subtitle: progress.subtitle)
  }

  private struct DownloadsTaskProgress {
    var title: String
    var subtitle: String
    var totalUnitCount: Int64
    var completedUnitCount: Int64
  }

  private var currentProgress: DownloadsTaskProgress {
    let title: String
    if downloads.count == 1 {
      title = String.localizedStringWithFormat(
        Strings.backgroundDownloadingTitleSingleDownload,
        downloads[0].filename
      )
    } else {
      title = String.localizedStringWithFormat(
        Strings.backgroundDownloadingTitleMultiDownload,
        downloads.count
      )
    }

    let formatter = ByteCountFormatter()
    formatter.countStyle = .file
    formatter.zeroPadsFractionDigits = true  // Otherwise the progress label jumps around

    let combinedBytesDownloaded = downloads.reduce(0) { $0 + $1.bytesDownloaded }
    let combinedTotalBytesExpected = downloads.reduce(0) { $0 + ($1.totalBytesExpected ?? 0) }
    let downloadedSize = formatter.string(fromByteCount: combinedBytesDownloaded)
    let expectedSize =
      combinedTotalBytesExpected != 0
      ? formatter.string(fromByteCount: combinedTotalBytesExpected) : nil
    let subtitle =
      expectedSize.map {
        String.localizedStringWithFormat(
          Strings.backgroundDownloadingSubtitleWithExpectedSize,
          downloadedSize,
          $0
        )
      } ?? String.localizedStringWithFormat(Strings.backgroundDownloadingSubtitle, downloadedSize)
    return .init(
      title: title,
      subtitle: subtitle,
      totalUnitCount: combinedTotalBytesExpected,
      completedUnitCount: combinedBytesDownloaded
    )
  }

  @available(iOS 26.0, *)
  private func submitTaskRequestForDownloads() {
    // Ensure a task isn't already requested and that there are active downloads being tracked
    guard task == nil, !isTaskBeingScheduled, !downloads.isEmpty else {
      return
    }
    let progress = currentProgress
    let request = BGContinuedProcessingTaskRequest(
      identifier: taskIdentifier,
      title: progress.title,
      subtitle: progress.subtitle
    )
    do {
      try taskScheduler.submit(request)
      isTaskBeingScheduled = true
    } catch {
      Logger.module.error("Failed to submit download continued processing task: \(error)")
    }
  }

  @available(iOS 26.0, *)
  private func taskSchedulerLaunchedTask(_ task: BGContinuedProcessingTask) {
    isTaskBeingScheduled = false
    if downloads.isEmpty {
      // Already removed prior download before the task launched
      task.setTaskCompleted(success: false)
      return
    }
    self.task = task
    task.expirationHandler = { [weak self] in
      Task { @MainActor in
        self?.handleTaskExpiredOrCancelled()
        task.setTaskCompleted(success: false)
      }
    }
    updateActiveTaskProgress()
  }

  @available(iOS 26.0, *)
  private func handleTaskExpiredOrCancelled() {
    for download in downloads {
      download.cancel()
    }
    downloads.removeAll()
    self.task = nil
  }
}
