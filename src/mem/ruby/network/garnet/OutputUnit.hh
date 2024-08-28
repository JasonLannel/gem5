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


#ifndef __MEM_RUBY_NETWORK_GARNET_0_OUTPUTUNIT_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_OUTPUTUNIT_HH__

#include <iostream>
#include <vector>

#include "base/compiler.hh"
#include "mem/ruby/common/Consumer.hh"
#include "mem/ruby/network/garnet/CommonTypes.hh"
#include "mem/ruby/network/garnet/NetworkLink.hh"
#include "mem/ruby/network/garnet/OutVcState.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

class CreditLink;
class Router;

class OutputUnit : public Consumer
{
  public:
    OutputUnit(int id, PortDirection direction, Router *router,
               uint32_t consumerVcs);
    ~OutputUnit() = default;
    void set_out_link(NetworkLink *link);
    void set_credit_link(CreditLink *credit_link);
    void wakeup();
    flitBuffer* getOutQueue();
    void print(std::ostream& out) const {};
    void decrement_credit(int out_vc);
    void increment_credit(int out_vc);
    bool has_credit(int out_vc);
    bool has_free_vc(int vnet, int outvc_class, RoutingAlgorithm ra);
    int select_free_vc(int vnet, int outvc_class, RoutingAlgorithm ra);
    int get_free_vc_count(int vnet, int outvc_class, RoutingAlgorithm ra);
    bool is_legal(int out_vc, uint32_t dr, RoutingAlgorithm ra);
    bool has_legal_vc(int vnet, int outvc_class, uint32_t dr, RoutingAlgorithm ra);
    int get_legal_vc_count(int vnet, int outvc_class, uint32_t dr, RoutingAlgorithm ra);
    int select_legal_vc(int vnet, int outvc_class, uint32_t dr, RoutingAlgorithm ra);

    inline PortDirection get_direction() { return m_direction; }

    int
    get_credit_count(int vc)
    {
        return outVcState[vc].get_credit_count();
    }

    inline int
    get_outlink_id()
    {
        return m_out_link->get_id();
    }

    inline void
    set_vc_state(VC_state_type state, int vc, Tick curTime)
    {
      outVcState[vc].setState(state, curTime);
    }

    inline bool
    is_vc_idle(int vc, Tick curTime)
    {
        return (outVcState[vc].isInState(IDLE_, curTime));
    }

    void insert_flit(flit *t_flit);

    inline int
    getVcsPerVnet()
    {
        return m_vc_per_vnet;
    }

    bool functionalRead(Packet *pkt, WriteMask &mask);
    uint32_t functionalWrite(Packet *pkt);

    class WaitingQueue {
        public:
          WaitingQueue() {
            waitingQueue.clear(); channel_dr = 0;
          }
          inline int size() {
              return waitingQueue.size();
          }

          inline bool empty() {
              return waitingQueue.empty();
          }

          inline uint32_t getDr() {
              return channel_dr;
          }

          inline void enqueue(int inport, int invc, uint32_t dr) {
              channel_dr = dr;
              waitingQueue.push_back(std::make_pair(inport, invc));
          }

          inline std::pair<int, int> peek() {
              if (empty()) {
                  return std::make_pair(-1, -1);
              }
              return waitingQueue.front();
          }

          inline void dequeue() {
              if (!empty()) {
                  waitingQueue.pop_front();
              }
          }

        private:
          std::deque<std::pair<int, int>> waitingQueue;
          uint32_t channel_dr;
    };

    inline void enqueueWaitingQueue(int outvc, int inport, int invc, uint32_t dr) {
        waitingQueues[outvc].enqueue(inport, invc, dr);
    }

  private:
    Router *m_router;
    GEM5_CLASS_VAR_USED int m_id;
    PortDirection m_direction;
    int m_vc_per_vnet;
    NetworkLink *m_out_link;
    CreditLink *m_credit_link;

    // This is for the network link to consume
    flitBuffer outBuffer;
    // vc state of downstream router
    std::vector<OutVcState> outVcState;

    // waiting queue of outvcs (used in adaptive algo)
    std::vector<WaitingQueue> waitingQueues;
};

} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_OUTPUTUNIT_HH__
