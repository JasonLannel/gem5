/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "mem/ruby/network/garnet/RoutingUnit.hh"

#include "base/cast.hh"
#include "base/compiler.hh"
#include "base/random.hh"
#include "debug/RubyNetwork.hh"
#include "debug/jizhou.hh"
#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/OutputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/slicc_interface/Message.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

RoutingUnit::RoutingUnit(Router *router)
{
    m_router = router;
    m_routing_table.clear();
    m_weight_table.clear();
}

void
RoutingUnit::addRoute(std::vector<NetDest>& routing_table_entry)
{
    if (routing_table_entry.size() > m_routing_table.size()) {
        m_routing_table.resize(routing_table_entry.size());
    }
    for (int v = 0; v < routing_table_entry.size(); v++) {
        m_routing_table[v].push_back(routing_table_entry[v]);
    }
}

void
RoutingUnit::addWeight(int link_weight)
{
    m_weight_table.push_back(link_weight);
}

bool
RoutingUnit::supportsVnet(int vnet, std::vector<int> sVnets)
{
    // If all vnets are supported, return true
    if (sVnets.size() == 0) {
        return true;
    }

    // Find the vnet in the vector, return true
    if (std::find(sVnets.begin(), sVnets.end(), vnet) != sVnets.end()) {
        return true;
    }

    // Not supported vnet
    return false;
}

/*
 * This is the default routing algorithm in garnet.
 * The routing table is populated during topology creation.
 * Routes can be biased via weight assignments in the topology file.
 * Correct weight assignments are critical to provide deadlock avoidance.
 */
int
RoutingUnit::lookupRoutingTable(int vnet, NetDest msg_destination)
{
    // First find all possible output link candidates
    // For ordered vnet, just choose the first
    // (to make sure different packets don't choose different routes)
    // For unordered vnet, randomly choose any of the links
    // To have a strict ordering between links, they should be given
    // different weights in the topology file

    int output_link = -1;
    int min_weight = INFINITE_;
    std::vector<int> output_link_candidates;
    int num_candidates = 0;

    // Identify the minimum weight among the candidate output links
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

        if (m_weight_table[link] <= min_weight)
            min_weight = m_weight_table[link];
        }
    }

    // Collect all candidate output links with this minimum weight
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

            if (m_weight_table[link] == min_weight) {
                num_candidates++;
                output_link_candidates.push_back(link);
            }
        }
    }

    if (output_link_candidates.size() == 0) {
        fatal("Fatal Error:: No Route exists from this Router.");
        exit(0);
    }

    // Randomly select any candidate output link
    int candidate = 0;
    if (!(m_router->get_net_ptr())->isVNetOrdered(vnet))
        candidate = rand() % num_candidates;

    output_link = output_link_candidates.at(candidate);
    return output_link;
}


void
RoutingUnit::addInDirection(PortDirection inport_dirn, int inport_idx)
{
    m_inports_dirn2idx[inport_dirn] = inport_idx;
    m_inports_idx2dirn[inport_idx]  = inport_dirn;
}

void
RoutingUnit::addOutDirection(PortDirection outport_dirn, int outport_idx)
{
    m_outports_dirn2idx[outport_dirn] = outport_idx;
    m_outports_idx2dirn[outport_idx]  = outport_dirn;
}

// outportCompute() is called by the InputUnit
// It calls the routing table by default.
// A template for adaptive topology-specific routing algorithm
// implementations using port directions rather than a static routing
// table is provided here.

