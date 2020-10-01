// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"
#include "Framework/AnalysisDataModel.h"
#include "CommonConstants/PhysicsConstants.h"

#include "Analysis/TriggerAliases.h"
#include "Analysis/EventSelection.h"
#include "Analysis/Multiplicity.h"
#include "Analysis/Centrality.h"
#include "Analysis/TrackSelectionTables.h"

#include "PID/PIDResponse.h"

#include <TH1F.h>
#include <TH2F.h>
#include <TLorentzVector.h>

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;

struct NucleiSpecraTask {

  OutputObj<TH2F> hTPCsignal{TH2F("hTPCsignal", ";#it{p} (GeV/#it{c}); d#it{E} / d#it{X} (a. u.)", 600, 0., 3, 1400, 0, 1400)};
  OutputObj<TH1F> hMultiplicity{TH1F("hMultiplicity", ";V0M (%);", 101, -0.5, 100.5)};

  FiltertrackFilter = aod::track::isGlobalTrack == true; 

  void process(soa::Join<aod::Collisions, aod::EvSels, aod::Cents>::iterator const& col, soa::Filtered<soa::Join<aod::Tracks, aod::TracksExtra, aod::TrackSelection, aod::pidRespTPC>>::iterator const& track)
  { 
    if (!col.alias()[kINT7])
      return;
    if (!col.sel7())
      return;

    hMultiplicity->Fill(col.centV0M());
    
    TLorentzVector cutVector{};
    cutVector.SetPtEtaPhiM(track.pt(), track.eta(), track.phi(), constants::physics::MassDeuteron);
    if (cutVector.Rapidity() < yMin + beamRapidity || cutVector.Rapidity() > yMax + beamRapidity)
      continue;

    hTPCsignal->Fill(track.p(), track.tpcSignal());
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  return WorkflowSpec{
    adaptAnalysisTask<NucleiSpecraTask>("nuclei-spectra")};
}
