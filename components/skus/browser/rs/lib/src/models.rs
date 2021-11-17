use std::fmt::{self, Display};
use std::str::FromStr;

use challenge_bypass_ristretto::voprf::*;
use chrono::prelude::*;
use serde::{Deserialize, Serialize};

use crate::errors::*;

#[derive(Debug, Clone, PartialEq)]
pub enum Environment {
    Local,
    Testing,
    Development,
    Staging,
    Production,
}

impl Display for Environment {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Environment::Local => write!(f, "local"),
            Environment::Testing => write!(f, "testing"),
            Environment::Development => write!(f, "development"),
            Environment::Staging => write!(f, "staging"),
            Environment::Production => write!(f, "production"),
        }
    }
}

impl FromStr for Environment {
    type Err = InternalError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "local" => Ok(Self::Local),
            "testing" => Ok(Self::Testing),
            "development" => Ok(Self::Development),
            "staging" => Ok(Self::Staging),
            "production" => Ok(Self::Production),
            _ => Err(InternalError::UnhandledVariant),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Wallet {
    pub id: String,
    pub environment: String,
    pub wallet_type: String,
    pub seed: Vec<u8>,
}

impl Wallet {
    pub fn new(id: &str, environment: &str, wallet_type: &str, seed: [u8; 64]) -> Wallet {
        Wallet {
            id: id.to_string(),
            environment: environment.to_string(),
            wallet_type: wallet_type.to_string(),
            seed: seed.to_vec(),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Promotion {
    pub id: String,
    pub approximate_value: f64,
    pub available: bool,
    pub created_at: NaiveDateTime,
    pub expires_at: NaiveDateTime,
    pub legacy_claimed: bool,
    pub platform: String,
    pub suggestions_per_grant: i32,
    pub promotion_type: String,
    pub version: i32,
    pub suggestion_value: f64,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Claim {
    pub promotion_id: String,
    pub tokens: String,
    pub claim_id: String,
    pub complete: bool,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct UnredeemedToken {
    pub token: String,
    pub unblinded_token: String,
    pub public_key: String,
    pub promotion_id: String,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "kebab-case")]
pub enum CredentialType {
    SingleUse,
    TimeLimited,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct OrderMetadata {
    pub stripe_checkout_session_id: Option<String>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct OrderItem {
    pub id: String,
    pub order_id: String,
    pub sku: String,
    pub created_at: NaiveDateTime,
    pub updated_at: NaiveDateTime,
    pub currency: String,
    pub quantity: i32,
    pub price: f64,
    pub subtotal: f64,
    pub location: String,
    pub description: String,
    pub credential_type: CredentialType,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct CredentialSummary {
    pub item: OrderItem,
    pub remaining_credential_count: u32,
    pub expires_at: Option<NaiveDateTime>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Order {
    pub id: String,
    pub created_at: NaiveDateTime,
    pub currency: String,
    pub updated_at: NaiveDateTime,
    pub total_price: f64,
    pub location: String,
    pub merchant_id: String,
    pub status: String,
    pub items: Vec<OrderItem>,
    pub metadata: Option<OrderMetadata>,
    pub expires_at: Option<NaiveDateTime>,
    pub last_paid_at: Option<NaiveDateTime>,
}

impl Order {
    pub fn location_matches(&self, environment: &Environment, domain: &str) -> bool {
        if *environment != Environment::Local && *environment != Environment::Testing {
            // domain we are comparing with must be the location or a subdomain of it
            if domain
                .strip_suffix(&self.location)
                .filter(|s| s.is_empty() || s.ends_with('.'))
                .is_none()
            {
                return false;
            }
        }
        true
    }

    pub fn is_paid(&self) -> bool {
        self.status == "paid" || (self.status == "canceled" && self.expires_at.is_some())
    }
}

#[derive(Serialize, Deserialize, Debug)]
pub struct SingleUseCredential {
    pub unblinded_cred: UnblindedToken,
    pub spent: bool,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct SingleUseCredentials {
    pub item_id: String,
    pub creds: Vec<Token>,
    pub unblinded_creds: Option<Vec<SingleUseCredential>>,
    pub issuer_id: Option<String>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct TimeLimitedCredential {
    pub item_id: String,
    pub issued_at: NaiveDateTime,
    pub expires_at: NaiveDateTime,
    pub token: String,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct TimeLimitedCredentials {
    pub item_id: String,
    pub creds: Vec<TimeLimitedCredential>,
}
