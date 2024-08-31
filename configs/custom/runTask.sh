#!/bin/bash

function testRun {
  if [ -f "$1" ]; then
    rm "$1"
  fi
  touch "$1"
  injection_rates=("0.01" "0.1" "0.2" "0.3" "0.4" "0.5" "0.6" "0.7" "0.8" "0.9" "1")
  for ((i = 0; i < ${#injection_rates[*]}; i++)); do
    injection_rate="${injection_rates[i]}"
    cd "${run_ev}"
    ../../../build/NULL/gem5.opt ../../example/garnet_synth_traffic.py \
    --network=garnet --vcs-per-vnet=16 --topology=Torus \
    --inj-vnet=0 --synthetic="${traffic}" \
    --injectionrate="${injection_rate}" --sim-cycles=5000000 $2
    cd ..
    if [ -s "${run_ev}/m5out/stats.txt" ]; then
      echo "${injection_rate}" >> "$1"
      grep "packets_injected::total" "${run_ev}/m5out/stats.txt" | sed '{s/system.ruby.network.packets_injected::total\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
      grep "packets_received::total" "${run_ev}/m5out/stats.txt" | sed '{s/system.ruby.network.packets_received::total\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
      grep "average_packet_queueing_latency" "${run_ev}/m5out/stats.txt" | sed '{s/system.ruby.network.average_packet_queueing_latency\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
      grep "average_packet_network_latency" "${run_ev}/m5out/stats.txt" | sed '{s/system.ruby.network.average_packet_network_latency\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
      grep "average_packet_latency" "${run_ev}/m5out/stats.txt" | sed '{s/system.ruby.network.average_packet_latency\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
      grep "average_hops" "${run_ev}/m5out/stats.txt" | sed '{s/system.ruby.network.average_hops\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
      grep "average_drs" "${run_ev}/m5out/stats.txt" | sed '{s/system.ruby.network.average_drs\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
      grep "average_misrouting" "${run_ev}/m5out/stats.txt" | sed '{s/system.ruby.network.average_misrouting\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
      grep "reception_rate" "${run_ev}/m5out/stats.txt" | sed '{s/system.ruby.network.reception_rate\s*//g;s/nan/-1/g;s/[^0-9.\-]*//g}' >> "$1"
    fi
  done
}

function testcase {
  local -a synthetic_choices
  synthetic_choices=("uniform_random" "bit_reverse" "bit_rotation" "bit_complement" "transpose" "tornado")
  topo_choices=("--num-ary=8 --num-dim=2" "--num-ary=4 --num-dim=3")
  topo_name=("k8d2" "k4d3")
  if [ ! -e "${data_save_path}" ]; then
    mkdir "${data_save_path}"
  fi
  for ((topo_id = 0; topo_id < ${#topo_choices[*]}; ++topo_id)); do
     traffic=${synthetic_choices[idx]}
     testRun "${data_save_path}/${topo_name[topo_id]}_${traffic}_deter.txt" \
     "${topo_choices[topo_id]} --routing-algorithm=2"

     testRun "${data_save_path}/${topo_name[topo_id]}_${traffic}_sta_1.txt" \
     "${topo_choices[topo_id]} --routing-algorithm=3 --vcs-adaptive=8 --dr-lim=1 --misrouting-lim=16 --pick-algorithm=1"

     testRun "${data_save_path}/${topo_name[topo_id]}_${traffic}_sta_2.txt" \
     "${topo_choices[topo_id]} --routing-algorithm=3 --vcs-adaptive=8 --dr-lim=2 --misrouting-lim=16  --pick-algorithm=1"

     testRun "${data_save_path}/${topo_name[topo_id]}_${traffic}_dyn_0_1.txt" \
     "${topo_choices[topo_id]} --routing-algorithm=4 --vcs-adaptive=8 --misrouting-lim=16  --pick-algorithm=1"

     testRun "${data_save_path}/${topo_name[topo_id]}_${traffic}_dyn_2_1.txt" \
     "${topo_choices[topo_id]} --routing-algorithm=4 --vcs-adaptive=8 --misrouting-lim=16 --throttling-degree=2 --pick-algorithm=1"

     testRun "${data_save_path}/${topo_name[topo_id]}_${traffic}_dyn_4_1.txt" \
     "${topo_choices[topo_id]} --routing-algorithm=4 --vcs-adaptive=8 --misrouting-lim=16 --throttling-degree=4 --pick-algorithm=1"

     testRun "${data_save_path}/${topo_name[topo_id]}_${traffic}_dyn_0_0.txt" \
     "${topo_choices[topo_id]} --routing-algorithm=4 --vcs-adaptive=8 --misrouting-lim=16 --pick-algorithm=0"

     testRun "${data_save_path}/${topo_name[topo_id]}_${traffic}_dyn_0_2.txt" \
     "${topo_choices[topo_id]} --routing-algorithm=4 --vcs-adaptive=8 --misrouting-lim=16 --pick-algorithm=2"
  done
}

run_ev="$1"
idx="$2"
data_save_path="./data"
if [ ! -e "${run_ev}" ]; then
  mkdir "${run_ev}"
fi
testcase
rm -rf "${run_ev}"
