use std::io::Write;
use diesel::expression::{AsExpression, Expression};
use diesel::pg::Pg;
use diesel::serialize::{self, IsNull, Output, ToSql};
use diesel::deserialize::{self, FromSql};
use diesel::sql_types::Cidr;
use crate::{IpNetwork, Ipv4Network, Ipv6Network};
use crate::postgres_common;

impl FromSql<Cidr, Pg> for Ipv4Network {
    fn from_sql(bytes: Option<&[u8]>) -> deserialize::Result<Self> {
        let bytes = not_none!(bytes);
        postgres_common::from_sql_ipv4_network(bytes)
    }
}

impl FromSql<Cidr, Pg> for Ipv6Network {
    fn from_sql(bytes: Option<&[u8]>) -> deserialize::Result<Self> {
        let bytes = not_none!(bytes);
        postgres_common::from_sql_ipv6_network(bytes)
    }
}

impl FromSql<Cidr, Pg> for IpNetwork {
    fn from_sql(bytes: Option<&[u8]>) -> deserialize::Result<Self> {
        let bytes = not_none!(bytes);
        match bytes[0] {
            postgres_common::IPV4_TYPE => Ok(IpNetwork::V4(
                postgres_common::from_sql_ipv4_network(bytes)?,
            )),
            postgres_common::IPV6_TYPE => Ok(IpNetwork::V6(
                postgres_common::from_sql_ipv6_network(bytes)?,
            )),
            _ => Err("CIDR is not IP version 4 or 6".into()),
        }
    }
}

impl ToSql<Cidr, Pg> for Ipv4Network {
    fn to_sql<W: Write>(&self, out: &mut Output<W, Pg>) -> serialize::Result {
        let data = postgres_common::to_sql_ipv4_network(self);
        out.write_all(&data).map(|_| IsNull::No).map_err(Into::into)
    }
}

impl ToSql<Cidr, Pg> for Ipv6Network {
    fn to_sql<W: Write>(&self, out: &mut Output<W, Pg>) -> serialize::Result {
        let data = postgres_common::to_sql_ipv6_network(self);
        out.write_all(&data).map(|_| IsNull::No).map_err(Into::into)
    }
}

impl ToSql<Cidr, Pg> for IpNetwork {
    fn to_sql<W: Write>(&self, out: &mut Output<W, Pg>) -> serialize::Result {
        match self {
            IpNetwork::V4(network) => ToSql::<Cidr, Pg>::to_sql(network, out),
            IpNetwork::V6(network) => ToSql::<Cidr, Pg>::to_sql(network, out),
        }
    }
}

#[allow(dead_code)]
mod foreign_derives {
    use super::*;

    #[derive(FromSqlRow, AsExpression)]
    #[diesel(foreign_derive)]
    #[sql_type = "Cidr"]
    struct IpNetworkProxy(IpNetwork);

    #[derive(FromSqlRow, AsExpression)]
    #[diesel(foreign_derive)]
    #[sql_type = "Cidr"]
    struct Ipv4NetworkProxy(Ipv4Network);

    #[derive(FromSqlRow, AsExpression)]
    #[diesel(foreign_derive)]
    #[sql_type = "Cidr"]
    struct Ipv6NetworkProxy(Ipv6Network);
}

diesel_infix_operator!(IsContainedBy, " << ", backend: Pg);
diesel_infix_operator!(IsContainedByOrEquals, " <<= ", backend: Pg);
diesel_infix_operator!(Contains, " >> ", backend: Pg);
diesel_infix_operator!(ContainsOrEquals, " >>= ", backend: Pg);
diesel_infix_operator!(ContainsOrIsContainedBy, " && ", backend: Pg);

/// Support for PostgreSQL Network Address Operators for Diesel
///
/// See [PostgreSQL documentation for details](https://www.postgresql.org/docs/current/static/functions-net.html).
pub trait PqCidrExtensionMethods: Expression<SqlType = Cidr> + Sized {
    /// Creates a SQL `<<` expression.
    fn is_contained_by<T>(self, other: T) -> IsContainedBy<Self, T::Expression>
    where
        T: AsExpression<Self::SqlType>,
    {
        IsContainedBy::new(self, other.as_expression())
    }

    /// Creates a SQL `<<=` expression.
    fn is_contained_by_or_equals<T>(self, other: T) -> IsContainedByOrEquals<Self, T::Expression>
    where
        T: AsExpression<Self::SqlType>,
    {
        IsContainedByOrEquals::new(self, other.as_expression())
    }

    /// Creates a SQL `>>` expression.
    fn contains<T>(self, other: T) -> Contains<Self, T::Expression>
    where
        T: AsExpression<Self::SqlType>,
    {
        Contains::new(self, other.as_expression())
    }

    /// Creates a SQL `>>=` expression.
    fn contains_or_equals<T>(self, other: T) -> ContainsOrEquals<Self, T::Expression>
    where
        T: AsExpression<Self::SqlType>,
    {
        ContainsOrEquals::new(self, other.as_expression())
    }

