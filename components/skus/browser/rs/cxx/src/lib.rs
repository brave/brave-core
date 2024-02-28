// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

mod errors;
mod httpclient;
mod log;
mod storage;

use std::cell::RefCell;
use std::fmt;
use std::rc::Rc;
use std::sync::Arc;
use std::thread;

use cxx::{type_id, ExternType, UniquePtr};
use futures::executor::{LocalPool, LocalSpawner};
use futures::future::BoxFuture;
use futures::task::ArcWake;
use futures::task::Context;
use futures::task::LocalSpawnExt;
use futures::task::waker_ref;
use futures::lock::Mutex;

use tracing::debug;

pub use skus;

use crate::httpclient::{HttpRoundtripContext, WakeupContext};
use crate::storage::{StorageGetContext, StoragePurgeContext, StorageSetContext};
use errors::result_to_string;

pub struct NativeClientExecutor {
    is_shutdown: bool,
    pool: Option<LocalPool>,
    spawner: LocalSpawner,
    thread_id: thread::ThreadId,
}

#[derive(Clone)]
pub struct NativeClientInner {
    environment: skus::Environment,
    executor: Rc<RefCell<NativeClientExecutor>>,
    ctx: Rc<RefCell<UniquePtr<ffi::SkusContext>>>,
}

#[derive(Clone)]
pub struct NativeClient {
    executor: Rc<RefCell<NativeClientExecutor>>,
    inner: Rc<Mutex<NativeClientInner>>,
}

impl fmt::Debug for NativeClient {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("NativeClient").finish()
    }
}

impl NativeClient {
    fn try_run_until_stalled(&self) {
        let executor = self.executor.clone();
        if let Ok(mut executor) = executor.try_borrow_mut() {
            executor.try_run_until_stalled()
        };
    }

    fn get_spawner(&self) -> LocalSpawner {
        self.executor.borrow().spawner.clone()
    }
}

impl NativeClientExecutor {
    fn new() -> Self {
        let pool = LocalPool::new();
        let spawner = pool.spawner();
        Self {
            is_shutdown: false,
            pool: Some(pool),
            spawner,
            thread_id: thread::current().id(),
        }
    }

    fn shutdown(&mut self) {
        // drop any existing futures
        drop(self.pool.take());
        // ensure lingering callbacks passed to c++ are short circuited
        self.is_shutdown = true;
    }

    fn try_run_until_stalled(&mut self) {
        assert!(thread::current().id() == self.thread_id, "sdk called on a different thread!");
        let _ = thread::current().id() == self.thread_id;
        if self.is_shutdown {
            debug!("sdk is shutdown, exiting");
            return;
        }
        if let Some(pool) = &mut self.pool {
            pool.run_until_stalled();
        }
    }
}

#[allow(unused)]
#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = skus)]
mod ffi {
    #[derive(Debug)]
    pub enum TracingLevel {
        /// The "trace" level.
        ///
        /// Designates very low priority, often extremely verbose, information.
        Trace = 0,
        /// The "debug" level.
        ///
        /// Designates lower priority information.
        Debug = 1,
        /// The "info" level.
        ///
        /// Designates useful information.
        Info = 2,
        /// The "warn" level.
        ///
        /// Designates hazardous situations.
        Warn = 3,
        /// The "error" level.
        ///
        /// Designates very serious errors.
        Error = 4,
    }

    #[derive(Debug)]
    pub enum SkusResult {
        Ok,
        RequestFailed,
        InternalServer,
        BadRequest,
        UnhandledStatus,
        RetryLater,
        NotFound,
        SerializationFailed,
        InvalidResponse,
        InvalidProof,
        QueryError,
        OutOfCredentials,
        StorageWriteFailed,
        StorageReadFailed,
        OrderUnpaid,
        UnhandledVariant,
        OrderLocationMismatch,
        OrderMisconfiguration,
        ItemCredentialsMissing,
        ItemCredentialsExpired,
        InvalidMerchantOrSku,
        UnknownError,
        BorrowFailed,
        FutureCancelled,
        InvalidCall,
    }

