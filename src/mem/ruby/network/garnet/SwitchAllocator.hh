/*
 * Copyright (c) 2020 Inria
 * Copyright (c) 2016 Georgia Institute of Technology
 * Copyright (c) 2008 Princeton University
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


#ifndef __MEM_RUBY_NETWORK_GARNET_0_SWITCHALLOCATOR_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_SWITCHALLOCATOR_HH__

#include <iostream>
#include <vector>

#include "mem/ruby/common/Consumer.hh"
#include "mem/ruby/network/garnet/CommonTypes.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

class Router;
class InputUnit;
class OutputUnit;

class SwitchAllocator : public Consumer
{
  public:
    SwitchAllocator(Router *router);
    void wakeup();
    void init();
    void clear_request_vector();
    void check_for_wakeup();
    int get_vnet (int invc);
    void print(std::ostream& out) const {};
    void arbitrate_inports();
    void arbitrate_outports();
    bool send_allowed(int inport, int invc, int outport, int outvc);
    int vc_allocate(int outport, int inport, int invc);

    inline double
    get_input_arbiter_activity()
    {
        return m_input_arbiter_activity;
    }
    inline double
    get_output_arbiter_activity()
    {
        return m_output_arbiter_activity;
    }

    void resetStats();

    bool is_dimension_reversal(int inport, int outport);
    bool is_misrouting(int inport, int outport, int dest_id);

  private:
    int m_num_inports, m_num_outports;
    int m_num_vcs, m_vc_per_vnet;

    double m_input_arbiter_activity, m_output_arbiter_activity;

    Router *m_router;
    std::vector<int> m_round_robin_invc;
    std::vector<int> m_round_robin_inport;
    std::vector<int> m_port_requests;
    std::vector<int> m_vc_winners;
};

} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_SWITCHALLOCATOR_HH__
