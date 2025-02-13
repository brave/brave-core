use serde::Deserialize;
use serde::Serialize;

#[derive(Debug, Serialize, Deserialize)]
pub(crate) struct Recipe {
    pub(crate) name: String,
    pub(crate) description: Option<String>,
    #[serde(default)]
    pub(crate) modules: Vec<Modules>,
    #[serde(default)]
    pub(crate) packages: Vec<Packages>,
}

#[derive(Debug, Serialize, Deserialize)]
pub(crate) struct Modules {
    pub(crate) name: String,
    pub(crate) version: Option<String>,
}

#[derive(Debug, Serialize, Deserialize)]
pub(crate) struct Packages {
    pub(crate) name: String,
    pub(crate) version: Option<String>,
}

#[test]
fn both_ends() {
    let recipe_works = toml::from_str::<Recipe>(
        r#"
        name = "testing"
        description = "example"
        modules = []

        [[packages]]
        name = "base"
    "#,
    )
    .unwrap();
    toml::to_string(&recipe_works).unwrap();

    let recipe_fails = toml::from_str::<Recipe>(
        r#"
        name = "testing"
        description = "example"
        packages = []

        [[modules]]
        name = "base"
    "#,
    )
    .unwrap();

    let recipe_toml = toml::Table::try_from(recipe_fails).unwrap();
    recipe_toml.to_string();
}