int
RoutingUnit::outportCompute(RouteInfo route,
							int inport,
							int invc,
                            PortDirection inport_dirn)
{
    int outport = -1;

    if (route.dest_router == m_router->get_id()) {

        // Multiple NIs may be connected to this router,
        // all with output port direction = "Local"
        // Get exact outport id from table
        outport = lookupRoutingTable(route.vnet, route.net_dest);
        return outport;
    }

    // Routing Algorithm set in GarnetNetwork.py
    // Can be over-ridden from command line using --routing-algorithm = 1
    RoutingAlgorithm routing_algorithm =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();

    switch (routing_algorithm) {
        case TABLE_:  outport =
            lookupRoutingTable(route.vnet, route.net_dest); break;
        case XY_:     outport =
            outportComputeXY(route, inport, inport_dirn); break;
		case DETERMINISTIC_: outport =
		    outportComputeDeterministic(route, inport, invc, inport_dirn); break;
        case STATIC_ADAPTIVE_: outport =
            outportComputeStaticAdaptive(route, inport, invc, inport_dirn); break;
        case DYNAMIC_ADAPTIVE_: outport =
            outportComputeDynamicAdaptive(route, inport, invc, inport_dirn); break;
        default: outport =
            lookupRoutingTable(route.vnet, route.net_dest); break;
    }

    assert(outport != -1);
    return outport;
}
// XY routing implemented using port directions
// Only for reference purpose in a Mesh
// By default Garnet uses the routing table
int
RoutingUnit::outportComputeXY(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    [[maybe_unused]] int num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    if (x_hops > 0) {
        if (x_dirn) {
            assert(inport_dirn == "Local" || inport_dirn == "West");
            outport_dirn = "East";
        } else {
            assert(inport_dirn == "Local" || inport_dirn == "East");
            outport_dirn = "West";
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            // "Local" or "South" or "West" or "East"
            assert(inport_dirn != "North");
            outport_dirn = "North";
        } else {
            // "Local" or "North" or "West" or "East"
            assert(inport_dirn != "South");
            outport_dirn = "South";
        }
    } else {
        // x_hops == 0 and y_hops == 0
        // this is not possible
        // already checked that in outportCompute() function
        panic("x_hops == y_hops == 0");
    }

    return m_outports_dirn2idx[outport_dirn];
}

// Virtual channel class :
// Lower channel : 0 ~ vc_per_vnet / 2 - 1
// Upper channel : otherwise
int RoutingUnit::outportComputeDeterministic(RouteInfo route,
                                             int inport,
                                             int invc,
                                             PortDirection inport_dirn)
{
    auto vc_per_vnet = m_router->get_vc_per_vnet();
    assert(vc_per_vnet >= 2);

    int num_ary = m_router->get_net_ptr()->getNumAry();
    int num_dim = m_router->get_net_ptr()->getNumDim();

    int my_id = m_router->get_id();

    int dest_id = route.dest_router;

    assert (my_id != dest_id);

    if (inport_dirn == "Local") {
        int i = 0;
        int my_digit = my_id % num_ary;
        int dest_digit = dest_id % num_ary;
        //DPRINTF(jizhou, "Local my %d dest %d\n", my_id, dest_id);
        for (; i < num_dim; ++i) {
            //DPRINTF(jizhou, "i %d \n", i);
            if (my_digit != dest_digit)
                break;
            my_id /= num_ary;
            dest_id /= num_ary;
            my_digit = my_id % num_ary;
            dest_digit = dest_id % num_ary;
        }
        assert(i >= 0 && i < num_dim);
        //DPRINTF(jizhou, "Local from %d to %d diff %d \n", m_router->get_id(), route.dest_router, i);
        if ((dest_digit > my_digit && dest_digit - my_digit < num_ary / 2) || (dest_digit < my_digit && num_ary - my_digit + dest_digit < num_ary / 2))
            return m_outports_dirn2idx["upper" + std::to_string(i)];
        return m_outports_dirn2idx["lower" + std::to_string(i)];
    }


    // 0 for lower channel, 1 for upper channel
    // Notation c_{dvx} -> n_j
    char v[6];
    int d;
    assert(sscanf(inport_dirn.c_str(), "%5[a-zA-Z]%d", v, &d) == 2);
    assert(d >= 0 && d < num_dim);
    //DPRINTF(jizhou, "Global my %d dest %d d %d \n", my_id, dest_id, d);
    int my_digit;
    int dest_digit;

    for (int i = 1; i <= d; ++i) {
        my_id /= num_ary;
        dest_id /= num_ary;
    }
    my_digit = my_id % num_ary;
    dest_digit = dest_id % num_ary;

    if (my_digit != dest_digit) {
        //DPRINTF(jizhou, "Global from %d to %d diff %d \n", m_router->get_id(), route.dest_router, d);
        if ((dest_digit > my_digit && dest_digit - my_digit < num_ary / 2) || (dest_digit < my_digit && num_ary - my_digit + dest_digit < num_ary / 2))
            return m_outports_dirn2idx["upper" + std::to_string(d)];
        return m_outports_dirn2idx["lower" + std::to_string(d)];
    }
    else {
        int i = d;
        for (; i < num_dim; ++i) {
            if (my_digit != dest_digit)
                break;
            my_id /= num_ary;
            dest_id /= num_ary;
            my_digit = my_id % num_ary;
            dest_digit = dest_id % num_ary;
        }
        assert(i >= 0 && i < num_dim);
        //DPRINTF(jizhou, "Global from %d to %d diff %d \n", m_router->get_id(), route.dest_router, i);
        if ((dest_digit > my_digit && dest_digit - my_digit < num_ary / 2) || (dest_digit < my_digit && num_ary - my_digit + dest_digit < num_ary / 2))
            return m_outports_dirn2idx["upper" + std::to_string(i)];
        return m_outports_dirn2idx["lower" + std::to_string(i)];
    }
}

