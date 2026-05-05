MRuby::Gem::Specification.new('picoruby-sam2695') do |spec|
  spec.license = 'MIT'
  spec.author  = 'Toshio Maki'
  spec.summary = 'SAM2695 MIDI synth — thin wrapper over picoruby-uart_midi'
  spec.require_name = 'sam2695'
  spec.add_dependency 'picoruby-uart_midi'
end
