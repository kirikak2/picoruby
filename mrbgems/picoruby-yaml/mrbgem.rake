MRuby::Gem::Specification.new('picoruby-yaml') do |spec|
  spec.license = 'MIT'
  spec.author  = 'HASUMI Hitoshi'
  spec.summary = 'YAML parser for PicoRuby'

  if build.posix?
    if build.vm_mrubyc?
      spec.add_dependency 'picoruby-posix-io'
    else
      spec.add_dependency 'mruby-io'
    end
  else
    # Skip littlefs if filesystem-fat is in the build (mutually exclusive).
    unless build.gems.any? { |g| g.name == 'picoruby-filesystem-fat' }
      spec.add_dependency 'picoruby-littlefs'
    end
    spec.add_dependency 'picoruby-vfs'
  end
end