    pub struct HttpRequest {
        pub url: String,
        pub method: String,
        pub headers: Vec<String>,
        pub body: Vec<u8>,
    }
    #[derive(Debug)]
    pub struct HttpResponse<'a> {
        pub result: SkusResult,
        pub return_code: u16,
        pub headers: &'a CxxVector<CxxString>,
        pub body: &'a CxxVector<u8>,
    }

    extern "Rust" {
        type HttpRoundtripContext;
        type WakeupContext;
        type StoragePurgeContext;
        type StorageSetContext;
        type StorageGetContext;
        type ArcTask;

        type CppSDK;
        fn initialize_sdk(ctx: UniquePtr<SkusContext>, env: String) -> Box<CppSDK>;
        fn shutdown(self: &CppSDK);
        fn refresh_order(
            self: &CppSDK,
            callback: RefreshOrderCallback,
            callback_state: UniquePtr<RefreshOrderCallbackState>,
            order_id: String,
        );
        fn fetch_order_credentials(
            self: &CppSDK,
            callback: FetchOrderCredentialsCallback,
            callback_state: UniquePtr<FetchOrderCredentialsCallbackState>,
            order_id: String,
        );
        fn prepare_credentials_presentation(
            self: &CppSDK,
            callback: PrepareCredentialsPresentationCallback,
            callback_state: UniquePtr<PrepareCredentialsPresentationCallbackState>,
            domain: String,
            path: String,
        );
        fn credential_summary(
            self: &CppSDK,
            callback: CredentialSummaryCallback,
            callback_state: UniquePtr<CredentialSummaryCallbackState>,
            domain: String,
        );
        fn submit_receipt(
            self: &CppSDK,
            callback: SubmitReceiptCallback,
            callback_state: UniquePtr<SubmitReceiptCallbackState>,
            order_id: String,
            receipt: String,
        );
        fn create_order_from_receipt(
            self: &CppSDK,
            callback: CreateOrderFromReceiptCallback,
            callback_state: UniquePtr<CreateOrderFromReceiptCallbackState>,
            receipt: String,
        );

        fn result_to_string(result: &SkusResult) -> String;

        fn init_runner(
            runner: UniquePtr<TaskRunner>,
        );
        fn poll(self: &ArcTask);
    }

    unsafe extern "C++" {

        type SkusContext;
        type SkusUrlLoader;
        type TaskRunner;

        fn shim_logMessage(file: &str, line: u32, level: TracingLevel, message: &str);

        fn shim_executeRequest(
            ctx: &SkusContext,
            req: &HttpRequest,
            done: fn(Box<HttpRoundtripContext>, resp: HttpResponse),
            rt_ctx: Box<HttpRoundtripContext>,
        ) -> UniquePtr<SkusUrlLoader>;

        fn shim_scheduleWakeup(
            delay_ms: u64,
            done: fn(Box<WakeupContext>),
            ctx: Box<WakeupContext>,
        );

        fn shim_purge(
            ctx: Pin<&mut SkusContext>,
            done: fn(Box<StoragePurgeContext>, bool),
            st_ctx: Box<StoragePurgeContext>,
        );

        fn shim_set(
            ctx: Pin<&mut SkusContext>,
            key: &str,
            value: &str,
            done: fn(Box<StorageSetContext>, bool),
            st_ctx: Box<StorageSetContext>,
        );

        fn shim_get(
            ctx: Pin<&mut SkusContext>,
            key: &str,
            done: fn(Box<StorageGetContext>, String, bool),
            st_ctx: Box<StorageGetContext>,
        );

        fn shim_postTask(
            runner: &TaskRunner,
            task: Box<ArcTask>,
        );

        type RefreshOrderCallbackState;
        type RefreshOrderCallback = crate::RefreshOrderCallback;
        type FetchOrderCredentialsCallbackState;
        type FetchOrderCredentialsCallback = crate::FetchOrderCredentialsCallback;
        type PrepareCredentialsPresentationCallbackState;
        type PrepareCredentialsPresentationCallback = crate::PrepareCredentialsPresentationCallback;
        type CredentialSummaryCallbackState;
        type CredentialSummaryCallback = crate::CredentialSummaryCallback;
        type SubmitReceiptCallbackState;
        type SubmitReceiptCallback = crate::SubmitReceiptCallback;
        type CreateOrderFromReceiptCallbackState;
        type CreateOrderFromReceiptCallback = crate::CreateOrderFromReceiptCallback;
    }
}

unsafe impl Sync for ffi::TaskRunner {}
unsafe impl Send for ffi::TaskRunner {}

