macro_rules! action {
    (| $self:tt, $input:ident | > $action_fn:ident ? $($args:expr),* ) => {
        $self.$action_fn($input $(,$args),*).map_err(ParsingTermination::ActionError)?;
    };

    (| $self:tt, $input:ident | > $action_fn:ident $($args:expr),* ) => {
        $self.$action_fn($input $(,$args),*);
    };

    ( @state_transition | $self:tt, $input:ident | > reconsume in $state:ident) => {
        $self.unconsume_ch();
        action!(@state_transition | $self, $input | > --> $state);
    };

    ( @state_transition | $self:tt, $input:ident | > - -> $state:ident) => {
        $self.switch_state(Self::$state);
        return Ok(());
    };

    ( @state_transition | $self:tt, $input:ident | > - -> dyn $state_getter:ident) => {
        {
            let state = $self.$state_getter();
            $self.switch_state(state);
        }

        return Ok(());
    };
}