int RoutingUnit::outVcClassCompute(RouteInfo route, PortDirection inport_dirn) {
    RoutingAlgorithm algo =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();
    if (algo == DETERMINISTIC_){
        int num_ary = m_router->get_net_ptr()->getNumAry();
        int num_dim = m_router->get_net_ptr()->getNumDim();

        int my_id = m_router->get_id();
        int dest_id = route.dest_router;

        int my_digit;
        int dest_digit;

        if (my_id == dest_id)
            return 2;

        char v[6];
        int d;
        if (inport_dirn == "Local") {
            int i = 0;
            my_digit = my_id % num_ary;
            dest_digit = dest_id % num_ary;
            for (; i < num_dim; ++i) {
                if (my_digit != dest_digit)
                    break;
                my_id /= num_ary;
                dest_id /= num_ary;
                my_digit = my_id % num_ary;
                dest_digit = dest_id % num_ary;
            }
            assert(i >= 0 && i < num_dim);
            d = i;
        }
        else {
            assert(sscanf(inport_dirn.c_str(), "%5[a-zA-Z]%d", v, &d) == 2);
            for (int i = 1; i <= d; ++i) {
                my_id /= num_ary;
                dest_id /= num_ary;
            }
            my_digit = my_id % num_ary;
            dest_digit = dest_id % num_ary;
        }

        int outport_class = 1;
        if (my_digit > dest_digit || my_digit == 0) outport_class = 0;

        return outport_class;
    } else {
        panic("%s placeholder executed", __FUNCTION__);
    }
}