/// A future that can reschedule itself to be polled by an `Executor`.
struct Task {
    /// In-progress future that should be pushed to completion.
    ///
    /// The `Mutex` is not necessary for correctness, since we only have
    /// one thread executing tasks at once. However, Rust isn't smart
    /// enough to know that `future` is only mutated from one thread,
    /// so we need to use the `Mutex` to prove thread-safety. A production
    /// executor would not need this, and could use `UnsafeCell` instead.
    future: std::sync::Mutex<Option<BoxFuture<'static, ()>>>,

    /// Handle to place the task itself back onto the task queue.
    task_runner: UniquePtr<ffi::TaskRunner>,
}

struct ArcTask(Arc<Task>);

impl ArcWake for Task {
    fn wake_by_ref(arc_self: &Arc<Self>) {
        // Implement `wake` by sending this task back onto the task channel
        // so that it will be polled again by the executor.
        let cloned = ArcTask(arc_self.clone());
        ffi::shim_postTask(&arc_self.task_runner, Box::new(cloned));
    }
}

impl ArcTask {
    fn poll(&self) {
        // Take the future, and if it has not yet completed (is still Some),
        // poll it in an attempt to complete it.
        let mut future_slot = self.0.future.lock().unwrap();
        if let Some(mut future) = future_slot.take() {
            // Create a `LocalWaker` from the task itself
            let waker = waker_ref(&self.0);
            let context = &mut Context::from_waker(&waker);
            // `BoxFuture<T>` is a type alias for
            // `Pin<Box<dyn Future<Output = T> + Send + 'static>>`.
            // We can get a `Pin<&mut dyn Future + Send + 'static>`
            // from it by calling the `Pin::as_mut` method.
            if future.as_mut().poll(context).is_pending() {
                // We're not done processing the future, so put it
                // back in its task to be run again in the future.
                *future_slot = Some(future);
            }
        }
    }
}

// this is just a stub function to have cxx implement the UniquePtrTarget
// trait for TaskRunner
// TODO should pass runner as part of SDK initialization
fn init_runner(runner: UniquePtr<ffi::TaskRunner>) {
}

pub struct CppSDK {
    sdk: Rc<skus::sdk::SDK<NativeClient>>,
}

fn initialize_sdk(ctx: UniquePtr<ffi::SkusContext>, env: String) -> Box<CppSDK> {
    match tracing_subscriber::fmt()
        .event_format(log::CppFormatter::new())
        .with_max_level(tracing::Level::TRACE)
        .try_init()
    {
        Ok(_) => println!("tracing_subscriber - init success"),
        Err(_) => println!("tracing_subscriber - maybe already initialized"),
    };

    let env = env.parse::<skus::Environment>().unwrap_or(skus::Environment::Local);

    let executor = Rc::new(RefCell::new(NativeClientExecutor::new()));
    let sdk = skus::sdk::SDK::new(
        NativeClient {
            executor: executor.clone(),
            inner: Rc::new(Mutex::new(NativeClientInner {
                environment: env.clone(),
                executor,
                ctx: Rc::new(RefCell::new(ctx)),
            })),
        },
        env,
        None,
        None,
    );
    let sdk = Rc::new(sdk);
    {
        let sdk = sdk.clone();
        let spawner = sdk.client.get_spawner();
        let init = async move { sdk.initialize().await };
        if spawner.spawn_local(init).is_err() {
            debug!("pool is shutdown");
        }
    }

    sdk.client.try_run_until_stalled();

    Box::new(CppSDK { sdk })
}

impl CppSDK {
    fn shutdown(&self) {
        self.sdk.client.executor.borrow_mut().shutdown();
    }

    fn refresh_order(
        &self,
        callback: RefreshOrderCallback,
        callback_state: UniquePtr<ffi::RefreshOrderCallbackState>,
        order_id: String,
    ) {
        let spawner = self.sdk.client.get_spawner();
        if spawner
            .spawn_local(refresh_order_task(self.sdk.clone(), callback, callback_state, order_id))
            .is_err()
        {
            debug!("pool is shutdown");
        }

        self.sdk.client.try_run_until_stalled();
    }

    fn fetch_order_credentials(
        &self,
        callback: FetchOrderCredentialsCallback,
        callback_state: UniquePtr<ffi::FetchOrderCredentialsCallbackState>,
        order_id: String,
    ) {
        let spawner = self.sdk.client.get_spawner();
        if spawner
            .spawn_local(fetch_order_credentials_task(
                self.sdk.clone(),
                callback,
                callback_state,
                order_id,
            ))
            .is_err()
        {
            debug!("pool is shutdown");
        }

        self.sdk.client.try_run_until_stalled();
    }

