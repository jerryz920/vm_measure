#!/usr/bin/ruby
if ARGV.length == 0 then 
  print 0
else
  print ARGV.reduce(0) {|sum, elem| sum + elem.to_f} / ARGV.length
end
