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


#include "mem/ruby/network/garnet/OutputUnit.hh"

#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/garnet/Credit.hh"
#include "mem/ruby/network/garnet/CreditLink.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/flitBuffer.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

OutputUnit::OutputUnit(int id, PortDirection direction, Router *router,
  uint32_t consumerVcs)
  : Consumer(router), m_router(router), m_id(id), m_direction(direction),
    m_vc_per_vnet(consumerVcs)
{
    const int m_num_vcs = consumerVcs * m_router->get_num_vnets();
    outVcState.reserve(m_num_vcs);
    for (int i = 0; i < m_num_vcs; i++) {
        outVcState.emplace_back(i, m_router->get_net_ptr(), consumerVcs);
    }
    waitingQueues.reserve(m_num_vcs);
    for (int i = 0; i < m_num_vcs; i++) {
        waitingQueues.emplace_back();
    }
}

void
OutputUnit::decrement_credit(int out_vc)
{
    DPRINTF(RubyNetwork, "Router %d OutputUnit %s decrementing credit:%d for "
            "outvc %d at time: %lld for %s\n", m_router->get_id(),
            m_router->getPortDirectionName(get_direction()),
            outVcState[out_vc].get_credit_count(),
            out_vc, m_router->curCycle(), m_credit_link->name());

    outVcState[out_vc].decrement_credit();
}

void
OutputUnit::increment_credit(int out_vc)
{
    DPRINTF(RubyNetwork, "Router %d OutputUnit %s incrementing credit:%d for "
            "outvc %d at time: %lld from:%s\n", m_router->get_id(),
            m_router->getPortDirectionName(get_direction()),
            outVcState[out_vc].get_credit_count(),
            out_vc, m_router->curCycle(), m_credit_link->name());

    outVcState[out_vc].increment_credit();
}

// Check if the output VC (i.e., input VC at next router)
// has free credits (i..e, buffer slots).
// This is tracked by OutVcState
bool
OutputUnit::has_credit(int out_vc)
{
    assert(outVcState[out_vc].isInState(ACTIVE_, curTick()));
    return outVcState[out_vc].has_credit();
}


// Check if the output port (i.e., input port at next router) has free VCs.

bool
OutputUnit::has_free_vc(int vnet, int outvc_class, RoutingAlgorithm ra)
{
    int vc_base = vnet*m_vc_per_vnet;
    auto range = m_router->get_vc_range(outvc_class, ra);
    for (int vc = vc_base + range.first; vc < vc_base + range.second; vc++) {
        if (is_vc_idle(vc, curTick()) && waitingQueues[vc].empty())
            return true;
    }

    return false;
}

// Assign a free output VC to the winner of Switch Allocation
int
OutputUnit::select_free_vc(int vnet, int outvc_class, RoutingAlgorithm ra)
{
    int vc_base = vnet*m_vc_per_vnet;
    auto range = m_router->get_vc_range(outvc_class, ra);
    for (int vc = vc_base + range.first; vc < vc_base + range.second; vc++) {
        if (is_vc_idle(vc, curTick()) && waitingQueues[vc].empty()) {
            outVcState[vc].setState(ACTIVE_, curTick());
            return vc;
        }
    }

    return -1;
}

// Get free vc count for given vc class
// Adaptive: We will grant free vc to those waiting, so no conflict.
int
OutputUnit::get_free_vc_count(int vnet, int outvc_class, RoutingAlgorithm ra)
{
    int vc_base = vnet*m_vc_per_vnet;
    auto range = m_router->get_vc_range(outvc_class, ra);
    int cnt = 0;
    for (int vc = vc_base + range.first; vc < vc_base + range.second; vc++) {
        if (is_vc_idle(vc, curTick()) && waitingQueues[vc].empty()) {
            ++cnt;
        }
    }
    return cnt;
}

bool
OutputUnit::is_legal(int out_vc, uint32_t dr, RoutingAlgorithm ra) {
    assert(ra == DYNAMIC_ADAPTIVE_ || ra == STATIC_ADAPTIVE_);
    if ((ra == STATIC_ADAPTIVE_) || (waitingQueues[out_vc].empty())) {
        return true;
    } else if (waitingQueues[out_vc].getDr() > dr) {
        return true;
    } else {
        return false;
    }
}

bool
OutputUnit::has_legal_vc(int vnet, int outvc_class, uint32_t dr, RoutingAlgorithm ra)
{
    assert(ra == DYNAMIC_ADAPTIVE_ || ra == STATIC_ADAPTIVE_);
    int vc_base = vnet*m_vc_per_vnet;
    auto range = m_router->get_vc_range(outvc_class, ra);
    for (int vc = vc_base + range.first; vc < vc_base + range.second; vc++) {
        if (is_legal(vc, dr, ra)) {
            return true;
        }
    }
    return false;
}

