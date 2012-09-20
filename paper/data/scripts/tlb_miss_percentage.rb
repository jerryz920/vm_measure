#!/usr/bin/ruby

ARGV.each { |fname|
  File.open(fname) { |f|
    f.each_line {|line|
      arr = line.split
      print "%s %.4f\n" % [arr[0], arr[2].to_f / arr[1].to_f]
    }
  }
}


