#!/bin/bash

function testRun {
  for ((i = 50; i <= 50; i++)); do
    injection_rate="0.${i}"
    if [ $i -lt 10 ]; then
      injection_rate="0.0${i}"
    fi
    #echo "${injection_rate}"
    echo "${injection_rate}" >> "$1"
    ../../build/NULL/gem5.opt ../example/garnet_synth_traffic.py \
    --network=garnet --num-ary=4 --num-dim=3 \
    --topology=Torus \
    --inj-vnet=0 --synthetic="${synthetic_choices[idx]}" \
    --injectionrate="${injection_rate}" --sim-cycles=100000000 $2
    grep "packets_injected::total" m5out/stats.txt | sed '{s/system.ruby.network.packets_injected::total\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
    grep "packets_received::total" m5out/stats.txt | sed '{s/system.ruby.network.packets_received::total\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
    grep "average_packet_queueing_latency" m5out/stats.txt | sed '{s/system.ruby.network.average_packet_queueing_latency\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
    grep "average_packet_network_latency" m5out/stats.txt | sed '{s/system.ruby.network.average_packet_network_latency\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
    grep "average_packet_latency" m5out/stats.txt | sed '{s/system.ruby.network.average_packet_latency\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
    grep "average_hops" m5out/stats.txt | sed '{s/system.ruby.network.average_hops\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
    grep "reception_rate" m5out/stats.txt | sed '{s/system.ruby.network.reception_rate\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
  done
}

function testcase {
  local -a synthetic_choices
  synthetic_choices=("uniform_random" "bit_reverse")
  for ((idx = 0; idx < ${#synthetic_choices[*]}; ++idx)); do

    # simple (0 args)
     result_filename="simple_${synthetic_choices[idx]}.txt"
     rm "${result_filename}"
     touch "${result_filename}"
     testRun "${result_filename}" "--enable-bidirectional"

  done
}

testcase