    fn prepare_credentials_presentation(
        &self,
        callback: PrepareCredentialsPresentationCallback,
        callback_state: UniquePtr<ffi::PrepareCredentialsPresentationCallbackState>,
        domain: String,
        path: String,
    ) {
        let spawner = self.sdk.client.get_spawner();
        if spawner
            .spawn_local(prepare_credentials_presentation_task(
                self.sdk.clone(),
                callback,
                callback_state,
                domain,
                path,
            ))
            .is_err()
        {
            debug!("pool is shutdown");
        }

        self.sdk.client.try_run_until_stalled();
    }

    fn credential_summary(
        &self,
        callback: CredentialSummaryCallback,
        callback_state: UniquePtr<ffi::CredentialSummaryCallbackState>,
        domain: String,
    ) {
        let spawner = self.sdk.client.get_spawner();
        if spawner
            .spawn_local(credential_summary_task(
                self.sdk.clone(),
                callback,
                callback_state,
                domain,
            ))
            .is_err()
        {
            debug!("pool is shutdown");
        }

        self.sdk.client.try_run_until_stalled();
    }

    fn submit_receipt(
        self: &CppSDK,
        callback: SubmitReceiptCallback,
        callback_state: UniquePtr<ffi::SubmitReceiptCallbackState>,
        order_id: String,
        receipt: String,
    ) {
        let spawner = self.sdk.client.get_spawner();
        if spawner
            .spawn_local(submit_receipt_task(
                self.sdk.clone(),
                callback,
                callback_state,
                order_id,
                receipt,
            ))
            .is_err()
        {
            debug!("pool is shutdown");
        }

        self.sdk.client.try_run_until_stalled();
    }
    fn create_order_from_receipt(
        self: &CppSDK,
        callback: CreateOrderFromReceiptCallback,
        callback_state: UniquePtr<ffi::CreateOrderFromReceiptCallbackState>,
        receipt: String,
    ) {
        let spawner = self.sdk.client.get_spawner();
        if spawner
            .spawn_local(create_order_from_receipt_task(
                self.sdk.clone(),
                callback,
                callback_state,
                receipt,
            ))
            .is_err()
        {
            debug!("pool is shutdown");
        }

        self.sdk.client.try_run_until_stalled();
    }
}

#[allow(improper_ctypes_definitions)]
#[repr(transparent)]
pub struct RefreshOrderCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::RefreshOrderCallbackState,
        result: ffi::SkusResult,
        order: &str,
    ),
);

unsafe impl ExternType for RefreshOrderCallback {
    type Id = type_id!("skus::RefreshOrderCallback");
    type Kind = cxx::kind::Trivial;
}

async fn refresh_order_task(
    sdk: Rc<skus::sdk::SDK<NativeClient>>,
    callback: RefreshOrderCallback,
    callback_state: UniquePtr<ffi::RefreshOrderCallbackState>,
    order_id: String,
) {
    match sdk
        .refresh_order(&order_id)
        .await
        .and_then(|order| serde_json::to_string(&order).map_err(|e| e.into()))
        .map_err(|e| e.into())
    {
        Ok(order) => callback.0(callback_state.into_raw(), ffi::SkusResult::Ok, &order),
        Err(e) => callback.0(callback_state.into_raw(), e, ""),
    }
}

#[repr(transparent)]
pub struct FetchOrderCredentialsCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::FetchOrderCredentialsCallbackState,
        result: ffi::SkusResult,
    ),
);

unsafe impl ExternType for FetchOrderCredentialsCallback {
    type Id = type_id!("skus::FetchOrderCredentialsCallback");
    type Kind = cxx::kind::Trivial;
}

async fn fetch_order_credentials_task(
    sdk: Rc<skus::sdk::SDK<NativeClient>>,
    callback: FetchOrderCredentialsCallback,
    callback_state: UniquePtr<ffi::FetchOrderCredentialsCallbackState>,
    order_id: String,
) {
    match sdk.fetch_order_credentials(&order_id).await.map_err(|e| e.into()) {
        Ok(_) => callback.0(callback_state.into_raw(), ffi::SkusResult::Ok),
        Err(e) => callback.0(callback_state.into_raw(), e),
    }
}

