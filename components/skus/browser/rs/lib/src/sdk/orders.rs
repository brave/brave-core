use core::convert::{TryFrom, TryInto};

use chrono::{DateTime, Utc};
use futures_retry::FutureRetry;
use serde::{Deserialize, Serialize};
use tracing::instrument;

#[cfg(feature = "e2e_test")]
use serde_json::{json, to_vec};

use crate::errors::{InternalError, SkusError};
use crate::http::HttpHandler;
use crate::models::*;
use crate::sdk::SDK;
use crate::{HTTPClient, StorageClient};

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct OrderMetadataResponse {
    stripe_checkout_session_id: Option<String>,
}

impl From<OrderMetadataResponse> for OrderMetadata {
    fn from(order_metadata: OrderMetadataResponse) -> Self {
        OrderMetadata {
            stripe_checkout_session_id: order_metadata.stripe_checkout_session_id,
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct OrderItemResponse {
    id: String,
    order_id: String,
    sku: String,
    created_at: DateTime<Utc>,
    updated_at: DateTime<Utc>,
    currency: String,
    quantity: i32,
    price: String,
    subtotal: String,
    location: String,
    description: String,
    credential_type: CredentialType,
    // TODO add credential_version
    // TODO add accepted payment types
}

impl TryFrom<OrderItemResponse> for OrderItem {
    type Error = SkusError;

    fn try_from(order_item: OrderItemResponse) -> Result<Self, Self::Error> {
        let price = order_item.price.parse::<f64>().map_err(|_| {
            InternalError::InvalidResponse("Could not parse unit price".to_string())
        })?;
        let subtotal = order_item
            .subtotal
            .parse::<f64>()
            .map_err(|_| InternalError::InvalidResponse("Could not parse subtotal".to_string()))?;
        Ok(OrderItem {
            id: order_item.id,
            order_id: order_item.order_id,
            sku: order_item.sku,
            created_at: order_item.created_at.naive_utc(),
            updated_at: order_item.updated_at.naive_utc(),
            currency: order_item.currency,
            quantity: order_item.quantity,
            price,
            subtotal,
            location: order_item.location,
            description: order_item.description,
            credential_type: order_item.credential_type,
            //valid_for: order_item.valid_for,
        })
    }
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct OrderResponse {
    id: String,
    created_at: DateTime<Utc>,
    currency: String,
    updated_at: DateTime<Utc>,
    total_price: String,
    location: String,
    merchant_id: String,
    status: String,
    items: Vec<OrderItemResponse>,
    metadata: Option<OrderMetadataResponse>,
    expires_at: Option<DateTime<Utc>>,
    last_paid_at: Option<DateTime<Utc>>,
}

impl TryFrom<OrderResponse> for Order {
    type Error = SkusError;

    fn try_from(order: OrderResponse) -> Result<Self, Self::Error> {
        let total_price = order.total_price.parse::<f64>().map_err(|_| {
            InternalError::InvalidResponse("Could not parse total price".to_string())
        })?;
        let items: Result<Vec<OrderItem>, _> = order
            .items
            .into_iter()
            .map(|item| item.try_into())
            .collect();
        Ok(Order {
            id: order.id,
            created_at: order.created_at.naive_utc(),
            currency: order.currency,
            updated_at: order.updated_at.naive_utc(),
            total_price,
            location: order.location,
            merchant_id: order.merchant_id,
            status: order.status,
            items: items?,
            metadata: order.metadata.map(|m| m.into()),
            expires_at: order.expires_at.map(|ex| ex.naive_utc()),
            last_paid_at: order.last_paid_at.map(|lp| lp.naive_utc()),
            //valid_for: order.valid_for,
        })
    }
}

impl<U> SDK<U>
where
    U: HTTPClient + StorageClient,
{
    #[cfg(feature = "e2e_test")]
    pub async fn create_order(&self, kind: &str) -> Result<Order, SkusError> {
        let sku = match kind {
            "trial" => "AgEVc2VhcmNoLmJyYXZlLnNvZnR3YXJlAh9zZWFyY2ggY2xvc2VkIGJldGEgcHJvZ3JhbSBkZW1vAAIWc2t1PXNlYXJjaC1iZXRhLWFjY2VzcwACB3ByaWNlPTAAAgxjdXJyZW5jeT1CQVQAAi1kZXNjcmlwdGlvbj1TZWFyY2ggY2xvc2VkIGJldGEgcHJvZ3JhbSBhY2Nlc3MAAhpjcmVkZW50aWFsX3R5cGU9c2luZ2xlLXVzZQAABiB3uXfAAkNSRQd24jSauRny3VM0BYZ8yOclPTEgPa0xrA==",
            "paid" => "MDAyOWxvY2F0aW9uIHRvZ2V0aGVyLmJzZy5icmF2ZS5zb2Z0d2FyZQowMDMwaWRlbnRpZmllciBicmF2ZS10b2dldGhlci1wYWlkIHNrdSB0b2tlbiB2MQowMDIwY2lkIHNrdT1icmF2ZS10b2dldGhlci1wYWlkCjAwMTBjaWQgcHJpY2U9NQowMDE1Y2lkIGN1cnJlbmN5PVVTRAowMDQzY2lkIGRlc2NyaXB0aW9uPU9uZSBtb250aCBwYWlkIHN1YnNjcmlwdGlvbiBmb3IgQnJhdmUgVG9nZXRoZXIKMDAyNWNpZCBjcmVkZW50aWFsX3R5cGU9dGltZS1saW1pdGVkCjAwMjZjaWQgY3JlZGVudGlhbF92YWxpZF9kdXJhdGlvbj1QMU0KMDAyZnNpZ25hdHVyZSDKLJ7NuuzP3KdmTdVnn0dI3JmIfNblQKmY+WBJOqnQJAo=",
            "beta" => "AgEVc2VhcmNoLmJyYXZlLnNvZnR3YXJlAh9zZWFyY2ggY2xvc2VkIGJldGEgcHJvZ3JhbSBkZW1vAAIWc2t1PXNlYXJjaC1iZXRhLWFjY2VzcwACB3ByaWNlPTAAAgxjdXJyZW5jeT1CQVQAAi1kZXNjcmlwdGlvbj1TZWFyY2ggY2xvc2VkIGJldGEgcHJvZ3JhbSBhY2Nlc3MAAhpjcmVkZW50aWFsX3R5cGU9c2luZ2xlLXVzZQAABiB3uXfAAkNSRQd24jSauRny3VM0BYZ8yOclPTEgPa0xrA==",
            _ => "",
        };

        let quantity = match kind {
            "beta" => 10,
            _ => 1,
        };

        let order = json!({
            "items": [
                {
                    "sku": sku,
                    "quantity": quantity
                }
           ]
        });

        let request_with_retries = FutureRetry::new(
            || async {
                let mut builder = http::Request::builder();
                builder.method("POST");
                builder.uri(format!("{}/v1/orders", self.base_url));

                let body = to_vec(&order).or(Err(InternalError::SerializationFailed))?;

                let req = builder.body(body).unwrap();
                let resp = self.fetch(req).await?;

                match resp.status() {
                    http::StatusCode::CREATED => Ok(resp),
                    _ => Err(resp.into()),
                }
            },
            HttpHandler::new(3, "Create order request", &self.client),
        );
        let (resp, _) = request_with_retries.await?;

        let order: OrderResponse = serde_json::from_slice(resp.body())?;
        let order: Order = order.try_into()?;

        self.client.upsert_order(&order).await?;

        Ok(order)
    }

    #[instrument]
    pub async fn fetch_order(&self, order_id: &str) -> Result<Order, SkusError> {
        let request_with_retries = FutureRetry::new(
            || async {
                let mut builder = http::Request::builder();
                builder.method("GET");
                builder.uri(format!("{}/v1/orders/{}", self.base_url, order_id));

                let req = builder.body(vec![]).unwrap();
                let resp = self.fetch(req).await?;

                match resp.status() {
                    http::StatusCode::OK => Ok(resp),
                    http::StatusCode::NOT_FOUND => Err(InternalError::NotFound),
                    _ => Err(resp.into()),
                }
            },
            HttpHandler::new(3, "Fetch order request", &self.client),
        );
        let (resp, _) = request_with_retries.await?;

        let order: OrderResponse = serde_json::from_slice(resp.body())?;
        let order: Order = order.try_into()?;

        Ok(order)
    }

    #[instrument]
    pub async fn refresh_order(&self, order_id: &str) -> Result<Order, SkusError> {
        let order = self.fetch_order(order_id).await?;
        self.client.upsert_order(&order).await?;
        Ok(order)
    }
}
