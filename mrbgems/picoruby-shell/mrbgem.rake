MRuby::Gem::Specification.new('picoruby-shell') do |spec|
  spec.license = 'MIT'
  spec.author  = 'HASUMI Hitoshi'
  spec.summary = 'PicoRuby Shell library'
  spec.add_dependency 'picoruby-terminal'
  spec.add_dependency 'picoruby-vfs'
  spec.add_dependency 'picoruby-filesystem-fat'
  spec.add_dependency 'picoruby-sandbox'

  executable_mrbfiles = Array.new
  executable_dir = "#{build_dir}/shell_executables"
  directory executable_dir
  Dir.glob("#{dir}/shell_executables/*.rb") do |rbfile|
    mrbfile = "#{executable_dir}/#{rbfile.pathmap('%n')}.c"
    file mrbfile => [rbfile, executable_dir] do |t|
      File.open(t.name, 'w') do |f|
        mrbc.run(f, t.prerequisites[0], "executable_#{t.name.pathmap("%n").gsub('-', '_')}", false)
      end
    end
    executable_mrbfiles << mrbfile
  end

  mrbgems_dir = File.expand_path "..", build_dir

  build.libmruby_objs << objfile("#{mrbgems_dir}/executables_init")
  file objfile("#{mrbgems_dir}/executables_init") => ["#{mrbgems_dir}/executables_init.c"]

  file "#{mrbgems_dir}/executables_init.c" => executable_mrbfiles do |t|
    mkdir_p File.dirname t.name
    open(t.name, 'w+') do |f|
      f.puts <<~PICOGEM
        #include <stdio.h>
        #include <stdbool.h>
        #include <mrubyc.h>
        #include <alloc.h>
      PICOGEM
      # shell executables
      executable_mrbfiles.each do |mrb|
        Rake::FileTask[mrb].invoke
        f.puts "#include \"#{mrb}\"" if File.exist?(mrb)
      end
      f.puts
      f.puts <<~PICOGEM
        typedef struct shell_executables {
          const char *name;
          const uint8_t *mrb;
        } shell_executables;
      PICOGEM
      f.puts
      f.puts "static shell_executables executables[] = {"
      executable_mrbfiles.each do |mrb|
        name = File.basename(mrb, ".c")
        f.puts "  {\"#{name}\", executable_#{name}}," if File.exist?(mrb)
      end
      f.puts "  {NULL, NULL} /* sentinel */"
      f.puts "};"
      f.puts
      f.puts <<~PICOGEM
        static void
        c_next_executable(mrbc_vm *vm, mrbc_value *v, int argc)
        {
          static int i = 0;
          if (executables[i].name) {
            const uint8_t *mrb = executables[i].mrb;
            mrbc_value hash = mrbc_hash_new(vm, 2);
            mrbc_value name = mrbc_string_new_cstr(vm, (char *)executables[i].name);
            mrbc_hash_set(&hash,
              &mrbc_symbol_value(mrbc_str_to_symid("name")),
              &name
            );
            uint32_t codesize = (mrb[8] << 24) + (mrb[9] << 16) + (mrb[10] << 8) + mrb[11];
            mrbc_value code_val = mrbc_string_new(vm, mrb, codesize);
            mrbc_hash_set(&hash,
              &mrbc_symbol_value(mrbc_str_to_symid("code")),
              &code_val
            );
            SET_RETURN(hash);
            i++;
          } else {
            SET_NIL_RETURN();
          }
        }
      PICOGEM
      f.puts <<~PICOGEM
        void
        picoruby_init_executables(void)
        {
          mrbc_class *mrbc_class_Shell = mrbc_define_class(0, "Shell", mrbc_class_object);
          mrbc_define_method(0, mrbc_class_Shell, "next_executable", c_next_executable);
        }
      PICOGEM
    end
  end
end
