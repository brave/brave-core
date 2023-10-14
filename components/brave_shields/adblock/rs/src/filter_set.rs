/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use adblock::{lists::ParseOptions, resources::PermissionMask, FilterSet as InnerFilterSet};
use cxx::CxxVector;

use crate::ffi::{FilterListMetadata, FilterListMetadataResult};
use crate::result::InternalError;

pub struct FilterSet(pub(crate) InnerFilterSet);

impl Default for Box<FilterSet> {
    fn default() -> Self {
        new_filter_set()
    }
}

pub fn new_filter_set() -> Box<FilterSet> {
    Box::new(FilterSet(InnerFilterSet::new(false)))
}

impl FilterSet {
    pub fn add_filter_list(&mut self, rules: &CxxVector<u8>) -> FilterListMetadataResult {
        self.add_filter_list_with_permissions(rules, 0)
    }

    pub fn add_filter_list_with_permissions(
        &mut self,
        rules: &CxxVector<u8>,
        permission_mask: u8,
    ) -> FilterListMetadataResult {
        || -> Result<FilterListMetadata, InternalError> {
            Ok(self
                .0
                .add_filter_list(
                    std::str::from_utf8(rules.as_slice())?,
                    ParseOptions {
                        permissions: PermissionMask::from_bits(permission_mask),
                        ..Default::default()
                    },
                )
                .into())
        }()
        .into()
    }
}
