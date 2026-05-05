MRuby::Gem::Specification.new('picoruby-usb_midi_host') do |spec|
  spec.license = 'MIT'
  spec.author  = 'Toshio Maki'
  spec.summary = 'USB-MIDI Host transport layer'
  spec.require_name = 'usb_midi_host'
  spec.add_dependency 'picoruby-machine'
end
