//! Tests for Abscissa's component functionality

mod example_app;

use self::example_app::{ExampleApp, ExampleConfig};
use abscissa_core::{component, Component, FrameworkError, FrameworkErrorKind::ComponentError};

/// ID for `FoobarComponent` (example component #1)
const FOOBAR_COMPONENT_ID: component::Id = component::Id::new("component::FoobarComponent");

/// ID for `BazComponent` (example component #2)
const BAZ_COMPONENT_ID: component::Id = component::Id::new("component::BazComponent");

/// ID for `QuuxComponent` (example component #3)
const QUUX_COMPONENT_ID: component::Id = component::Id::new("component::QuuxComponent");

/// Example component #1
#[derive(Component, Debug, Default)]
pub struct FoobarComponent {
    /// Component state
    pub state: Option<String>,
}

impl FoobarComponent {
    /// Set the state string to a particular value
    pub fn set_state(&mut self, state_str: &str) {
        self.state = Some(state_str.to_owned());
    }
}

/// Example component #2
#[derive(Component, Debug, Default)]
pub struct BazComponent {}

/// Example component #3
#[derive(Component, Debug, Default)]
#[component(inject = "init_foobar(component::FoobarComponent)")]
#[component(inject = "init_baz(component::BazComponent)")]
pub struct QuuxComponent {
    /// State of Foo at the time we were initialized?
    pub foobar_state: Option<String>,

    /// Did we get a callback that `Baz` has been initialized?
    pub baz_initialized: bool,
}

impl QuuxComponent {
    /// Callback run after `FoobarComponent` has been initialized
    pub fn init_foobar(&mut self, foobar: &mut FoobarComponent) -> Result<(), FrameworkError> {
        self.foobar_state = foobar.state.clone();
        foobar.state = Some("hijacked!".to_owned());
        Ok(())
    }

    /// Callback run after `BazComponent` has been initialized
    pub fn init_baz(&mut self, _baz: &BazComponent) -> Result<(), FrameworkError> {
        self.baz_initialized = true;
        Ok(())
    }
}

fn init_components() -> Vec<Box<dyn Component<ExampleApp>>> {
    let mut foobar = FoobarComponent::default();
    foobar.set_state("original foobar state");

    let component1 = Box::new(foobar);
    let component2 = Box::<BazComponent>::default();
    let component3 = Box::<QuuxComponent>::default();

    let components: Vec<Box<dyn Component<ExampleApp>>> = vec![component1, component2, component3];

    // Ensure component IDs are as expected
    assert_eq!(components[0].id(), FOOBAR_COMPONENT_ID);
    assert_eq!(components[1].id(), BAZ_COMPONENT_ID);
    assert_eq!(components[2].id(), QUUX_COMPONENT_ID);

    components
}

#[test]
fn component_registration() {
    let mut registry = component::Registry::default();
    assert!(registry.is_empty());

    let components = init_components();
    registry.register(components).unwrap();
    assert_eq!(registry.len(), 3);

    // Fetch components and make sure they got registered correctly
    let foobar_comp = registry.get_by_id(FOOBAR_COMPONENT_ID).unwrap();
    assert_eq!(foobar_comp.id(), FOOBAR_COMPONENT_ID);

    let baz_comp = registry.get_by_id(BAZ_COMPONENT_ID).unwrap();
    assert_eq!(baz_comp.id(), BAZ_COMPONENT_ID);

    let quux_comp = registry.get_by_id(QUUX_COMPONENT_ID).unwrap();
    assert_eq!(quux_comp.id(), QUUX_COMPONENT_ID);
}

#[test]
fn duplicate_component_registration() {
    let foobar1 = Box::<FoobarComponent>::default();
    let foobar2 = Box::<FoobarComponent>::default();
    let components: Vec<Box<dyn Component<ExampleApp>>> = vec![foobar1, foobar2];

    let mut registry = component::Registry::default();
    assert!(registry.is_empty());

    let err = registry.register(components).err().unwrap();
    assert_eq!(*err.kind(), ComponentError);
    assert_eq!(registry.len(), 1);

    let foobar = registry.get_by_id(FOOBAR_COMPONENT_ID).unwrap();
    assert_eq!(foobar.id(), FOOBAR_COMPONENT_ID);
}

#[test]
fn get_downcast_ref() {
    let mut registry = component::Registry::default();
    let component = Box::<FoobarComponent>::default() as Box<dyn Component<ExampleApp>>;
    registry.register(vec![component]).unwrap();

    {
        let foo_mut = registry.get_downcast_mut::<FoobarComponent>().unwrap();
        foo_mut.set_state("mutated!");
    }

    {
        let foo_comp = registry.get_downcast_ref::<FoobarComponent>().unwrap();
        assert_eq!(foo_comp.state.as_ref().unwrap(), "mutated!");
    }
}

#[test]
fn dependency_injection() {
    let mut registry = component::Registry::default();
    let mut components = init_components();

    // Start component up in reverse order to make sure sorting works
    components.reverse();

    registry.register(components).unwrap();
    registry.after_config(&ExampleConfig::default()).unwrap();

    let foobar_comp = registry.get_downcast_ref::<FoobarComponent>().unwrap();
    assert_eq!(foobar_comp.state.as_ref().unwrap(), "hijacked!");

    let quux_comp = registry.get_downcast_ref::<QuuxComponent>().unwrap();
    assert_eq!(
        quux_comp.foobar_state.as_ref().unwrap(),
        "original foobar state"
    );
}
