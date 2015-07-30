/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_DAEMON_FW_FORWARDER_HPP
#define NFD_DAEMON_FW_FORWARDER_HPP

#include "common.hpp"
#include "core/scheduler.hpp"
#include "forwarder-counters.hpp"
#include "face-table.hpp"
#include "table/fib.hpp"
#include "table/pit.hpp"
#include "table/cs.hpp"
#include "table/measurements.hpp"
#include "table/strategy-choice.hpp"
#include "table/dead-nonce-list.hpp"

#include "ns3/ndnSIM/model/cs/ndn-content-store.hpp"

namespace nfd {

namespace fw {
class Strategy;
} // namespace fw

class NullFace;

/** \brief main class of NFD
 *
 *  Forwarder owns all faces and tables, and implements forwarding pipelines.
 */
class Forwarder
{
public:
  Forwarder();

  VIRTUAL_WITH_TESTS
  ~Forwarder();

  const ForwarderCounters&
  getCounters() const;

public: // faces
  FaceTable&
  getFaceTable();

  /** \brief get existing Face
   *
   *  shortcut to .getFaceTable().get(face)
   */
  shared_ptr<Face>
  getFace(FaceId id) const;

  /** \brief add new Face
   *
   *  shortcut to .getFaceTable().add(face)
   */
  void
  addFace(shared_ptr<Face> face);

public: // forwarding entrypoints and tables
  void
  onInterest(Face& face, const Interest& interest);

  void
  onData(Face& face, const Data& data);

  NameTree&
  getNameTree();

  Fib&
  getFib();

  Pit&
  getPit();

  Cs&
  getCs();

  ns3::Ptr<ns3::ndn::ContentStore> getCsFromNdnSim();

  Measurements&
  getMeasurements();

  StrategyChoice&
  getStrategyChoice();

  DeadNonceList&
  getDeadNonceList();

public: // allow enabling ndnSIM content store (will be removed in the future)
  void
  setCsFromNdnSim(ns3::Ptr<ns3::ndn::ContentStore> cs);

public:
  /** \brief trigger before PIT entry is satisfied
   *  \sa Strategy::beforeSatisfyInterest
   */
  signal::Signal<Forwarder, pit::Entry, Face, Data> beforeSatisfyInterest;

  /** \brief trigger before PIT entry expires
   *  \sa Strategy::beforeExpirePendingInterest
   */
  signal::Signal<Forwarder, pit::Entry> beforeExpirePendingInterest;

PUBLIC_WITH_TESTS_ELSE_PRIVATE: // pipelines
  /** \brief incoming Interest pipeline
   */
  VIRTUAL_WITH_TESTS void
  onIncomingInterest(Face& inFace, const Interest& interest);

  /** \brief Interest loop pipeline
   */
  VIRTUAL_WITH_TESTS void
  onInterestLoop(Face& inFace, const Interest& interest,
                 shared_ptr<pit::Entry> pitEntry);

  /** \brief outgoing Interest pipeline
   */
  VIRTUAL_WITH_TESTS void
  onOutgoingInterest(shared_ptr<pit::Entry> pitEntry, Face& outFace,
                     bool wantNewNonce = false);

  /** \brief Interest reject pipeline
   */
  VIRTUAL_WITH_TESTS void
  onInterestReject(shared_ptr<pit::Entry> pitEntry);

  /** \brief Interest unsatisfied pipeline
   */
  VIRTUAL_WITH_TESTS void
  onInterestUnsatisfied(shared_ptr<pit::Entry> pitEntry);

  /** \brief Interest finalize pipeline
   *  \param isSatisfied whether the Interest has been satisfied
   *  \param dataFreshnessPeriod FreshnessPeriod of satisfying Data
   */
  VIRTUAL_WITH_TESTS void
  onInterestFinalize(shared_ptr<pit::Entry> pitEntry, bool isSatisfied,
                     const time::milliseconds& dataFreshnessPeriod = time::milliseconds(-1));

  /** \brief incoming Data pipeline
   */
  VIRTUAL_WITH_TESTS void
  onIncomingData(Face& inFace, const Data& data);

  /** \brief Data unsolicited pipeline
   */
  VIRTUAL_WITH_TESTS void
  onDataUnsolicited(Face& inFace, const Data& data);

