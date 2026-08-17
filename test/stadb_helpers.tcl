# Shared helpers for stadb regressions.
#
# Success prints a short "name matches: 1" line. Failure prints the drifted
# key, cold vs warm values, and the stadb file to edit.

if { ![info exists test_dir] } {
  source [file join [file dirname [info script]] helpers.tcl]
}

set stadb_helpers_file [file normalize [info script]]
set stadb_mismatch_limit 20

# Prefix of a dump key or check name -> where to edit.
array set stadb_fix_sites {
  cmd       {stadb/StaDb.tcl}
  sdc       {stadb/DbSdc.cc  DbSdcKind write/read}
  liberty   {stadb/StaDbWriter.cc StaDbReader.cc  DbSections.hh}
  network   {stadb/StaDbWriter.cc StaDbReader.cc  DbNetworkWriter}
  graph     {stadb/DbGraph.cc}
  edge      {stadb/DbGraph.cc}
  search    {stadb/DbSearch.cc}
  prop      {search/Property.cc plus the matching stadb writer}
  parasitics {stadb/DbFormat.hh DbSectionId::parasitics (not written)}
  error     {stadb/DbFile.cc DbCodec.cc}
}

proc stadb_run { body { tag "child" } } {
  global stadb_helpers_file
  set script [make_result_file "stadb.$tag.tcl"]
  set stream [open $script "w"]
  puts $stream "source $stadb_helpers_file"
  puts $stream $body
  puts $stream "exit"
  close $stream
  set status [catch { exec [info nameofexecutable] -no_init -no_splash \
                        -exit $script 2>@1 } output]
  return $output
}

proc stadb_contents { filename } {
  set stream [open $filename "rb"]
  set contents [read $stream]
  close $stream
  return $contents
}

proc stadb_names { objects } {
  set names {}
  foreach_in_collection obj $objects {
    lappend names [get_full_name $obj]
  }
  return [lsort -dictionary $names]
}

proc stadb_stringify { value } {
  if { $value eq "" || $value eq "NULL" } {
    return ""
  }
  if { ![catch { is_object $value } is_obj] && $is_obj } {
    if { [catch { get_full_name $value } name] } {
      if { [catch { get_property $value name } name] } {
        return $value
      }
    }
    return $name
  }
  if { ![catch { sta::is_collection $value } is_col] && $is_col } {
    set names {}
    foreach_in_collection obj $value {
      lappend names [stadb_stringify $obj]
    }
    return [join [lsort $names] ","]
  }
  if { [llength $value] > 1 } {
    set names {}
    set all_obj 1
    foreach item $value {
      if { [catch { is_object $item } item_obj] || !$item_obj } {
        set all_obj 0
        break
      }
      lappend names [stadb_stringify $item]
    }
    if { $all_obj } {
      return [join [lsort $names] ","]
    }
  }
  return $value
}

proc stadb_prop { object property } {
  if { [catch { get_property $object $property } value] } {
    return "ERROR"
  }
  return [stadb_stringify $value]
}

proc stadb_fix_for { name } {
  global stadb_fix_sites
  foreach prefix [array names stadb_fix_sites] {
    if { $name eq $prefix || [string match "$prefix *" $name] \
           || [string match "${prefix}_*" $name] \
           || [string match "* $prefix *" " $name "] } {
      return $stadb_fix_sites($prefix)
    }
  }
  return "stadb/"
}

proc stadb_lines { text } {
  set lines {}
  foreach line [split $text "\n"] {
    set line [string trimright $line]
    if { $line ne "" } {
      lappend lines $line
    }
  }
  return $lines
}

proc stadb_line_key { line } {
  set fields [split $line]
  if { [llength $fields] < 2 } {
    return [list $line ""]
  }
  set value [lindex $fields end]
  set key [join [lrange $fields 0 end-1] " "]
  return [list $key $value]
}

proc stadb_print_mismatch { name cold warm } {
  global stadb_mismatch_limit
  puts "stadb mismatch: $name"
  puts "  fix: [stadb_fix_for $name]"
  array set cold_map {}
  array set warm_map {}
  foreach line [stadb_lines $cold] {
    lassign [stadb_line_key $line] key value
    set cold_map($key) $value
  }
  foreach line [stadb_lines $warm] {
    lassign [stadb_line_key $line] key value
    set warm_map($key) $value
  }
  set keys {}
  foreach key [concat [array names cold_map] [array names warm_map]] {
    if { [lsearch -exact $keys $key] < 0 } {
      lappend keys $key
    }
  }
  set keys [lsort $keys]
  set shown 0
  set extra 0
  foreach key $keys {
    set c ""
    set w ""
    if { [info exists cold_map($key)] } {
      set c $cold_map($key)
    }
    if { [info exists warm_map($key)] } {
      set w $warm_map($key)
    }
    if { $c eq $w } {
      continue
    }
    if { $shown < $stadb_mismatch_limit } {
      if { $c eq "" } {
        set c "(missing)"
      }
      if { $w eq "" } {
        set w "(missing)"
      }
      puts "  $key"
      puts "    cold: $c"
      puts "    warm: $w"
      incr shown
    } else {
      incr extra
    }
  }
  if { $extra > 0 } {
    puts "  ... and $extra more"
  }
  if { $shown == 0 } {
    puts "  cold: $cold"
    puts "  warm: $warm"
  }
}

