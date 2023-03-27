require "shell"
require "sqlite3"

Shell.setup(:ram)

Dir.open "/" do |dir|
  while ent = dir.read
    p ent
  end
end

SQLite3::Database.vfs_methods = FAT::File.vfs_methods
db = SQLite3::Database.open "/home/test.db"
p db

db.close
