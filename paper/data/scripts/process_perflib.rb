#!/usr/bin/ruby


timing = Hash.new(0);
ARGV.each { |file_name|
  File.open(file_name) { |f|
    f.each_line {|line|
      arr=line.split
      timing[arr[0]] += arr[1].to_f
    }
  }
}

timing.each {|key, val| val /= 80; print key, " ", val, "\n" }

