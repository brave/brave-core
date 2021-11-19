#![allow(unused_variables)]

use std::cell::{RefCell, RefMut};
use std::fmt;
use std::rc::Rc;

use async_trait::async_trait;

use wasm_bindgen::prelude::*;
use wasm_bindgen::JsCast;
use wasm_bindgen_futures::futures_0_3::{future_to_promise, JsFuture};
use web_sys::{HtmlDocument, Request, RequestInit, RequestMode, Response};

use tracing_subscriber::{filter::LevelFilter, layer::SubscriberExt, registry::Registry};
use tracing_wasm::{WASMLayer, WASMLayerConfig};

use skus::{errors, http::http, sdk, tracing, Environment, HTTPClient, KVClient, KVStore};

#[wasm_bindgen]
extern "C" {
    #[wasm_bindgen(js_name = Headers)]
    type Headers;

    #[wasm_bindgen(method, js_name = entries)]
    fn entries(this: &Headers) -> js_sys::Iterator;
}

// TODO try to update to wasm-bindgen-futures 0.4

#[wasm_bindgen]
pub fn initialize(
    env: String,
    remote_sdk: Option<bool>,
    log_level: Option<String>,
) -> js_sys::Promise {
    let future = async move {
        console_error_panic_hook::set_once();

        let env = env.parse::<Environment>().or(Err("invalid environment"))?;

        let level_filter = match log_level {
            Some(log_level) => log_level
                .parse::<LevelFilter>()
                .or(Err("invalid log level"))?,
            None => LevelFilter::DEBUG,
        };

        skus::tracing::subscriber::set_global_default(
            Registry::default()
                .with(WASMLayer::new(WASMLayerConfig::default()))
                .with(level_filter),
        )
        .expect("default global");

        if let Some(remote_sdk) = remote_sdk {
            if remote_sdk {
                tracing::warn!("remote sdk has been deprecated");
            }
        }

        let window = web_sys::window().ok_or("couldn't get window")?;
        if let Ok(Some(local_storage)) = window.local_storage() {
            let sdk = sdk::SDK::new(
                JSClient {
                    local_storage: Rc::new(RefCell::new(JSStorage {
                        environment: env.clone(),
                        store: local_storage,
                    })),
                },
                env,
                None,
                None,
            );
            sdk.initialize().await;

            Ok(JSSDK { sdk: Rc::new(sdk) }.into())
        } else {
            Err("couldn't find store".into())
        }
    };

    future_to_promise(future)
}

pub struct JSStorage {
    environment: Environment,
    store: web_sys::Storage,
}

#[derive(Clone)]
pub struct JSClient {
    local_storage: Rc<RefCell<JSStorage>>,
}

impl fmt::Debug for JSClient {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("JSClient").finish()
    }
}

#[wasm_bindgen]
pub struct JSSDK {
    sdk: Rc<sdk::SDK<JSClient>>,
}

#[wasm_bindgen]
impl JSSDK {
    #[wasm_bindgen]
    pub fn refresh_order(&self, order_id: String) -> js_sys::Promise {
        let sdk = self.sdk.clone();

        let future = async move {
            let order = sdk
                .refresh_order(&order_id)
                .await
                .map_err(|e| e.to_string())?;
            Ok(JsValue::from_serde(&order).unwrap())
        };

        future_to_promise(future)
    }

    #[wasm_bindgen]
    pub fn submit_order_credentials_to_sign(&self, order_id: String) -> js_sys::Promise {
        let sdk = self.sdk.clone();

        let future = async move {
            let order = sdk
                .submit_order_credentials_to_sign(&order_id)
                .await
                .map_err(|e| e.to_string())?;
            Ok(JsValue::from_serde(&order).unwrap())
        };

        future_to_promise(future)
    }

    #[wasm_bindgen]
    pub fn fetch_order_credentials(&self, order_id: String) -> js_sys::Promise {
        let sdk = self.sdk.clone();

        let future = async move {
            let order = sdk
                .fetch_order_credentials(&order_id)
                .await
                .map_err(|e| e.to_string())?;
            Ok(JsValue::from_serde(&order).unwrap())
        };

        future_to_promise(future)
    }

    #[wasm_bindgen]
    pub fn delete_order_credentials(&self, order_id: String) -> js_sys::Promise {
        let sdk = self.sdk.clone();

        let future = async move {
            let order = sdk
                .delete_order_credentials(&order_id)
                .await
                .map_err(|e| e.to_string())?;
            Ok(JsValue::from_serde(&order).unwrap())
        };

        future_to_promise(future)
    }

    #[wasm_bindgen]
    pub fn credential_summary(&self, subdomain: Option<String>) -> js_sys::Promise {
        let sdk = self.sdk.clone();

        let future = async move {
            let window = web_sys::window().ok_or("couldn't get window")?;
            let host = window.location().hostname()?;

            if let Some(credential_summary) = sdk
                .matching_credential_summary(&subdomain.unwrap_or(host))
                .await
                .map_err(|e| e.to_string())?
            {
                return Ok(JsValue::from_serde(&credential_summary).unwrap());
            }

            Ok(JsValue::UNDEFINED)
        };

        future_to_promise(future)
    }

    #[wasm_bindgen]
    pub fn order_credential_summary(
        &self,
        order_id: String,
        subdomain: Option<String>,
    ) -> js_sys::Promise {
        let sdk = self.sdk.clone();

        let future = async move {
            let window = web_sys::window().ok_or("couldn't get window")?;
            let host = window.location().hostname()?;

            if let Some(credential_summary) = sdk
                .matching_order_credential_summary(&order_id, &subdomain.unwrap_or(host))
                .await
                .map_err(|e| e.to_string())?
            {
                return Ok(JsValue::from_serde(&credential_summary).unwrap());
            }

            Ok(JsValue::UNDEFINED)
        };

        future_to_promise(future)
    }