int
OutputUnit::select_legal_vc(int vnet, int outvc_class, uint32_t dr, RoutingAlgorithm ra)
{
    assert(ra == DYNAMIC_ADAPTIVE_ || ra == STATIC_ADAPTIVE_);
    int vc_base = vnet*m_vc_per_vnet;
    auto range = m_router->get_vc_range(outvc_class, ra);
    int min_waiting_len = 0x7fffffff;
    for (int vc = vc_base + range.first; vc < vc_base + range.second; vc++) {
        if (is_legal(vc, dr, ra)) {
            min_waiting_len = std::min(min_waiting_len, waitingQueues[vc].size());
        }
    }
    for (int vc = vc_base + range.first; vc < vc_base + range.second; vc++) {
        if (is_legal(vc, dr, ra) && min_waiting_len == waitingQueues[vc].size()) {
            return vc;
        }
    }
    return -1;
}

int
OutputUnit::get_min_waiting_length(int vnet, int outvc_class, uint32_t dr, RoutingAlgorithm ra)
{
    assert(ra == DYNAMIC_ADAPTIVE_ || ra == STATIC_ADAPTIVE_);
    int vc_base = vnet*m_vc_per_vnet;
    auto range = m_router->get_vc_range(outvc_class, ra);
    int min_waiting_len = 0x7fffffff;
    for (int vc = vc_base + range.first; vc < vc_base + range.second; vc++) {
        if (is_legal(vc, dr, ra)) {
            min_waiting_len = std::min(min_waiting_len, waitingQueues[vc].size());
        }
    }
    return min_waiting_len;
}

/*
 * The wakeup function of the OutputUnit reads the credit signal from the
 * downstream router for the output VC (i.e., input VC at downstream router).
 * It increments the credit count in the appropriate output VC state.
 * If the credit carries is_free_signal as true,
 * the output VC is marked IDLE.
 */

void
OutputUnit::wakeup()
{
    {
        std::string s;
        for (int i = 0; i < waitingQueues.size(); ++i) {
            s += std::to_string(waitingQueues[i].size()) + ' ';
        }
        DPRINTF(RubyNetwork, "Router %d OutputUnit %s Waiting Queues Status (BEF): %s\n", m_router->get_id(),
            m_router->getPortDirectionName(get_direction()), s);
    }
    if (m_credit_link->isReady(curTick())) {
        Credit *t_credit = (Credit*) m_credit_link->consumeLink();
        increment_credit(t_credit->get_vc());

        if (t_credit->is_free_signal()) {
            int vc = t_credit->get_vc();
            set_vc_state(IDLE_, vc, curTick());
            if (!waitingQueues[vc].empty()) {
                waitingQueues[vc].dequeue();
            }
            if (waitingQueues[vc].empty()) {
                DPRINTF(RubyNetwork, "FREE CHANNEL\n");
            } else {
                auto invc_info = waitingQueues[vc].peek();
                assert(invc_info.first != -1);
                set_vc_state(ACTIVE_, vc, curTick());
                m_router->getInputUnit(invc_info.first)->grant_outvc(invc_info.second, vc);
                DPRINTF(RubyNetwork, "REACTIVATE CHANNEL: %s %d (WITH LENGTH %d)\n", m_router->getInportDirection(invc_info.first), invc_info.second, waitingQueues[vc].size());
            }
        }

        delete t_credit;

        if (m_credit_link->isReady(curTick())) {
            scheduleEvent(Cycles(1));
        }
    }

    {
        std::string s;
        for (int i = 0; i < waitingQueues.size(); ++i) {
            s += std::to_string(waitingQueues[i].size()) + ' ';
        }
        DPRINTF(RubyNetwork, "Router %d OutputUnit %s Waiting Queues Status (AFT): %s\n", m_router->get_id(),
            m_router->getPortDirectionName(get_direction()), s);
    }
}

flitBuffer*
OutputUnit::getOutQueue()
{
    return &outBuffer;
}

void
OutputUnit::set_out_link(NetworkLink *link)
{
    m_out_link = link;
}

void
OutputUnit::set_credit_link(CreditLink *credit_link)
{
    m_credit_link = credit_link;
}

void
OutputUnit::insert_flit(flit *t_flit)
{
    outBuffer.insert(t_flit);
    m_out_link->scheduleEventAbsolute(m_router->clockEdge(Cycles(1)));
}

bool
OutputUnit::functionalRead(Packet *pkt, WriteMask &mask)
{
    return outBuffer.functionalRead(pkt, mask);
}

uint32_t
OutputUnit::functionalWrite(Packet *pkt)
{
    return outBuffer.functionalWrite(pkt);
}

} // namespace garnet
} // namespace ruby
} // namespace gem5
