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

  const int nCentBins = 13;
  float vCentBins[nCentBins + 1] = {-5.f, 0.f, 1.f, 5.f, 10.f, 20.f, 30.f, 40.f, 50.f, 60.f, 70.f, 80.f, 90.f, 100.f};

  const int nPtBins = 19;
  float vPtBins[nPtBins + 1] = {-5.f, 0.f, 1.f, 5.f, 10.f, 20.f, 30.f, 40.f, 50.f, 60.f, 70.f, 80.f, 90.f, 100.f};

  const int nDCAbins = 52;
  float vDCAbins[nDCAbins + 1] = {
    -1.30, -1.20, -1.10, -1.00, -0.90, -0.80, -0.70, -0.60, -0.50, -0.40,
    -0.35, -0.30, -0.25, -0.20, -0.15, -0.12, -0.10, -0.09, -0.08, -0.07,
    -0.06, -0.05, -0.04, -0.03, -0.02, -0.01, 0.00, 0.01, 0.02, 0.03,
    0.04, 0.05, 0.06, 0.07, 0.08, 0.09, 0.10, 0.12, 0.15, 0.20,
    0.25, 0.30, 0.35, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 1.00,
    1.10, 1.20, 1.30};

  const int nTOFbins = 75;
  float fTOFlowBoundary = -2.4;
  float fTOFhighBoundary = 3.6;
  float vTOFbins[nTOFbins + 1];
  const float deltaTOF = (fTOFhighBoundary - fTOFlowBoundary) / nTOFbins;
  for (int i = 0; i <= nTOFbins; ++i) {
    vTOFbins[i] = i * deltaTOF + fTOFlowBoundary;
  }

  const int nTPCbins = 240;
  float fSigmaLimit = 6;
  float vTPCbins[nTPCbins + 1];
  const float deltaSigma = 2.f * fSigmaLimit / nTPCBins;
  for (int i = 0; i <= nTPCbins; ++i) {
    vTPCbins[i] = i * deltaSigma - fSigmaLimit;
  }

  OutputObj<TH2F> fTPCsignal{TH2F("fTPCsignal", ";#it{p} (GeV/#it{c}); d#it{E} / d#it{X} (a. u.)", 600, 0., 3, 1400, 0, 1400)};
  OutputObj<TH1F> fMultiplicity{TH1F("fMultiplicity", ";V0M (%);", 101, -0.5, 100.5)};
  OutputObj<TH3F> fTOFsignal[2];
  // OutputObj<TH3F> fTOFsignalPos{TH3F("fMTOFsignal", ";Centrality (%);#it{p}_{T} (GeV/#it{c});#it{m}^{2}-m_{PDG}^{2} (GeV/#it{c}^{2})^{2}", nCentBins, vCentBins, nPtBins, vPtBins, nTOFnBins, vTOFbins)}
  // OutputObj<TH3F> fTOFsignalNeg{TH3F("fATOFsignal", ";Centrality (%);#it{p}_{T} (GeV/#it{c});#it{m}^{2}-m_{PDG}^{2} (GeV/#it{c}^{2})^{2}", nCentBins, vCentBins, nPtBins, vPtBins, nTOFnBins, vTOFbins)}
  // OutputObj<TH3F> fTPCcountsPos{TH3F("fMTPCcounts", ";Centrality (%);#it{p}_{T} (GeV/#it{c}); n_{#sigma} d", nCentBins, centBins, nPtBins, pTbins, nTPCbins, vTPCbins)};
  // OutputObj<TH3F> fTPCcountsNeg{TH3F("fATPCcounts", ";Centrality (%);#it{p}_{T} (GeV/#it{c}); n_{#sigma} d", nCentBins, centBins, nPtBins, pTbins, nTPCbins, vTPCbins)};

  Configurable<float> yMin{"yMin", -0.5, "Maximum rapidity"};
  Configurable<float> yMax{"yMax", 0.5, "Minimum rapidity"};
  Configurable<float> yBeam{"yBeam", 0., "Beam rapidity"};

  Filter trackFilter = aod::track::isGlobalTrack == true;

  void process(soa::Join<aod::Collisions, aod::EvSels, aod::Cents>::iterator const& col, soa::Filtered<soa::Join<aod::Tracks, aod::TracksExtra, aod::TrackSelection, aod::pidRespTPC>> const& tracks)
  {
    if (!col.alias()[kINT7])
      return;
    if (!col.sel7())
      return;

    fMultiplicity->Fill(col.centV0M());

    for (auto track : tracks) {
      TLorentzVector cutVector{};
      cutVector.SetPtEtaPhiM(track.pt(), track.eta(), track.phi(), constants::physics::MassDeuteron);
      if (cutVector.Rapidity() < yMin + yBeam || cutVector.Rapidity() > yMax + yBeam)
        continue;

      fTPCsignal->Fill(track.p(), track.tpcSignal());
    }
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  return WorkflowSpec{
    adaptAnalysisTask<NucleiSpecraTask>("nuclei-spectra")};
}
