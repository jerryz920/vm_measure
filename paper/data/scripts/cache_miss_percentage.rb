#!/usr/bin/ruby

ARGV.each { |fname|
  File.open(fname) { |f|
    f.each_line {|line|
      arr = line.split
      print (arr.shift 1), "\t"
      arr.map! {|elem| elem.to_f}
      sum = arr.reduce('+')
      arr.map! {|elem| printf "%.4f " % [elem * 100/ sum]; elem}
      puts ""
    }
  }
}


