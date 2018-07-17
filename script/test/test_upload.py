#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import sys
import unittest
import os

dirname = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(dirname, '..'))

import upload

from mock import Repo

class TestGetDraft(unittest.TestCase):
  def setUp(self):
    self.repo = Repo()

  def test_returns_existing_draft(self):
    self.repo.releases._releases = [{'tag_name': 'test', 'draft': True}]
    self.assertEquals(upload.get_draft(self.repo, 'test')['tag_name'], 'test')

  def test_fails_on_existing_release(self):
    self.repo.releases._releases = [{'tag_name': 'test', 'draft': False}]
    self.assertRaises(UserWarning, upload.get_draft, self.repo, 'test')

  def test_returns_none_on_new_draft(self):
    self.repo.releases._releases = [{'tag_name': 'old', 'draft': False}]
    upload.get_draft(self.repo, 'new')
    self.assertEquals(upload.get_draft(self.repo, 'test'), None)

class TestYieldBravePackages(unittest.TestCase):
  def setUp(self):
    self.yield_pkgs_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'test_yield_pkgs')

  def test_only_returns_dev_darwin_package(self):
    upload.PLATFORM = 'darwin'
    pkgs = list(upload.yield_brave_packages(os.path.join(self.yield_pkgs_dir, upload.PLATFORM), 'dev', '0.50.8'))
    self.assertEquals(pkgs, ['Brave-Browser-Dev.dmg'])

  def test_only_returns_dev_linux_packages(self):
    upload.PLATFORM = 'linux'
    pkgs = list(upload.yield_brave_packages(os.path.join(self.yield_pkgs_dir, upload.PLATFORM), 'dev', '0.50.8'))
    self.assertEquals(sorted(pkgs), sorted(['brave-browser-dev-0.50.8-1.x86_64.rpm', 'brave-browser-dev_0.50.8_amd64.deb']))


if __name__ == '__main__':
  print unittest.main()
