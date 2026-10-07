MRuby::Gem::Specification.new('picoruby-network') do |spec|
  spec.license = 'MIT'
  spec.author  = 'HASUMI Hitoshi'
  spec.summary = 'Network abstraction layer for different platforms'

  spec.add_dependency 'picoruby-machine'
  if %w(pico_w pico2_w).include?(ENV['PICORB_BOARD'])
    spec.add_dependency 'picoruby-cyw43'
  end
  # mrblib/network.rb reads ESP32::WiFi while it loads. With the mruby VM every
  # gem's mrblib runs when the VM opens, in dependency order, so picoruby-esp32
  # has to be initialised first (mruby/c only runs it on `require`).
  if build.cc.defines.include?('ESP32_PLATFORM')
    spec.add_dependency 'picoruby-esp32'
  end
end