int
RoutingUnit::outportComputeStaticAdaptive(RouteInfo route,
                                          int inport,
                                          int invc,
                                          PortDirection inport_dirn)
{
    // STATIC ADAPTIVE
    // 0. Check class now: Deterministic (DR=lim) can only choose deter.
    // 1. Find all unoccupied adaptive channels closest to dest.
    //    If some, pick, DR++ if necessary.
    // 2. Find those adaptive channels permitted to select.
    //    If some, pick, and wait, DR++ if necessary.
    // 3. Pick Deterministic.
    DPRINTF(RubyNetwork, "ENTERING COMPUTE: STATIC ADAPTIVE (%s)\n", inport_dirn);
    auto vc_per_vnet = m_router->get_vc_per_vnet();
    int vnet = invc/vc_per_vnet;
    auto dr = route.dr;
    int misrouting = route.misrouting;
    auto dr_lim = m_router->get_net_ptr()->getDrLim();
    int num_ary = m_router->get_net_ptr()->getNumAry();
    int num_dim = m_router->get_net_ptr()->getNumDim();
    int misrouting_lim = m_router->get_net_ptr()->getMisroutingLim();
    int cur_route_dim = -1;
    if (inport_dirn != "Local") {
        char v[6];
        assert(sscanf(inport_dirn.c_str(), "%5[a-zA-Z]%d", v, &cur_route_dim) == 2);
    }
    std::vector<int> my_dim_id, dest_dim_id;
    my_dim_id.resize(num_dim);
    dest_dim_id.resize(num_dim);
    for (int i = 0, my_id = m_router->get_id(), dest_id = route.dest_router; i < num_dim; ++i) {
        my_dim_id[i] = my_id % num_ary;
        dest_dim_id[i] = dest_id % num_ary;
        my_id /= num_ary;
        dest_id /= num_ary;
    }

    assert(m_router->get_id() != route.dest_router);
    if (dr < dr_lim) {
        // Legal ports with shortest path
        std::vector<int> sp_dims_legal;
        std::vector<int> sp_outports_legal;
        std::vector<int> sp_vc_classes_legal;
        // All legal ports
        std::vector<int> dims_legal;
        std::vector<int> outports_legal;
        std::vector<int> vc_classes_legal;
        for (int i = 0; i < num_dim; ++i) {
            if ((cur_route_dim <= i) || (dr + 1 < dr_lim)) {
                int vc_class = 1;
                if (inport_dirn == "Local" || cur_route_dim > i) {
                    vc_class = 2;
                } else if (cur_route_dim == i && (my_dim_id[i] > dest_dim_id[i] || my_dim_id[i] == 0)) {
                    vc_class = 0;
                }
                if (cur_route_dim > i) {
                    vc_class += 3 * (dr + 1);    // Any can be chosen.
                } else {
                    vc_class += 3 * dr;
                }
                int outports_i[2] = {m_outports_dirn2idx["lower"+std::to_string(i)], m_outports_dirn2idx["upper"+std::to_string(i)]};
                int diff = (num_ary + dest_dim_id[i] - my_dim_id[i]) % num_ary;
                int dir_i = ((diff > 0) &&
                    (diff < num_ary / 2)) ? 1 : 0;
                
                if (my_dim_id[i] != dest_dim_id[i] && m_router->getOutputUnit(outports_i[dir_i])
                            ->has_legal_vc(vnet, vc_class, dr, STATIC_ADAPTIVE_)) {
                    sp_outports_legal.push_back(outports_i[dir_i]);
                    sp_dims_legal.push_back(i);
                    sp_vc_classes_legal.push_back(vc_class);
                    outports_legal.push_back(outports_i[dir_i]);
                    dims_legal.push_back(i);
                    vc_classes_legal.push_back(vc_class);
                }
                if (misrouting < misrouting_lim && cur_route_dim != i){
                    dir_i = (dir_i + 1) % 2;
                    if (cur_route_dim > i) {
                        vc_class = 3 * (dr + 1) + 2;   
                    } else {
                        vc_class = 3 * dr + 1;
                    }
                    if (my_dim_id[i] != dest_dim_id[i] && m_router->getOutputUnit(outports_i[dir_i])
                            ->has_legal_vc(vnet, vc_class, dr, STATIC_ADAPTIVE_)) {
                        outports_legal.push_back(outports_i[dir_i]);
                        dims_legal.push_back(i);
                        vc_classes_legal.push_back(vc_class);
                    } 
                    if (my_dim_id[i] == dest_dim_id[i]) {
                        if (m_router->getOutputUnit(outports_i[0])
                            ->has_legal_vc(vnet, vc_class, dr, STATIC_ADAPTIVE_)) {
                            outports_legal.push_back(outports_i[0]);
                            dims_legal.push_back(i);
                            vc_classes_legal.push_back(vc_class);
                        }
                        if (m_router->getOutputUnit(outports_i[1])
                            ->has_legal_vc(vnet, vc_class, dr, STATIC_ADAPTIVE_)) {
                            outports_legal.push_back(outports_i[1]);
                            dims_legal.push_back(i);
                            vc_classes_legal.push_back(vc_class);
                        }
                    }
                }
                
                
            }
        }
        DPRINTF(RubyNetwork, "FINISH LEGAL\n");
        if (!outports_legal.empty()) {
            // Pick.
            std::vector<int> sp_dims_free;
            std::vector<int> sp_outports_free;
            std::vector<int> sp_vc_classes_free;
            assert(vnet < m_router->get_num_vnets());
            for (int i = 0; i < sp_outports_legal.size(); ++i) {
                if (m_router->getOutputUnit(sp_outports_legal[i])
                            ->has_free_vc(vnet, sp_vc_classes_legal[i], STATIC_ADAPTIVE_)) {
                    sp_dims_free.push_back(sp_dims_legal[i]);
                    sp_outports_free.push_back(sp_outports_legal[i]);
                    sp_vc_classes_free.push_back(sp_vc_classes_legal[i]);
                }
            }
            DPRINTF(RubyNetwork, "FINISH FREE\n");
            int pick_outport_idx;
            int outport, outvc_class, outvc;
            if (!sp_outports_free.empty()) {
                pick_outport_idx = pickFreeOutport(sp_dims_free,
                                                   sp_outports_free,
                                                   sp_vc_classes_free,
                                                   vnet,
                                                   cur_route_dim);
                outport = sp_outports_free[pick_outport_idx];
                outvc_class = sp_vc_classes_free[pick_outport_idx];
                DPRINTF(RubyNetwork, "Output(FREE): %s %d\n", m_router->getOutportDirection(outport), outvc_class);
                outvc = m_router->getOutputUnit(outport)
                                ->select_free_vc(vnet, outvc_class, STATIC_ADAPTIVE_);
                DPRINTF(RubyNetwork, "Output(FREE): %s %d %d\n", m_router->getOutportDirection(outport), outvc_class, outvc);
                // OCCUPY IMMEDIATELY
                assert(outvc != -1);
                m_router->getInputUnit(inport)->grant_outvc(invc, outvc);
            } else {
                // Try mis routing or wait
                
                pick_outport_idx = pickLegalOutport(dims_legal,
                                               outports_legal,
                                               vc_classes_legal,
                                               vnet,
                                               cur_route_dim,
                                               dr);
                outport = outports_legal[pick_outport_idx];
                outvc_class = vc_classes_legal[pick_outport_idx];
                outvc = m_router->getOutputUnit(outport)
                                ->select_free_vc(vnet, outvc_class, STATIC_ADAPTIVE_);
                if (outvc != -1) {
                    m_router->getInputUnit(inport)->grant_outvc(invc, outvc);
                }
                else {
                    outvc = m_router->getOutputUnit(outport)
                                ->select_legal_vc(vnet, outvc_class, dr, STATIC_ADAPTIVE_);
                }
                assert(outvc != -1);
            }
            m_router->getInputUnit(inport)
                    ->grant_outvc_class(invc, outvc_class);
            m_router->getOutputUnit(outport)
                    ->enqueueWaitingQueue(outvc, inport, invc, dr);
            DPRINTF(RubyNetwork, "END ROUTING: STATIC ADAPTIVE\n");
            return outport;
        }
    }
    DPRINTF(RubyNetwork, "FALL BACK TO DETERMINISTIC\n");
    // Fall back to deterministic
    int outport_class = 3 * dr_lim + 1;
    int i = 0;
    for (; i < num_dim; ++i)
        if (my_dim_id[i] != dest_dim_id[i])
            break;
    assert(i >= 0 && i < num_dim);
    if (i == cur_route_dim && (my_dim_id[i] > dest_dim_id[i] || my_dim_id[i] == 0))
        outport_class = 3 * dr_lim + 0;

    m_router->getInputUnit(inport)->grant_outvc_class(invc, outport_class);
    int diff = (num_ary + dest_dim_id[i] - my_dim_id[i]) % num_ary;
    if ((diff > 0) &&
        (diff < num_ary / 2)) {
        return m_outports_dirn2idx["upper"+std::to_string(i)];
    } else {
        return m_outports_dirn2idx["lower"+std::to_string(i)];
    }
}

