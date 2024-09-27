# Steps to reproduce

- `git clone https://github.com/aolsenjazz/midi-test-coremidi`
- Repos contains a binary which you can run, just let Security preferences that you want to run it
- You can also just build from source if you want: `./build.sh`
- Download + install [mimic](https://github.com/aolsenjazz/mimic)
- Launch an APC Key 25 from top-left "New" menu. Note, c++ script is hardcoded to this device. You can change in the script
- Run `./midi_test`
- midi_test will connect to APC Key 25. Mimic will log messages received in the message history panel
- Fire up MIDI monitor as well, set your listen-to and spy-on
- Spam one of the "pads" in Mimic a bunch, a few times
- You'll see the discrepancy between Mimic-reported times and MIDI Monitor-reported times

This should show you the behavior I'm seeing!