    #[wasm_bindgen]
    pub fn prepare_credentials_presentation(
        &self,
        origin: String,
        path: Option<String>,
    ) -> js_sys::Promise {
        let sdk = self.sdk.clone();

        let future = async move {
            let presentation = sdk
                .prepare_credentials_presentation(&origin, &path.unwrap_or_else(|| "/".to_string()))
                .await
                .map_err(|e| e.to_string())?;
            Ok(JsValue::from_serde(&presentation).unwrap())
        };

        future_to_promise(future)
    }

    #[wasm_bindgen]
    pub fn present_order_credentials(
        &self,
        order_id: String,
        subdomain: Option<String>,
        path: Option<String>,
    ) -> js_sys::Promise {
        let sdk = self.sdk.clone();

        let future = async move {
            let window = web_sys::window().ok_or("couldn't get window")?;
            let host = window.location().hostname()?;

            sdk.present_order_credentials(
                &order_id,
                &subdomain.unwrap_or(host),
                &path.unwrap_or_else(|| "/".to_string()),
            )
            .await
            .map_err(|e| e.to_string())?;
            Ok(JsValue::UNDEFINED)
        };

        future_to_promise(future)
    }

    #[wasm_bindgen]
    pub fn present_credentials(
        &self,
        subdomain: Option<String>,
        path: Option<String>,
    ) -> js_sys::Promise {
        let sdk = self.sdk.clone();

        let future = async move {
            let window = web_sys::window().ok_or("couldn't get window")?;
            let host = window.location().hostname()?;

            if let Some(cred) = sdk
                .present_credentials(
                    &subdomain.unwrap_or(host),
                    &path.unwrap_or_else(|| "/".to_string()),
                )
                .await
                .map_err(|e| e.to_string())?
            {
                return Ok(JsValue::from_str(&cred));
            }

            Ok(JsValue::UNDEFINED)
        };

        future_to_promise(future)
    }
}

impl KVClient for JSClient {
    type Store = JSStorage;

    #[allow(clippy::needless_lifetimes)]
    fn get_store<'a>(&'a self) -> Result<RefMut<'a, JSStorage>, errors::InternalError> {
        self.local_storage
            .try_borrow_mut()
            .or(Err(errors::InternalError::BorrowFailed))
    }
}

impl KVStore for JSStorage {
    fn env(&self) -> &Environment {
        &self.environment
    }

    fn purge(&mut self) -> Result<(), errors::InternalError> {
        self.store.clear().map_err(|e| {
            errors::InternalError::StorageWriteFailed(
                e.as_string().unwrap_or_else(|| "unknown error".to_string()),
            )
        })
    }

    fn set(&mut self, key: &str, value: &str) -> Result<(), errors::InternalError> {
        self.store.set(key, value).map_err(|e| {
            errors::InternalError::StorageWriteFailed(
                e.as_string().unwrap_or_else(|| "unknown error".to_string()),
            )
        })
    }

    fn get(&mut self, key: &str) -> Result<Option<String>, errors::InternalError> {
        self.store.get(key).map_err(|e| {
            errors::InternalError::StorageReadFailed(
                e.as_string().unwrap_or_else(|| "unknown error".to_string()),
            )
        })
    }
}

#[async_trait(?Send)]
impl HTTPClient for JSClient {
    fn set_cookie(&self, value: &str) {
        let document = web_sys::window()
            .unwrap()
            .document()
            .unwrap()
            .dyn_into::<HtmlDocument>()
            .unwrap();

        tracing::debug!("set cookie: {}", value);
        document.set_cookie(value).unwrap();
    }

    fn get_cookie(&self, name: &str) -> Option<String> {
        wasm_cookies::get_raw(name)
    }

    fn schedule_wakeup(&self, _: u64) {
        // no need to wakeup on JS :)
    }

    async fn execute(
        &self,
        req: http::Request<Vec<u8>>,
    ) -> Result<http::Response<Vec<u8>>, errors::InternalError> {
        let window = web_sys::window().unwrap();

        let mut opts = RequestInit::new();
        opts.mode(RequestMode::Cors);

        opts.method(&req.method().to_string());

        let body = req.body();

        if !body.is_empty() {
            let typebuf: js_sys::Uint8Array = body.as_slice().into();
            opts.body(Some(&typebuf));
        }

        let request = Request::new_with_str_and_init(&req.uri().to_string(), &opts).unwrap();

        for (key, value) in req.headers().iter() {
            request
                .headers()
                .set(key.as_str(), value.to_str().unwrap())
                .unwrap();
        }

        let r = JsFuture::from(window.fetch_with_request(&request))
            .await
            .or(Err(errors::InternalError::RequestFailed))?;

        assert!(r.is_instance_of::<Response>());
        let r: Response = r.dyn_into().unwrap();

        let mut response = http::Response::builder();
        response.status(r.status());

        let headers: Headers = r.headers().dyn_into().unwrap();

        for value in headers.entries() {
            let header: (String, String) = value.unwrap().into_serde().unwrap();
            response.header(&header.0 as &str, &header.1 as &str);
        }

        let body = JsFuture::from(r.array_buffer().unwrap()).await;

        let typebuf: js_sys::Uint8Array = js_sys::Uint8Array::new(&body.unwrap());
        let mut body = vec![0; typebuf.length() as usize];
        typebuf.copy_to(&mut body);
        Ok(response.body(body).map_err(|_| {
            errors::InternalError::InvalidResponse("error getting body".to_string())
        })?)
    }
}
