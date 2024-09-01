#!/bin/bash

function testRun {
  if [ -f "$1" ]; then
    rm "$1"
  fi
  touch "$1"
  injection_rates=("1")
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
  if [ ! -e "${data_save_path}" ]; then
    mkdir "${data_save_path}"
  fi
 traffic=${synthetic_choices[idx]}
 testRun "${data_save_path}/k8d2_${traffic}_deter.txt" \
 "--num-ary=8 --num-dim=2 --routing-algorithm=2"

 testRun "${data_save_path}/k8d2_${traffic}_sta_1.txt" \
 "--num-ary=8 --num-dim=2 --routing-algorithm=3 --vcs-adaptive=8 --dr-lim=1 --misrouting-lim=16 --pick-algorithm=1"

 testRun "${data_save_path}/k8d2_${traffic}_sta_2.txt" \
 "--num-ary=8 --num-dim=2 --routing-algorithm=3 --vcs-adaptive=8 --dr-lim=2 --misrouting-lim=16  --pick-algorithm=1"

 testRun "${data_save_path}/k8d2_${traffic}_dyn_0_1.txt" \
 "--num-ary=8 --num-dim=2 --routing-algorithm=4 --vcs-adaptive=8 --misrouting-lim=16 --throttling-degree=0  --pick-algorithm=1"

 testRun "${data_save_path}/k4d3_${traffic}_dyn_0_1.txt" \
 "--num-ary=4 --num-dim=3 --routing-algorithm=4 --vcs-adaptive=8 --misrouting-lim=16 --throttling-degree=0  --pick-algorithm=1"

 testRun "${data_save_path}/k8d2_${traffic}_dyn_2_1.txt" \
 "--num-ary=8 --num-dim=2 --routing-algorithm=4 --vcs-adaptive=8 --misrouting-lim=16 --throttling-degree=2 --pick-algorithm=1"

 testRun "${data_save_path}/k8d2_${traffic}_dyn_4_1.txt" \
 "--num-ary=8 --num-dim=2 --routing-algorithm=4 --vcs-adaptive=8 --misrouting-lim=16 --throttling-degree=4 --pick-algorithm=1"

 testRun "${data_save_path}/k8d2_${traffic}_dyn_0_0.txt" \
 "--num-ary=8 --num-dim=2 --routing-algorithm=4 --vcs-adaptive=8 --misrouting-lim=16 --pick-algorithm=0"

 testRun "${data_save_path}/k8d2_${traffic}_dyn_0_2.txt" \
 "--num-ary=8 --num-dim=2 --routing-algorithm=4 --vcs-adaptive=8 --misrouting-lim=16 --pick-algorithm=2"
}

data_save_path="./data"
synthetic_choices=("uniform_random" "bit_reverse" "bit_rotation" "bit_complement" "transpose")
run_ev="./exp"
if [ ! -e "${run_ev}" ]; then
  mkdir "${run_ev}"
fi
if [ $# -lt 1 ]; then
  for ((j = 0; j < ${#synthetic_choices[*]}; j++)); do
    idx=j
    testcase
  done
else
  testcase
fi
rm -rf "${run_ev}"
python plot.py