    /// Creates a SQL `&&` expression.
    fn contains_or_is_contained_by<T>(
        self,
        other: T,
    ) -> ContainsOrIsContainedBy<Self, T::Expression>
    where
        T: AsExpression<Self::SqlType>,
    {
        ContainsOrIsContainedBy::new(self, other.as_expression())
    }
}

impl<T> PqCidrExtensionMethods for T where T: Expression<SqlType = Cidr> {}

/// CIDR functions.
pub mod functions {
    use diesel::sql_types::Cidr;

    sql_function! {
        /// Extract family of address; 4 for IPv4, 6 for IPv6.
        fn family(x: Cidr) -> Integer;
    }
    sql_function! {
        /// Extract netmask length.
        fn masklen(x: Cidr) -> Integer;
    }
}

pub mod helper_types {
    pub type Family<Expr> = super::functions::family::HelperType<Expr>;
    pub type Masklen<Expr> = super::functions::masklen::HelperType<Expr>;
}

pub mod dsl {
    pub use super::functions::*;
    pub use super::helper_types::*;
}

#[cfg(test)]
mod tests {
    use std::net::{Ipv4Addr, Ipv6Addr};
    use diesel::sql_types::Cidr;
    use diesel::pg::Pg;
    use diesel::serialize::{Output, ToSql};
    use diesel::deserialize::FromSql;
    use diesel::prelude::*;
    use diesel::debug_query;
    use super::PqCidrExtensionMethods;
    use super::{IpNetwork, Ipv4Network, Ipv6Network};
    use super::dsl::*;

    table! {
        test {
            id -> Integer,
            ip_network -> Cidr,
            ipv4_network -> Cidr,
            ipv6_network -> Cidr,
        }
    }

    #[derive(Insertable)]
    #[table_name = "test"]
    pub struct NewPost {
        pub id: i32,
        pub ip_network: IpNetwork,
        pub ipv4_network: Ipv4Network,
        pub ipv6_network: Ipv6Network,
    }

    fn test_output() -> Output<'static, Vec<u8>, Pg> {
        let uninit = std::mem::MaybeUninit::uninit();
        let fake_metadata_lookup = unsafe { uninit.assume_init() };
        Output::new(Vec::new(), fake_metadata_lookup)
    }

    #[test]
    fn ipv4_network() {
        let mut bytes = test_output();
        let ipv4_network = Ipv4Network::new(Ipv4Addr::new(1, 2, 3, 4), 32).unwrap();
        ToSql::<Cidr, Pg>::to_sql(&ipv4_network, &mut bytes).unwrap();
        let converted: Ipv4Network = FromSql::<Cidr, Pg>::from_sql(Some(bytes.as_ref())).unwrap();
        assert_eq!(ipv4_network, converted);
    }

    #[test]
    fn ipv6_network() {
        let mut bytes = test_output();
        let ipv6_network = Ipv6Network::new(Ipv6Addr::new(1, 2, 3, 4, 5, 6, 7, 8), 128).unwrap();
        ToSql::<Cidr, Pg>::to_sql(&ipv6_network, &mut bytes).unwrap();
        let converted: Ipv6Network = FromSql::<Cidr, Pg>::from_sql(Some(bytes.as_ref())).unwrap();
        assert_eq!(ipv6_network, converted);
    }

    #[test]
    fn ip_network() {
        let mut bytes = test_output();
        let ip_network =
            IpNetwork::V6(Ipv6Network::new(Ipv6Addr::new(1, 2, 3, 4, 5, 6, 7, 8), 128).unwrap());
        ToSql::<Cidr, Pg>::to_sql(&ip_network, &mut bytes).unwrap();
        let converted: IpNetwork = FromSql::<Cidr, Pg>::from_sql(Some(bytes.as_ref())).unwrap();
        assert_eq!(ip_network, converted);
    }

    #[test]
    fn operators() {
        let ip = IpNetwork::new(Ipv4Addr::new(127, 0, 0, 1), 32).unwrap();
        test::ip_network.is_contained_by(&ip);
        test::ip_network.is_contained_by_or_equals(&ip);
        test::ip_network.contains(&ip);
        test::ip_network.contains_or_equals(&ip);
        test::ip_network.contains_or_is_contained_by(&ip);
    }

    #[test]
    fn function_family() {
        let query = test::table.select(family(test::ip_network));
        let string_query = debug_query::<Pg, _>(&query).to_string();
        assert_eq!(
            "SELECT family(\"test\".\"ip_network\") FROM \"test\" -- binds: []",
            string_query
        );
    }

    #[test]
    fn function_masklen() {
        let query = test::table.select(masklen(test::ip_network));
        let string_query = debug_query::<Pg, _>(&query).to_string();
        assert_eq!(
            "SELECT masklen(\"test\".\"ip_network\") FROM \"test\" -- binds: []",
            string_query
        );
    }
}
