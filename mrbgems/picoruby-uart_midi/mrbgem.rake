MRuby::Gem::Specification.new('picoruby-uart_midi') do |spec|
  spec.license = 'MIT'
  spec.author  = 'Toshio Maki'
  spec.summary = 'UART/Serial MIDI transport layer for PicoRuby'
  spec.require_name = 'uart_midi'
  spec.add_dependency 'picoruby-machine'
end