#[allow(improper_ctypes_definitions)]
#[repr(transparent)]
pub struct PrepareCredentialsPresentationCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::PrepareCredentialsPresentationCallbackState,
        result: ffi::SkusResult,
        presentation: &str,
    ),
);

unsafe impl ExternType for PrepareCredentialsPresentationCallback {
    type Id = type_id!("skus::PrepareCredentialsPresentationCallback");
    type Kind = cxx::kind::Trivial;
}

async fn prepare_credentials_presentation_task(
    sdk: Rc<skus::sdk::SDK<NativeClient>>,
    callback: PrepareCredentialsPresentationCallback,
    callback_state: UniquePtr<ffi::PrepareCredentialsPresentationCallbackState>,
    domain: String,
    path: String,
) {
    match sdk.prepare_credentials_presentation(&domain, &path).await.map_err(|e| e.into()) {
        Ok(Some(presentation)) => {
            callback.0(callback_state.into_raw(), ffi::SkusResult::Ok, &presentation)
        }
        Ok(None) => callback.0(callback_state.into_raw(), ffi::SkusResult::Ok, ""),
        Err(e) => callback.0(callback_state.into_raw(), e, ""),
    }
}

#[allow(improper_ctypes_definitions)]
#[repr(transparent)]
pub struct CredentialSummaryCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::CredentialSummaryCallbackState,
        result: ffi::SkusResult,
        summary: &str,
    ),
);

unsafe impl ExternType for CredentialSummaryCallback {
    type Id = type_id!("skus::CredentialSummaryCallback");
    type Kind = cxx::kind::Trivial;
}

async fn credential_summary_task(
    sdk: Rc<skus::sdk::SDK<NativeClient>>,
    callback: CredentialSummaryCallback,
    callback_state: UniquePtr<ffi::CredentialSummaryCallbackState>,
    domain: String,
) {
    match sdk
        .matching_credential_summary(&domain)
        .await
        .and_then(|summary| {
            summary.map(|summary| serde_json::to_string(&summary).map_err(|e| e.into())).transpose()
        })
        .map_err(|e| e.into())
    {
        Ok(Some(summary)) => callback.0(callback_state.into_raw(), ffi::SkusResult::Ok, &summary),
        Ok(None) => callback.0(callback_state.into_raw(), ffi::SkusResult::Ok, "{}"), /* none, empty */
        Err(e) => callback.0(callback_state.into_raw(), e, "{}"),                     // none, empty
    }
}

#[repr(transparent)]
pub struct SubmitReceiptCallback(
    pub extern "C" fn(callback_state: *mut ffi::SubmitReceiptCallbackState, result: ffi::SkusResult),
);

unsafe impl ExternType for SubmitReceiptCallback {
    type Id = type_id!("skus::SubmitReceiptCallback");
    type Kind = cxx::kind::Trivial;
}

async fn submit_receipt_task(
    sdk: Rc<skus::sdk::SDK<NativeClient>>,
    callback: SubmitReceiptCallback,
    callback_state: UniquePtr<ffi::SubmitReceiptCallbackState>,
    order_id: String,
    receipt: String,
) {
    match sdk.submit_receipt(&order_id, &receipt).await.map_err(|e| e.into()) {
        Ok(_) => callback.0(callback_state.into_raw(), ffi::SkusResult::Ok),
        Err(e) => callback.0(callback_state.into_raw(), e),
    }
}

#[allow(improper_ctypes_definitions)]
#[repr(transparent)]
pub struct CreateOrderFromReceiptCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::CreateOrderFromReceiptCallbackState,
        result: ffi::SkusResult,
        order_id: &str,
    ),
);

unsafe impl ExternType for CreateOrderFromReceiptCallback {
    type Id = type_id!("skus::CreateOrderFromReceiptCallback");
    type Kind = cxx::kind::Trivial;
}

async fn create_order_from_receipt_task(
    sdk: Rc<skus::sdk::SDK<NativeClient>>,
    callback: CreateOrderFromReceiptCallback,
    callback_state: UniquePtr<ffi::CreateOrderFromReceiptCallbackState>,
    receipt: String,
) {
    match sdk.create_order_from_receipt(&receipt).await.map_err(|e| e.into()) {
        Ok(order_id) => callback.0(callback_state.into_raw(), ffi::SkusResult::Ok, &order_id),
        Err(e) => callback.0(callback_state.into_raw(), e, ""),
    }
}
