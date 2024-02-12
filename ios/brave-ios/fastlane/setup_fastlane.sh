# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/. 

#move all the build tools to the right places
clone_folder="../../build-tools"
rsync -a ${clone_folder}/scripts ../
rsync -a ${clone_folder}/fastlane/Appfile Appfile
rsync -a ${clone_folder}/fastlane/Snapfile Snapfile
rsync -a ${clone_folder}/fastlane/SnapshotHelper.swift SnapshotHelper.swift
rsync -a ${clone_folder}/fastlane/scripts .
rsync -a ${clone_folder}/fastlane/frames .
rsync -a ${clone_folder}/fastlane/templates .
rsync -a ${clone_folder}/fastlane/BaseFastfile Fastfile