int RoutingUnit::outportComputeDynamicAdaptive(RouteInfo route,
                                               int inport,
                                               int invc,
                                               PortDirection inport_dirn)
{
    // DYNAMIC ADAPTIVE
    // 0. Check class now: Deterministic can only choose deter.
    // 1. Find all unoccupied adaptive channels closest to dest.
    //    If Throttling: DR >= 0/1;
    //    If some, pick, DR++ if necessary.
    // 2. Find those adaptive channels permitted to select.
    //    In dynamic, permit (can wait) only if my_DR < your_DR.
    //    If Throttling: DR >= 0/1;
    //    If some, pick, and wait, DR++ if necessary.
    // 3. Pick Deterministic.
    auto vc_per_vnet = m_router->get_vc_per_vnet();
    int vnet = invc/vc_per_vnet;
    auto dr = route.dr;
    auto dr_lim = m_router->get_net_ptr()->getDrLim();
    int num_ary = m_router->get_net_ptr()->getNumAry();
    int num_dim = m_router->get_net_ptr()->getNumDim();
    int cur_route_dim = -1;
    int cur_vc_level = m_router->get_vc_level(0, DYNAMIC_ADAPTIVE_);    // No Throttling => No Level 0
    if (inport_dirn != "Local") {
        char v[6];
        assert(sscanf(inport_dirn.c_str(), "%5[a-zA-Z]%d", v, &cur_route_dim) == 2);
        cur_vc_level = m_router->get_vc_level(invc, DYNAMIC_ADAPTIVE_);
    }
    std::vector<int> my_dim_id, dest_dim_id;
    my_dim_id.resize(num_dim);
    dest_dim_id.resize(num_dim);
    for (int i = 0, my_id = m_router->get_id(), dest_id = route.dest_router; i < num_dim; ++i) {
        my_dim_id[i] = my_id % num_ary;
        dest_dim_id[i] = dest_id % num_ary;
        my_id /= num_ary;
        dest_id /= num_ary;
    }

    assert(m_router->get_id() != route.dest_router);
    // vc class 2 for deterministic, no entry to adaptive levels.
    if (cur_vc_level < 2) {
        std::vector<int> sp_dims_legal;
        std::vector<int> sp_outports_legal;
        std::vector<int> sp_vc_classes_legal;
        for (int i = 0; i < num_dim; ++i) {
            if (my_dim_id[i] == dest_dim_id[i]) {
                continue;
            }
            if ((cur_route_dim <= i) || (dr + 1 < dr_lim)) {
                int vc_class;
                if (inport_dirn == "Local") {
                    vc_class = 3 * cur_vc_level + 2;
                } else if (cur_route_dim > i) {
                    vc_class = 3 * 1 + 2;
                } else if (cur_route_dim == i && (my_dim_id[i] > dest_dim_id[i] || my_dim_id[i] == 0)) {
                    vc_class = 3 * cur_vc_level + 0;
                } else {
                    vc_class = 3 * cur_vc_level + 1;
                }
                int outport_i;
                int diff = (num_ary + dest_dim_id[i] - my_dim_id[i]) % num_ary;
                if ((diff > 0) &&
                    (diff < num_ary / 2)) {
                    outport_i = m_outports_dirn2idx["upper"+std::to_string(i)];
                } else {
                    outport_i = m_outports_dirn2idx["lower"+std::to_string(i)];
                }
                if (m_router->getOutputUnit(outport_i)
                            ->has_legal_vc(vnet, vc_class, dr, DYNAMIC_ADAPTIVE_)) {
                    sp_outports_legal.push_back(outport_i);
                    sp_dims_legal.push_back(i);
                    sp_vc_classes_legal.push_back(vc_class);
                }
            }
        }
        if (!sp_outports_legal.empty()) {
            // Pick.
            std::vector<int> sp_dims_free;
            std::vector<int> sp_outports_free;
            std::vector<int> sp_vc_classes_free;
            int vnet = invc/vc_per_vnet;
            assert(vnet < m_router->get_num_vnets());
            for (int i = 0; i < sp_outports_legal.size(); ++i) {
                if (m_router->getOutputUnit(sp_outports_legal[i])
                            ->has_free_vc(vnet, sp_vc_classes_legal[i], DYNAMIC_ADAPTIVE_)) {
                    sp_dims_free.push_back(sp_dims_legal[i]);
                    sp_outports_free.push_back(sp_outports_legal[i]);
                    sp_vc_classes_free.push_back(sp_vc_classes_legal[i]);
                }
            }
            int pick_outport_idx;
            int outport, outvc_class, outvc;
            if (!sp_outports_free.empty()) {
                pick_outport_idx = pickFreeOutport(sp_dims_free,
                                                   sp_outports_free,
                                                   sp_vc_classes_free,
                                                   vnet,
                                                   cur_route_dim);
                outport = sp_outports_free[pick_outport_idx];
                outvc_class = sp_vc_classes_free[pick_outport_idx];
                outvc = m_router->getOutputUnit(outport)
                                ->select_free_vc(vnet, outvc_class, DYNAMIC_ADAPTIVE_);
                // OCCUPY IMMEDIATELY
                assert(outvc != -1);
                m_router->getInputUnit(inport)->grant_outvc(invc, outvc);
            } else {
                pick_outport_idx = pickLegalOutport(sp_dims_legal,
                                                   sp_outports_legal,
                                                   sp_vc_classes_legal,
                                                   vnet,
                                                   cur_route_dim,
                                                   dr);
                outport = sp_outports_legal[pick_outport_idx];
                outvc_class = sp_vc_classes_legal[pick_outport_idx];
                outvc = m_router->getOutputUnit(outport)
                                ->select_legal_vc(vnet, outvc_class, dr, DYNAMIC_ADAPTIVE_);
                assert(outvc != -1);
                // WAIT TILL RELEASE
            }
            m_router->getInputUnit(inport)
                    ->grant_outvc_class(invc, outvc_class);
            m_router->getOutputUnit(outport)
                    ->enqueueWaitingQueue(outvc, inport, invc, dr);
            return outport;
        }
    }
    // Fall back to deterministic
    int outport_class = 3 * 2 + 1;
    int i = 0;
    for (; i < num_dim; ++i)
        if (my_dim_id[i] != dest_dim_id[i])
            break;
    assert(i >= 0 && i < num_dim);
    if (i == cur_route_dim && (my_dim_id[i] > dest_dim_id[i] || my_dim_id[i] == 0))
        outport_class = 3 * 2 + 0;

    m_router->getInputUnit(inport)->grant_outvc_class(invc, outport_class);
    int diff = (num_ary + dest_dim_id[i] - my_dim_id[i]) % num_ary;
    if ((diff > 0) &&
        (diff < num_ary / 2)) {
        return m_outports_dirn2idx["upper"+std::to_string(i)];
    } else {
        return m_outports_dirn2idx["lower"+std::to_string(i)];
    }
}

