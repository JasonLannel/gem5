# Copyright (c) 2008 Princeton University
# Copyright (c) 2009 Advanced Micro Devices, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Author: Tushar Krishna
#

from m5.params import *
from m5.proxy import *
from m5.objects.Network import RubyNetwork
from m5.objects.BasicRouter import BasicRouter
from m5.objects.ClockedObject import ClockedObject


class GarnetNetwork(RubyNetwork):
    type = "GarnetNetwork"
    cxx_header = "mem/ruby/network/garnet/GarnetNetwork.hh"
    cxx_class = "gem5::ruby::garnet::GarnetNetwork"

    num_rows = Param.Int(0, "number of rows if 2D (mesh/torus/..) topology")
    num_ary = Param.Int(
        0, "number of nodes in each dimension if *k*-ary n-dim topology"
    )
    num_dim = Param.Int(0, "number of dimension if k-ary *n*-dim topology")
    ni_flit_size = Param.UInt32(16, "network interface flit size in bytes")
    vcs_per_vnet = Param.UInt32(4, "virtual channels per virtual network")
    buffers_per_data_vc = Param.UInt32(4, "buffers per data virtual channel")
    buffers_per_ctrl_vc = Param.UInt32(1, "buffers per ctrl virtual channel")
    routing_algorithm = Param.Int(
        0,
        "0: Weight-based Table, 1: XY, 2: Deterministic, "
        "3: Static Adaptive, 4: Dynamic Adaptive",
    )
    pick_algorithm = Param.Int(
        0,
        "0: Random, 1: Minimum Congestion, 2: Straight Lines",
    )
    dr_lim = Param.UInt32(
        0,
        "maximal dimension reversal number for static adaptive routing, default 0",
    )
    misrouting_lim = Param.Int(
        0,
        "maximal misrouting number for adaptive routing, default 0",
    )
    throttling_degree = Param.UInt32(
        0,
        "vcs for throttling class (0) for dynamic adaptive routing, default 0",
    )
    vcs_adaptive = Param.UInt32(4, "virtual channels for adaptive class")
    enable_fault_model = Param.Bool(False, "enable network fault model")
    fault_model = Param.FaultModel(NULL, "network fault model")
    garnet_deadlock_threshold = Param.UInt32(
        50000, "network-level deadlock threshold"
    )


class GarnetNetworkInterface(ClockedObject):
    type = "GarnetNetworkInterface"
    cxx_class = "gem5::ruby::garnet::NetworkInterface"
    cxx_header = "mem/ruby/network/garnet/NetworkInterface.hh"

    id = Param.UInt32("ID in relation to other network interfaces")
    vcs_per_vnet = Param.UInt32(
        Parent.vcs_per_vnet, "virtual channels per virtual network"
    )
    virt_nets = Param.UInt32(
        Parent.number_of_virtual_networks, "number of virtual networks"
    )
    garnet_deadlock_threshold = Param.UInt32(
        Parent.garnet_deadlock_threshold, "network-level deadlock threshold"
    )


class GarnetRouter(BasicRouter):
    type = "GarnetRouter"
    cxx_class = "gem5::ruby::garnet::Router"
    cxx_header = "mem/ruby/network/garnet/Router.hh"
    vcs_per_vnet = Param.UInt32(
        Parent.vcs_per_vnet, "virtual channels per virtual network"
    )
    virt_nets = Param.UInt32(
        Parent.number_of_virtual_networks, "number of virtual networks"
    )
    width = Param.UInt32(
        Parent.ni_flit_size, "bit width supported by the router"
    )
