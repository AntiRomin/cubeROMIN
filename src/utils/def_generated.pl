#!/usr/bin/perl
use warnings;
use strict;

# This sript will generate templated files for peripherals

# io_def_generated.h

my @ports = ('A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I');
my @pins  = 0 .. 15;
my @timers = (1,2,3,4,6,7,8,15,16,17);
my $drivers_dir = "src/platform/common";

# change list separator to newline - we use @{} interpolation to merge multiline strings
$" = "\n";

chomp(my $license = <<"END");
/*
 * This file is part of Cleanflight and Betaflight.
 *
 * Cleanflight and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Cleanflight and Betaflight are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */
END

chomp(my $disclaimer_generated = <<"END");
// this file is automatically generated by src/utils/def_generated.pl script
// do not modify this file directly, your changes will be lost
END

my $io_def_file="$drivers_dir/io_def_generated.h";
my $fh;
open $fh, '>', $io_def_file or die "Cannot open $io_def_file: $!";
print { $fh} <<"END" or die  "Cannot write into $io_def_file: $!"; close $fh;
${license}

#pragma once

${disclaimer_generated}

// DEFIO_PORT_<port>_USED_MASK is bitmask of used pins on target
// DEFIO_PORT_<port>_USED_COUNT is count of used pins on target

@{[do {
    my @prev_ports = ();
    map { my $port = $_;  my $ret = << "END2"; push @prev_ports, $port; $ret } @ports; }]}
#if defined(TARGET_IO_PORT${port})
# define DEFIO_PORT_${port}_USED_MASK TARGET_IO_PORT${port}
# define DEFIO_PORT_${port}_USED_COUNT BITCOUNT(DEFIO_PORT_${port}_USED_MASK)
#else
# define DEFIO_PORT_${port}_USED_MASK 0
# define DEFIO_PORT_${port}_USED_COUNT 0
#endif
#define DEFIO_PORT_${port}_OFFSET (@{[join('+', map {  "DEFIO_PORT_${_}_USED_COUNT" } @prev_ports) || '0']})
END2


// DEFIO_GPIOID__<port> maps to port index
@{[ map { my $port = $_; chomp(my $ret = << "END2"); $ret } @ports ]}
#define DEFIO_GPIOID__${port} @{[ord($port)-ord('A')]}
END2

// DEFIO_TAG__P<port><pin> will expand to TAG if defined for target, error is triggered otherwise
// DEFIO_TAG_E__P<port><pin> will expand to TAG if defined, to NONE otherwise (usefull for tables that are CPU-specific)
// DEFIO_REC__P<port><pin> will expand to ioRec* (using DEFIO_REC_INDEX(idx))

@{[do {
    my @prev_ports = ();
    map { my $port = $_;  my @ret = map { my $pin = $_; chomp(my $ret = << "END2"); $ret } @pins ; push @prev_ports, $port; @ret } @ports; }]}
#if DEFIO_PORT_${port}_USED_MASK & BIT(${pin})
# define DEFIO_TAG__P${port}${pin} DEFIO_TAG_MAKE(DEFIO_GPIOID__${port}, ${pin})
# define DEFIO_TAG_E__P${port}${pin} DEFIO_TAG_MAKE(DEFIO_GPIOID__${port}, ${pin})
# define DEFIO_REC__P${port}${pin} DEFIO_REC_INDEXED(BITCOUNT(DEFIO_PORT_${port}_USED_MASK & (BIT(${pin}) - 1)) + @{[join('+', map {  "DEFIO_PORT_${_}_USED_COUNT" } @prev_ports) || '0']})
#else
# define DEFIO_TAG__P${port}${pin} defio_error_P${port}${pin}_is_not_supported_on_TARGET
# define DEFIO_TAG_E__P${port}${pin} DEFIO_TAG_E__NONE
# define DEFIO_REC__P${port}${pin} defio_error_P${port}${pin}_is_not_supported_on_TARGET
#endif
END2

// DEFIO_IO_USED_COUNT is number of io pins supported on target
#define DEFIO_IO_USED_COUNT (@{[join('+', map {  "DEFIO_PORT_${_}_USED_COUNT" } @ports) || '0']})

