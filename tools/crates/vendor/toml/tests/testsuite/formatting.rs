use serde::Deserialize;
use serde::Serialize;
use toml::to_string;

#[derive(Debug, Clone, Hash, PartialEq, Eq, Serialize, Deserialize)]
struct User {
    pub(crate) name: String,
    pub(crate) surname: String,
}

#[derive(Debug, Clone, Hash, PartialEq, Eq, Serialize, Deserialize)]
struct Users {
    pub(crate) user: Vec<User>,
}

#[derive(Debug, Clone, Hash, PartialEq, Eq, Serialize, Deserialize)]
struct TwoUsers {
    pub(crate) user0: User,
    pub(crate) user1: User,
}

#[test]
fn no_unnecessary_newlines_array() {
    assert!(!to_string(&Users {
        user: vec![
            User {
                name: "John".to_owned(),
                surname: "Doe".to_owned(),
            },
            User {
                name: "Jane".to_owned(),
                surname: "Dough".to_owned(),
            },
        ],
    })
    .unwrap()
    .starts_with('\n'));
}

#[test]
fn no_unnecessary_newlines_table() {
    assert!(!to_string(&TwoUsers {
        user0: User {
            name: "John".to_owned(),
            surname: "Doe".to_owned(),
        },
        user1: User {
            name: "Jane".to_owned(),
            surname: "Dough".to_owned(),
        },
    })
    .unwrap()
    .starts_with('\n'));
}