  /** \brief outgoing Data pipeline
   */
  VIRTUAL_WITH_TESTS void
  onOutgoingData(const Data& data, Face& outFace);

PROTECTED_WITH_TESTS_ELSE_PRIVATE:
  VIRTUAL_WITH_TESTS void
  setUnsatisfyTimer(shared_ptr<pit::Entry> pitEntry);

  VIRTUAL_WITH_TESTS void
  setStragglerTimer(shared_ptr<pit::Entry> pitEntry, bool isSatisfied,
                    const time::milliseconds& dataFreshnessPeriod = time::milliseconds(-1));

  VIRTUAL_WITH_TESTS void
  cancelUnsatisfyAndStragglerTimer(shared_ptr<pit::Entry> pitEntry);

  /** \brief insert Nonce to Dead Nonce List if necessary
   *  \param upstream if null, insert Nonces from all OutRecords;
   *                  if not null, insert Nonce only on the OutRecord of this face
   */
  VIRTUAL_WITH_TESTS void
  insertDeadNonceList(pit::Entry& pitEntry, bool isSatisfied,
                      const time::milliseconds& dataFreshnessPeriod,
                      Face* upstream);

  /// call trigger (method) on the effective strategy of pitEntry
#ifdef WITH_TESTS
  virtual void
  dispatchToStrategy(shared_ptr<pit::Entry> pitEntry, function<void(fw::Strategy*)> trigger);
#else
  template<class Function>
  void
  dispatchToStrategy(shared_ptr<pit::Entry> pitEntry, Function trigger);
#endif

private:
  ForwarderCounters m_counters;

  FaceTable m_faceTable;

  // tables
  NameTree       m_nameTree;
  Fib            m_fib;
  Pit            m_pit;
  Cs             m_cs;
  Measurements   m_measurements;
  StrategyChoice m_strategyChoice;
  DeadNonceList  m_deadNonceList;
  shared_ptr<NullFace> m_csFace;

  ns3::Ptr<ns3::ndn::ContentStore> m_csFromNdnSim;

  static const Name LOCALHOST_NAME;

  // allow Strategy (base class) to enter pipelines
  friend class fw::Strategy;
};

inline const ForwarderCounters&
Forwarder::getCounters() const
{
  return m_counters;
}

inline FaceTable&
Forwarder::getFaceTable()
{
  return m_faceTable;
}

inline shared_ptr<Face>
Forwarder::getFace(FaceId id) const
{
  return m_faceTable.get(id);
}

inline void
Forwarder::addFace(shared_ptr<Face> face)
{
  m_faceTable.add(face);
}

inline void
Forwarder::onInterest(Face& face, const Interest& interest)
{
  this->onIncomingInterest(face, interest);
}

inline void
Forwarder::onData(Face& face, const Data& data)
{
  this->onIncomingData(face, data);
}

inline NameTree&
Forwarder::getNameTree()
{
  return m_nameTree;
}

inline Fib&
Forwarder::getFib()
{
  return m_fib;
}

inline Pit&
Forwarder::getPit()
{
  return m_pit;
}

inline Cs&
Forwarder::getCs()
{
  return m_cs;
}

inline ns3::Ptr<ns3::ndn::ContentStore>
Forwarder::getCsFromNdnSim()
{
  return m_csFromNdnSim;
}

inline Measurements&
Forwarder::getMeasurements()
{
  return m_measurements;
}

inline StrategyChoice&
Forwarder::getStrategyChoice()
{
  return m_strategyChoice;
}

inline DeadNonceList&
Forwarder::getDeadNonceList()
{
  return m_deadNonceList;
}

inline void
Forwarder::setCsFromNdnSim(ns3::Ptr<ns3::ndn::ContentStore> cs)
{
  m_csFromNdnSim = cs;
}

#ifdef WITH_TESTS
inline void
Forwarder::dispatchToStrategy(shared_ptr<pit::Entry> pitEntry, function<void(fw::Strategy*)> trigger)
#else
template<class Function>
inline void
Forwarder::dispatchToStrategy(shared_ptr<pit::Entry> pitEntry, Function trigger)
#endif
{
  fw::Strategy& strategy = m_strategyChoice.findEffectiveStrategy(*pitEntry);
  trigger(&strategy);
}

} // namespace nfd

#endif // NFD_DAEMON_FW_FORWARDER_HPP