int RoutingUnit::pickFreeOutport(std::vector<int> dims,
                                 std::vector<int> outports,
                                 std::vector<int> vc_classes,
                                 int vnet,
                                 int cur_dim)
{
    assert(!dims.empty());
    assert(!outports.empty());
    assert(!vc_classes.empty());
    PickAlgorithm pick_algorithm =
        (PickAlgorithm) m_router->get_net_ptr()->getPickAlgorithm();
    if (pick_algorithm == MINIMUM_CONGESTION_) {
        RoutingAlgorithm routing_algorithm =
            (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();
        int maximal_free_vcs = 0;
        for (int i = 0; i < outports.size(); ++i) {
            maximal_free_vcs = std::max(
                maximal_free_vcs,
                m_router->getOutputUnit(outports[i])
                        ->get_free_vc_count(vnet, vc_classes[i], routing_algorithm));
        }
        std::vector<int> pick_pool;
        for (int i = 0; i < outports.size(); ++i) {
            if (maximal_free_vcs ==
                m_router->getOutputUnit(outports[i])
                        ->get_free_vc_count(vnet, vc_classes[i], routing_algorithm)) {
                pick_pool.push_back(i);
            }
        }
        int lottery = random_mt.random<unsigned>(0, pick_pool.size() - 1);
        return pick_pool[lottery];
    } else if (pick_algorithm == STRAIGHT_LINES_) {
        int idx_1 = std::upper_bound(dims.begin(), dims.end(), cur_dim) - dims.begin();
        int idx_2 = idx_1 - 1;
        if (idx_1 == dims.size()) {
            return idx_2;
        } else if (idx_2 < 0) {
            return idx_1;
        } else if (cur_dim - dims[idx_2] != dims[idx_1] - cur_dim){
            return cur_dim - dims[idx_2] < dims[idx_1] - cur_dim ? idx_2 : idx_1;
        } else {
            return random_mt.random<unsigned>(0, 1) ? idx_1 : idx_2;
        }
    } else if (pick_algorithm == RANDOM_) {
        return random_mt.random<unsigned>(0, outports.size() - 1);
    } else {
        panic("%s placeholder executed", __FUNCTION__);
    }
}

int RoutingUnit::pickLegalOutport(std::vector<int> dims,
                                 std::vector<int> outports,
                                 std::vector<int> vc_classes,
                                 int vnet,
                                 int cur_dim,
                                 uint32_t dr)
{
    assert(!dims.empty());
    assert(!outports.empty());
    assert(!vc_classes.empty());
    PickAlgorithm pick_algorithm =
        (PickAlgorithm) m_router->get_net_ptr()->getPickAlgorithm();
    if (pick_algorithm == MINIMUM_CONGESTION_) {
        RoutingAlgorithm routing_algorithm =
            (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();
        if (routing_algorithm != STATIC_ADAPTIVE_ && routing_algorithm != DYNAMIC_ADAPTIVE_) {
            return random_mt.random<unsigned>(0, outports.size() - 1);
        } else {
            int min_waiting_len = 0;
            for (int i = 0; i < outports.size(); ++i) {
                min_waiting_len = std::max(
                    min_waiting_len,
                    m_router->getOutputUnit(outports[i])
                            ->get_min_waiting_length(vnet, vc_classes[i], dr, routing_algorithm));
            }
            std::vector<int> pick_pool;
            for (int i = 0; i < outports.size(); ++i) {
                if (min_waiting_len ==
                    m_router->getOutputUnit(outports[i])
                            ->get_min_waiting_length(vnet, vc_classes[i], dr, routing_algorithm)) {
                    pick_pool.push_back(i);
                }
            }
            int lottery = random_mt.random<unsigned>(0, pick_pool.size() - 1);
            return pick_pool[lottery];
            return random_mt.random<unsigned>(0, outports.size() - 1);
        }
    } else if (pick_algorithm == STRAIGHT_LINES_) {
        int idx_1 = std::upper_bound(dims.begin(), dims.end(), cur_dim) - dims.begin();
        int idx_2 = idx_1 - 1;
        if (idx_1 == dims.size()) {
            return idx_2;
        } else if (idx_2 < 0) {
            return idx_1;
        } else if (cur_dim - dims[idx_2] != dims[idx_1] - cur_dim){
            return cur_dim - dims[idx_2] < dims[idx_1] - cur_dim ? idx_2 : idx_1;
        } else {
            return random_mt.random<unsigned>(0, 1) ? idx_1 : idx_2;
        }
    } else if (pick_algorithm == RANDOM_) {
        return random_mt.random<unsigned>(0, outports.size() - 1);
    } else {
        panic("%s placeholder executed", __FUNCTION__);
    }
}


} // namespace garnet
} // namespace ruby
} // namespace gem5
