/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use criterion::*;
use std::alloc::{GlobalAlloc, Layout, System};
use std::sync::atomic::{AtomicUsize, Ordering};
use serde::{Deserialize, Serialize};

use adblock::Engine;
use adblock::request::Request;

#[path = "../tests/test_utils.rs"]
mod test_utils;
use test_utils::rules_from_lists;

// Custom allocator to track memory usage
#[global_allocator]
static ALLOCATOR: MemoryTracker = MemoryTracker::new();

struct MemoryTracker {
    allocated: AtomicUsize,
    internal: System,
}

impl MemoryTracker {
    const fn new() -> Self {
        Self {
            allocated: AtomicUsize::new(0),
            internal: System,
        }
    }

    fn current_usage(&self) -> usize {
        self.allocated.load(Ordering::SeqCst)
    }

    fn reset(&self) {
        self.allocated.store(0, Ordering::SeqCst);
    }
}

unsafe impl GlobalAlloc for MemoryTracker {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let ret = self.internal.alloc(layout);
        if !ret.is_null() {
            self.allocated.fetch_add(layout.size(), Ordering::SeqCst);
        }
        ret
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        self.internal.dealloc(ptr, layout);
        self.allocated.fetch_sub(layout.size(), Ordering::SeqCst);
    }

    unsafe fn realloc(&self, ptr: *mut u8, layout: Layout, new_size: usize) -> *mut u8 {
        let ret = self.internal.realloc(ptr, layout, new_size);
        if !ret.is_null() {
            self.allocated.fetch_sub(layout.size(), Ordering::SeqCst);
            self.allocated.fetch_add(new_size, Ordering::SeqCst);
        }
        ret
    }

    unsafe fn alloc_zeroed(&self, layout: Layout) -> *mut u8 {
        let ret = self.internal.alloc_zeroed(layout);
        if !ret.is_null() {
            self.allocated.fetch_add(layout.size(), Ordering::SeqCst);
        }
        ret
    }
}

#[allow(non_snake_case)]
#[derive(Serialize, Deserialize, Clone)]
struct TestRequest {
    frameUrl: String,
    url: String,
    cpt: String,
}

impl From<&TestRequest> for Request {
    fn from(v: &TestRequest) -> Self {
        Request::new(&v.url, &v.frameUrl, &v.cpt).unwrap()
    }
}

fn load_requests() -> Vec<TestRequest> {
    let requests_str = rules_from_lists(&["data/requests.json"]);
    let reqs: Vec<TestRequest> = requests_str
        .into_iter()
        .map(|r| serde_json::from_str(&r))
        .filter_map(Result::ok)
        .collect();
    reqs
}

fn bench_memory_usage(c: &mut Criterion) {
    let mut group = c.benchmark_group("memory-usage");
    group.sample_size(10);
    group.measurement_time(std::time::Duration::from_secs(1));

    let mut noise = 0;
    let all_requests = load_requests();
    let first_1000_requests: Vec<_> = all_requests.iter().take(1000).collect();

    group.bench_function("brave-list-initial", |b| {
        let mut result = 0;
        b.iter_custom(|iters| {
            for _ in 0..iters {
              ALLOCATOR.reset();
              let rules = rules_from_lists(&["data/brave/brave-main-list.txt"]);
              let engine = Engine::from_rules(rules, Default::default());

              noise += 1; // add some noise to make criterion happy
              result += ALLOCATOR.current_usage() + noise;

              // Prevent engine from being optimized
              criterion::black_box(&engine);
            }

            // Return the memory usage as a Duration
            std::time::Duration::from_nanos(result as u64)
        });
    });

    group.bench_function("brave-list-after-1000-requests", |b| {
        b.iter_custom(|iters| {
            let mut result = 0;
            for _ in 0..iters {
                ALLOCATOR.reset();
                let rules = rules_from_lists(&["data/brave/brave-main-list.txt"]);
                let engine = Engine::from_rules(rules, Default::default());

              for request in first_1000_requests.clone() {
                  criterion::black_box(engine.check_network_request(&request.into()));
              }

              noise += 1; // add some noise to make criterion happy
              result += ALLOCATOR.current_usage() + noise;

              // Prevent engine from being optimized
              criterion::black_box(&engine);
            }

            // Return the memory usage as a Duration
            std::time::Duration::from_nanos(result as u64)
        })
    });

    group.finish();
}

criterion_group!(benches, bench_memory_usage);
criterion_main!(benches);