proc stadb_check { name cold warm } {
  set match [expr { $cold eq $warm }]
  puts "$name matches: $match"
  if { !$match } {
    stadb_print_mismatch $name $cold $warm
  }
  return $match
}

proc stadb_check_files { name cold_file warm_file } {
  stadb_check $name [stadb_contents $cold_file] [stadb_contents $warm_file]
}

proc stadb_check_backup { name cold_file warm_file } {
  if { [stadb_contents $cold_file] eq [stadb_contents $warm_file] } {
    puts "$name backup matches: 1"
  } else {
    puts "$name backup: keyed dump matched; backup dump drifted"
    puts "  fix: [stadb_fix_for $name]"
  }
}

proc stadb_scrub { text } {
  global result_dir test_dir
  return [string map [list \
                        [file normalize $result_dir] RESULTS \
                        [file normalize $test_dir] TEST \
                        $result_dir RESULTS \
                        $test_dir TEST] $text]
}

proc stadb_pin { name } {
  set pin [get_pins -quiet $name]
  if { [sizeof_collection $pin] == 0 } {
    set pin [get_ports -quiet $name]
  }
  return $pin
}

proc stadb_sdc_groups { filename } {
  set stream [open $filename "r"]
  set text [read $stream]
  close $stream
  array set groups {}
  set cmd ""
  foreach line [split $text "\n"] {
    set trimmed [string trim $line]
    if { $trimmed eq "" || [string index $trimmed 0] eq "#" } {
      continue
    }
    if { [string index $line 0] eq " " || [string index $line 0] eq "\t" } {
      if { $cmd ne "" } {
        append groups($cmd) $line "\n"
      }
      continue
    }
    if { ![regexp {^(\S+)} $trimmed raw] } {
      continue
    }
    set cmd [string trimright $raw "\\"]
    if { $cmd eq "" } {
      continue
    }
    append groups($cmd) $line "\n"
  }
  return [array get groups]
}

proc stadb_check_sdc { cold_file warm_file } {
  array set cold [stadb_sdc_groups $cold_file]
  array set warm [stadb_sdc_groups $warm_file]
  set cmds {}
  foreach cmd [concat [array names cold] [array names warm]] {
    if { [lsearch -exact $cmds $cmd] < 0 } {
      lappend cmds $cmd
    }
  }
  foreach cmd [lsort $cmds] {
    set c ""
    set w ""
    if { [info exists cold($cmd)] } {
      set c $cold($cmd)
    }
    if { [info exists warm($cmd)] } {
      set w $warm($cmd)
    }
    stadb_check "sdc $cmd" $c $w
  }
}

proc stadb_dump_liberty_cell { cell } {
  set name [get_property $cell name]
  foreach prop {area is_buffer is_inverter is_sequential is_clock_gate \
                  is_integrated_clock_gating_cell is_memory is_memory_cell \
                  dont_use is_physical_only has_timing_model} {
    puts "liberty cell $name $prop [stadb_prop $cell $prop]"
  }
}

proc stadb_dump_liberty_port { port } {
  set name [get_property $port full_name]
  foreach prop {direction capacitance is_clock is_register_clock} {
    puts "liberty port $name $prop [stadb_prop $port $prop]"
  }
}

proc stadb_dump_instance { inst } {
  set name [get_full_name $inst]
  foreach prop {ref_name is_hierarchical is_sequential is_buffer \
                  is_clock_gate is_inverter is_macro is_memory design_type} {
    puts "network inst $name $prop [stadb_prop $inst $prop]"
  }
}

proc stadb_dump_pin { pin } {
  set name [get_full_name $pin]
  foreach prop {direction is_hierarchical is_port is_register_clock \
                  is_clock is_rise_edge_triggered is_fall_edge_triggered} {
    puts "network pin $name $prop [stadb_prop $pin $prop]"
  }
}

proc stadb_dump_clock { clk } {
  set name [get_property $clk name]
  foreach prop {period is_generated is_virtual is_propagated} {
    puts "sdc clock $name $prop [stadb_prop $clk $prop]"
  }
}

proc stadb_dump_pin_timing { pin } {
  set name [get_full_name $pin]
  foreach prop {arrival_max_rise arrival_max_fall arrival_min_rise \
                  arrival_min_fall slack_max slack_min slew_max slew_min} {
    puts "search pin $name $prop [stadb_prop $pin $prop]"
  }
}

proc stadb_repo_file { args } {
  global test_dir
  return [file join $test_dir {*}$args]
}