// DEFIO_PORT_USED_LIST - comma separated list of bitmask for all used ports.
// DEFIO_PORT_OFFSET_LIST - comma separated list of port offsets (count of pins before this port)
// unused ports on end of list are skipped
@{[do {
    my @used_ports = @ports;
    map { my $port = $_; chomp(my $ret = << "END2"); @used_ports = grep {$_ ne $port} @used_ports; $ret } reverse(@ports) }]}
#if !defined DEFIO_PORT_USED_LIST && DEFIO_PORT_${port}_USED_COUNT > 0
# define DEFIO_PORT_USED_COUNT @{[scalar @used_ports]}
# define DEFIO_PORT_USED_LIST @{[join(',', map { "DEFIO_PORT_${_}_USED_MASK" } @used_ports)]}
# define DEFIO_PORT_OFFSET_LIST @{[join(',', map { "DEFIO_PORT_${_}_OFFSET" } @used_ports)]}
#endif
END2

#if !defined(DEFIO_PORT_USED_LIST)
# if !defined DEFIO_NO_PORTS   // supress warnings if we really don't want any pins
#  warning "No pins are defined. Maybe you forgot to define TARGET_IO_PORTx in target.h"
# endif
# define DEFIO_PORT_USED_COUNT 0
# define DEFIO_PORT_USED_LIST /* empty */
# define DEFIO_PORT_OFFSET_LIST /* empty */
#endif
END

exit; # only IO code is merged now

my $timer_def_file="$drivers_dir/timer_def_generated.h";
open $fh, '>', $timer_def_file or die "Cannot open $timer_def_file: $!";
print { $fh} <<"END" or die  "Cannot write into $timer_def_file: $!"; close $fh;
#pragma once
${disclaimer_generated}

// make sure macros for all timers are defined
@{[ map { my $timer = $_; chomp(my $ret = << "END2"); $ret } @timers ]}
#ifndef TARGET_TIMER_TIM${timer}
# define TARGET_TIMER_TIM${timer} -1
#endif
END2

// generate mask with used timers
@{[do {
     my @prev_timers = ();
     map { my $timer = $_; chomp(my $ret = << "END2"); push @prev_timers, $timer; $ret } @timers }]}
#if TARGET_TIMER_TIM${timer} > 0
# define TARGET_TIMER_TIM${timer}_BIT BIT(${timer})
# define TARGET_TIMER_TIM${timer}_INDEX  ( @{[ join("+", map("(TARGET_TIMER_TIM${_} >= 0)", @prev_timers)) || 0 ]} )
#else
# define TARGET_TIMER_TIM${timer}_BIT 0
# define TARGET_TIMER_TIM${timer}_INDEX deftimer_error_TIMER${timer}_is_not_enabled_on_target
#endif
END2
#define TIMER_USED_BITS  ( @{[ join "|", map("TARGET_TIMER_TIM${_}_BIT", @timers) ]} )
#define TIMER_USED_COUNT ( @{[ join "+", map("(TARGET_TIMER_TIM${_} >= 0)", @timers) ]} )

// structure to hold all timerRec_t.
// Number of channels per timer is user specifed, structure will ensure correct packing
struct timerRec_all {
@{[ map { my $timer = $_; chomp(my $ret = << "END2"); $ret } @timers ]}
#if TARGET_TIMER_TIM${timer} >= 0
    timerRec_t rec_TIM${timer};
# if TARGET_TIMER_TIM${timer} > 0
    timerChRec_t rec_TIM${timer}_ch[TARGET_TIMER_TIM${timer}];
# endif
#endif
END2
};
END

my $timer_inc_file="$drivers_dir/timer_c_generated.inc";
open $fh, '>', $timer_inc_file or die "Cannot open $timer_inc_file: $!";
print { $fh} <<"END" or die  "Cannot write into $timer_inc_file: $!"; close $fh;
${disclaimer_generated}

// this code is included into timer.c file

const timerDef_t timerDefs[] = {
@{[ map { my $timer = $_; chomp(my $ret = << "END2"); $ret } @timers ]}
#if TARGET_TIMER_TIM${timer} >= 0
       DEF_TIMER_DEFINE(${timer}),
#endif
END2
};

timerRec_t* const timerRecPtrs[] = {
@{[ map { my $timer = $_; chomp(my $ret = << "END2"); $ret } @timers ]}
#if TARGET_TIMER_TIM${timer} >= 0
       &timerRecs.rec_TIM${timer},
#endif
END2
NULL    // terminate the list
};
END
