macro_rules! action {
    (| $self:tt, $ctx:tt, $input:ident | > $action_fn:ident ? $($args:expr),* ) => {
        $self.$action_fn($ctx, $input $(,$args),*).map_err(ParsingTermination::ActionError)?;
    };

    (| $self:tt, $ctx:tt, $input:ident | > $action_fn:ident $($args:expr),* ) => {
        $self.$action_fn($ctx, $input $(,$args),*);
    };

    ( @state_transition | $self:tt, $ctx:tt, $input:ident | > reconsume in $state:ident) => {
        $self.unconsume_ch();
        action!(@state_transition | $self, $ctx, $input | > --> $state);
    };

    ( @state_transition | $self:tt, $ctx:tt, $input:ident | > - -> $state:ident) => {
        $self.switch_state(Self::$state);
        return Ok(());
    };

    ( @state_transition | $self:tt, $ctx:tt, $input:ident | > - -> dyn $state_getter:ident) => {
        {
            let state = $self.$state_getter();
            $self.switch_state(state);
        }

        return Ok(());
    };
}
