define_state_group!(data_states_group = {

    data_state {
        b'<' => ( emit_text?; mark_tag_start; --> tag_open_state )
        eoc  => ( emit_text?; )
        eof  => ( emit_text?; emit_eof?; )
        _    => ()
    }

});
