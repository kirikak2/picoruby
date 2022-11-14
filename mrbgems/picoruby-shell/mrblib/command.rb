class Shell
  class Command
    def initialize
    end

    attr_accessor :feed

    BUILTIN = %w(
      free
      alias
      cd
      echo
      export
      exec
      exit
      fc
      help
      history
      kill
      pwd
      type
      unset
    )

    def find_executable(name)
      ENV['PATH'].each do |path|
        file = "#{path}/#{name}"
        return file if File.exist? file
      end
      if name.include?("/") && File.exist?(name)
        return name
      end
      return nil
    end

    def exec(params)
      args = params[1, params.length - 1]
      case params[0]
      when "echo"
        _echo(*args)
      when "type"
        _type(*args)
      when "pwd"
        print Dir.pwd, @feed
      when "cd"
        print @feed if Dir.chdir(args[0] || ENV['HOME']) != 0
      when "free"
        Object.memory_statistics.each do |k, v|
          print "#{k.to_s.ljust(5)}: #{v.to_s.rjust(8)}", @feed
        end
      else
        if exefile = find_executable(params[0])
          f = File.open(exefile, "r")
          begin
            # PicoRuby compiler's bug
            # https://github.com/picoruby/picoruby/issues/120
            mrb = f.read
            if mrb.start_with?("RITE0300")
              ARGV.clear
              args.each do |param|
                ARGV << param
              end
              sandbox = Sandbox.new
              sandbox.exec_mrb(mrb)
              if sandbox.wait(nil) && error = sandbox.error
                print "#{error.message} (#{error.class})", @feed
              end
            else
              print "Invalide VM code", @feed
            end
          rescue => e
            p e
          ensure
            f.close
          end
        else
          print "#{params[0]}: command not found", @feed
        end
      end
    end

    #
    # Builtin commands
    #

    def _type(*params)
      params.each_with_index do |name, index|
        print @feed if 0 < index
        if BUILTIN.include?(name)
          print "#{name} is a shell builtin"
        elsif path = find_executable(name)
          print "#{name} is #{path}"
        else
          print "type: #{name}: not found"
        end
      end
      print @feed
    end

    def _echo(*params)
      params.each_with_index do |param, index|
        print " " if 0 < index
        print param
      end
    end

  end
end
