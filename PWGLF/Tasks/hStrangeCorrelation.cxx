// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
///
/// \brief This task serves to do hadron-(strange hadron) correlation studies.
///  The yield will be calculated using the two-particle correlation method.
///  Trigger particle : Hadrons
///  Associated Particles : V0s or Cascades
///  this task requires the hStrangeCorrelationFilter to have been run before.
///
/// \author Kai Cui (kaicui@mails.ccnu.edu.cn)
/// \author Lucia Anna Tarasovicova (lucia.anna.husova@cern.ch)
/// \author David Dobrigkeit Chinellato (david.dobrigkeit.chinellato@cern.ch)
/// \author Zhongbao Yin (Zhong-Bao.Yin@cern.ch)

#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"
#include "Common/DataModel/TrackSelectionTables.h"
#include "PWGLF/DataModel/LFStrangenessTables.h"
#include "PWGLF/DataModel/LFHStrangeCorrelationTables.h"
#include "Common/DataModel/EventSelection.h"
#include "Common/DataModel/PIDResponse.h"
#include "Framework/ASoAHelpers.h"
#include "Common/DataModel/Multiplicity.h"
#include "Common/DataModel/Centrality.h"
#include "Framework/StaticFor.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;

// simple checkers
#define bitset(var, nbit) ((var) |= (1 << (nbit)))
#define bitcheck(var, nbit) ((var) & (1 << (nbit)))

using TracksComplete = soa::Join<aod::Tracks, aod::TracksExtra>;

struct correlateStrangeness {
  HistogramRegistry histos{"Histos", {}, OutputObjHandlingPolicy::AnalysisObject};

  Configurable<bool> doCorrelationK0Short{"doCorrelationK0Short", true, "do K0Short correlation"};
  Configurable<bool> doCorrelationLambda{"doCorrelationLambda", false, "do Lambda correlation"};
  Configurable<bool> doCorrelationAntiLambda{"doCorrelationAntiLambda", false, "do AntiLambda correlation"};
  Configurable<bool> doCorrelationXiMinus{"doCorrelationXiMinus", false, "do XiMinus correlation"};
  Configurable<bool> doCorrelationXiPlus{"doCorrelationXiPlus", false, "do XiPlus correlation"};
  Configurable<bool> doCorrelationOmegaMinus{"doCorrelationOmegaMinus", false, "do OmegaMinus correlation"};
  Configurable<bool> doCorrelationOmegaPlus{"doCorrelationOmegaPlus", false, "do OmegaPlus correlation"};
  Configurable<bool> doCorrelationPion{"doCorrelationPion", false, "do Pion correlation"};
  Configurable<int> zVertexCut{"zVertexCut", 10, "Cut on PV position"};

  // Axes - configurable for smaller sizes
  ConfigurableAxis axisMult{"axisMult", {VARIABLE_WIDTH, 0.0f, 0.01f, 1.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 70.0f, 100.0f}, "Mixing bins - multiplicity"};
  ConfigurableAxis axisVtxZ{"axisVtxZ", {VARIABLE_WIDTH, -10.0f, -8.f, -6.f, -4.f, -2.f, 0.f, 2.f, 4.f, 6.f, 8.f, 10.f}, "Mixing bins - z-vertex"};
  ConfigurableAxis axisPhi{"axisPhi", {72, -0.5 * M_PI, 1.5 * M_PI}, "#phi"};
  ConfigurableAxis axisEta{"axisEta", {80, -0.8, +0.8}, "#eta"};
  ConfigurableAxis axisDeltaPhi{"axisDeltaPhi", {72, -PIHalf, PIHalf * 3}, "delta #varphi axis for histograms"};
  ConfigurableAxis axisDeltaEta{"axisDeltaEta", {50, -2, 2}, "delta eta axis for histograms"};
  ConfigurableAxis axisPtAssoc{"axisPtAssoc", {VARIABLE_WIDTH, 0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0, 10.0}, "pt associated axis for histograms"};
  ConfigurableAxis axisPtQA{"axisPtQA", {VARIABLE_WIDTH, 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f, 2.0f, 2.2f, 2.4f, 2.6f, 2.8f, 3.0f, 3.2f, 3.4f, 3.6f, 3.8f, 4.0f, 4.4f, 4.8f, 5.2f, 5.6f, 6.0f, 6.5f, 7.0f, 7.5f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 17.0f, 19.0f, 21.0f, 23.0f, 25.0f, 30.0f, 35.0f, 40.0f, 50.0f}, "pt axis for QA histograms"};
  ConfigurableAxis axisK0ShortMass{"axisK0ShortMass", {200, 0.400f, 0.600f}, "Inv. Mass (GeV/c^{2})"};
  ConfigurableAxis axisLambdaMass{"axisLambdaMass", {200, 1.01f, 1.21f}, "Inv. Mass (GeV/c^{2})"};
  ConfigurableAxis axisXiMass{"axisXiMass", {200, 1.22f, 1.42f}, "Inv. Mass (GeV/c^{2})"};
  ConfigurableAxis axisOmegaMass{"axisOmegaMass", {200, 1.57f, 1.77f}, "Inv. Mass (GeV/c^{2})"};

  using BinningType = ColumnBinningPolicy<aod::collision::PosZ, aod::cent::CentFT0M>;
  BinningType colBinning{{axisVtxZ, axisMult}, true}; // true is for 'ignore overflows' (true by default). Underflows and overflows will have bin -1.

  // collision slicing for mixed events
  Preslice<aod::TriggerTracks> collisionSliceTracks = aod::triggerTracks::collisionId;
  Preslice<aod::AssocV0s> collisionSliceV0s = aod::assocV0s::collisionId;
  Preslice<aod::AssocCascades> collisionSliceCascades = aod::assocCascades::collisionId;
  Preslice<aod::AssocPions> collisionSlicePions = aod::assocPions::collisionId;

  static constexpr std::string_view v0names[] = {"K0Short", "Lambda", "AntiLambda"};
  static constexpr std::string_view cascadenames[] = {"XiMinus", "XiPlus", "OmegaMinus", "OmegaPlus"};

  uint8_t doCorrelation;

  /// Function to aid in calculating delta-phi
  /// \param phi1 first phi value
  /// \param phi2 second phi value
  Double_t ComputeDeltaPhi(Double_t phi1, Double_t phi2)
  {
    Double_t deltaPhi = phi1 - phi2;
    if (deltaPhi < -TMath::Pi() / 2.) {
      deltaPhi += 2. * TMath::Pi();
    }
    if (deltaPhi > 3 * TMath::Pi() / 2.) {
      deltaPhi -= 2. * TMath::Pi();
    }
    return deltaPhi;
  }

  void fillCorrelationsV0(aod::TriggerTracks const& triggers, aod::AssocV0s const& assocs, bool mixing, float pvz, float mult)
  {
    for (auto& triggerTrack : triggers) {
      auto trigg = triggerTrack.track_as<TracksComplete>();
      if (!mixing)
        histos.fill(HIST("sameEvent/TriggerParticlesV0"), trigg.pt(), mult);
      for (auto& assocCandidate : assocs) {
        auto assoc = assocCandidate.v0Data();

        //---] removing autocorrelations [---
        auto postrack = assoc.posTrack_as<TracksComplete>();
        auto negtrack = assoc.negTrack_as<TracksComplete>();
        if (trigg.globalIndex() == postrack.globalIndex())
          continue;
        if (trigg.globalIndex() == negtrack.globalIndex())
          continue;
        // TODO: add histogram checking how many pairs are rejected (should be small!)

        float deltaphi = ComputeDeltaPhi(trigg.phi(), assoc.phi());
        float deltaeta = trigg.eta() - assoc.eta();
        float ptassoc = assoc.pt();
        static_for<0, 2>([&](auto i) {
          constexpr int index = i.value;
          if (bitcheck(doCorrelation, index)) {
            if (assocCandidate.compatible(index) && !mixing && assocCandidate.inMassRegionCheck(index, 1))
              histos.fill(HIST("sameEvent/LeftBg/") + HIST(v0names[index]), deltaphi, deltaeta, ptassoc, pvz, mult);
            if (assocCandidate.compatible(index) && !mixing && assocCandidate.inMassRegionCheck(index, 2))
              histos.fill(HIST("sameEvent/Signal/") + HIST(v0names[index]), deltaphi, deltaeta, ptassoc, pvz, mult);
            if (assocCandidate.compatible(index) && !mixing && assocCandidate.inMassRegionCheck(index, 3))
              histos.fill(HIST("sameEvent/RightBg/") + HIST(v0names[index]), deltaphi, deltaeta, ptassoc, pvz, mult);
            if (assocCandidate.compatible(index) && mixing && assocCandidate.inMassRegionCheck(index, 1))
              histos.fill(HIST("mixedEvent/LeftBg/") + HIST(v0names[index]), deltaphi, deltaeta, ptassoc, pvz, mult);
            if (assocCandidate.compatible(index) && mixing && assocCandidate.inMassRegionCheck(index, 2))
              histos.fill(HIST("mixedEvent/Signal/") + HIST(v0names[index]), deltaphi, deltaeta, ptassoc, pvz, mult);
            if (assocCandidate.compatible(index) && mixing && assocCandidate.inMassRegionCheck(index, 3))
              histos.fill(HIST("mixedEvent/RightBg/") + HIST(v0names[index]), deltaphi, deltaeta, ptassoc, pvz, mult);
          }
        });
      }
    }
  }

  void fillCorrelationsCascade(aod::TriggerTracks const& triggers, aod::AssocCascades const& assocs, bool mixing, float pvz, float mult)
  {
    for (auto& triggerTrack : triggers) {
      auto trigg = triggerTrack.track_as<TracksComplete>();
      if (!mixing)
        histos.fill(HIST("sameEvent/TriggerParticlesCascade"), trigg.pt(), mult);
      for (auto& assocCandidate : assocs) {
        auto assoc = assocCandidate.cascData();

        //---] removing autocorrelations [---
        auto v0index = assoc.v0_as<o2::aod::V0sLinked>();
        if (!(v0index.has_v0Data()))
          continue;                      // this should not happen - included for safety
        auto assocV0 = v0index.v0Data(); // de-reference index to correct v0data in case it exists
        auto postrack = assocV0.posTrack_as<TracksComplete>();
        auto negtrack = assocV0.negTrack_as<TracksComplete>();
        auto bachtrack = assoc.bachelor_as<TracksComplete>();
        if (trigg.globalIndex() == postrack.globalIndex())
          continue;
        if (trigg.globalIndex() == negtrack.globalIndex())
          continue;
        if (trigg.globalIndex() == bachtrack.globalIndex())
          continue;
        // TODO: add histogram checking how many pairs are rejected (should be small!)

        float deltaphi = ComputeDeltaPhi(trigg.phi(), assoc.phi());
        float deltaeta = trigg.eta() - assoc.eta();
        float ptassoc = assoc.pt();
        static_for<0, 3>([&](auto i) {
          constexpr int index = i.value;
          if (bitcheck(doCorrelation, index + 3)) {
            if (assocCandidate.compatible(index) && !mixing && assocCandidate.inMassRegionCheck(index, 1))
              histos.fill(HIST("sameEvent/LeftBg/") + HIST(cascadenames[index]), deltaphi, deltaeta, ptassoc, pvz, mult);
            if (assocCandidate.compatible(index) && !mixing && assocCandidate.inMassRegionCheck(index, 2))
              histos.fill(HIST("sameEvent/Signal/") + HIST(cascadenames[index]), deltaphi, deltaeta, ptassoc, pvz, mult);
            if (assocCandidate.compatible(index) && !mixing && assocCandidate.inMassRegionCheck(index, 3))
              histos.fill(HIST("sameEvent/RightBg/") + HIST(cascadenames[index]), deltaphi, deltaeta, ptassoc, pvz, mult);
            if (assocCandidate.compatible(index) && mixing && assocCandidate.inMassRegionCheck(index, 1))
              histos.fill(HIST("mixedEvent/LeftBg/") + HIST(cascadenames[index]), deltaphi, deltaeta, ptassoc, pvz, mult);
            if (assocCandidate.compatible(index) && mixing && assocCandidate.inMassRegionCheck(index, 2))
              histos.fill(HIST("mixedEvent/Signal/") + HIST(cascadenames[index]), deltaphi, deltaeta, ptassoc, pvz, mult);
            if (assocCandidate.compatible(index) && mixing && assocCandidate.inMassRegionCheck(index, 3))
              histos.fill(HIST("mixedEvent/RightBg/") + HIST(cascadenames[index]), deltaphi, deltaeta, ptassoc, pvz, mult);
          }
        });
      }
    }
  }

  void fillCorrelationsPion(aod::TriggerTracks const& triggers, aod::AssocPions const& assocs, bool mixing, float pvz, float mult)
  {

    for (auto& triggerTrack : triggers) {
      auto trigg = triggerTrack.track_as<TracksComplete>();
      if (!mixing)
        histos.fill(HIST("sameEvent/TriggerParticlesPion"), trigg.pt(), mult);
      for (auto& assocTrack : assocs) {
        auto assoc = assocTrack.track_as<TracksComplete>();

        //---] removing autocorrelations [---
        if (trigg.globalIndex() == assoc.globalIndex())
          continue;
        // TODO: add histogram checking how many pairs are rejected (should be small!)

        float deltaphi = ComputeDeltaPhi(trigg.phi(), assoc.phi());
        float deltaeta = trigg.eta() - assoc.eta();
        float ptassoc = assoc.pt();
        if (!mixing)
          histos.fill(HIST("sameEvent/Pion"), deltaphi, deltaeta, ptassoc, pvz, mult);
        else
          histos.fill(HIST("mixedEvent/Pion"), deltaphi, deltaeta, ptassoc, pvz, mult);
      }
    }
  }

  void init(InitContext const&)
  {
    // set bitmap for convenience
    doCorrelation = 0;
    if (doCorrelationK0Short)
      bitset(doCorrelation, 0);
    if (doCorrelationLambda)
      bitset(doCorrelation, 1);
    if (doCorrelationAntiLambda)
      bitset(doCorrelation, 2);
    if (doCorrelationXiMinus)
      bitset(doCorrelation, 3);
    if (doCorrelationXiPlus)
      bitset(doCorrelation, 4);
    if (doCorrelationOmegaMinus)
      bitset(doCorrelation, 5);
    if (doCorrelationOmegaPlus)
      bitset(doCorrelation, 6);
    if (doCorrelationPion)
      bitset(doCorrelation, 7);

    if (bitcheck(doCorrelation, 0)) {
      histos.add("h2dMassK0Short", "h2dMassK0Short", kTH2F, {axisPtQA, axisK0ShortMass, axisMult});
      histos.add("sameEvent/Signal/K0Short", "K0Short", kTHnF, {axisDeltaPhi, axisDeltaEta, axisPtAssoc, axisVtxZ, axisMult});
    }
    if (bitcheck(doCorrelation, 1)) {
      histos.add("h2dMassLambda", "h2dMassLambda", kTH2F, {axisPtQA, axisLambdaMass, axisMult});
      histos.add("sameEvent/Signal/Lambda", "Lambda", kTHnF, {axisDeltaPhi, axisDeltaEta, axisPtAssoc, axisVtxZ, axisMult});
    }
    if (bitcheck(doCorrelation, 2)) {
      histos.add("h2dMassAntiLambda", "h2dMassAntiLambda", kTH2F, {axisPtQA, axisLambdaMass, axisMult});
      histos.add("sameEvent/Signal/AntiLambda", "AntiLambda", kTHnF, {axisDeltaPhi, axisDeltaEta, axisPtAssoc, axisVtxZ, axisMult});
    }
    if (bitcheck(doCorrelation, 3)) {
      histos.add("h2dMassXiMinus", "h2dMassXiMinus", kTH2F, {axisPtQA, axisXiMass, axisMult});
      histos.add("sameEvent/Signal/XiMinus", "XiMinus", kTHnF, {axisDeltaPhi, axisDeltaEta, axisPtAssoc, axisVtxZ, axisMult});
    }
    if (bitcheck(doCorrelation, 4)) {
      histos.add("h2dMassXiPlus", "h2dMassXiPlus", kTH2F, {axisPtQA, axisXiMass, axisMult});
      histos.add("sameEvent/Signal/XiPlus", "XiPlus", kTHnF, {axisDeltaPhi, axisDeltaEta, axisPtAssoc, axisVtxZ, axisMult});
    }
    if (bitcheck(doCorrelation, 5)) {
      histos.add("h2dMassOmegaMinus", "h2dMassOmegaMinus", kTH2F, {axisPtQA, axisOmegaMass, axisMult});
      histos.add("sameEvent/Signal/OmegaMinus", "OmegaMinus", kTHnF, {axisDeltaPhi, axisDeltaEta, axisPtAssoc, axisVtxZ, axisMult});
    }
    if (bitcheck(doCorrelation, 6)) {
      histos.add("h2dMassOmegaPlus", "h2dMassOmegaPlus", kTH2F, {axisPtQA, axisOmegaMass, axisMult});
      histos.add("sameEvent/Signal/OmegaPlus", "OmegaPlus", kTHnF, {axisDeltaPhi, axisDeltaEta, axisPtAssoc, axisVtxZ, axisMult});
    }
    if (doCorrelationPion)
      histos.add("sameEvent/Pion", "Pion", kTHnF, {axisDeltaPhi, axisDeltaEta, axisPtAssoc, axisVtxZ, axisMult});

    if (doCorrelationK0Short || doCorrelationLambda || doCorrelationAntiLambda || doCorrelationXiMinus || doCorrelationXiPlus || doCorrelationOmegaMinus || doCorrelationOmegaPlus) {
      histos.addClone("sameEvent/Signal/", "sameEvent/LeftBg/");
      histos.addClone("sameEvent/Signal/", "sameEvent/RightBg/");
    }

    // mixed-event correlation functions
    if (doprocessMixedEventHV0s || doprocessMixedEventHCascades || doprocessMixedEventHPions) {
      histos.addClone("sameEvent/", "mixedEvent/");
    }

    // Some QA plots
    histos.add("hTrackEtaVsPtVsPhi", "hTrackEtaVsPtVsPhi", kTH3F, {axisPtQA, axisEta, axisPhi});
    histos.add("hV0EtaVsPtVsPhi", "hV0EtaVsPtVsPhi", kTH3F, {axisPtQA, axisEta, axisPhi});
    histos.add("hCascEtaVsPtVsPhi", "hCascEtaVsPtVsPhi", kTH3F, {axisPtQA, axisEta, axisPhi});
    histos.add("hPionEtaVsPtVsPhi", "hPionEtaVsPtVsPhi", kTH3F, {axisPtQA, axisEta, axisPhi});

    histos.add("sameEvent/TriggerParticlesV0", "TriggersV0", kTH2F, {axisPtQA, axisMult});
    histos.add("sameEvent/TriggerParticlesCascade", "TriggersCascade", kTH2F, {axisPtQA, axisMult});
    histos.add("sameEvent/TriggerParticlesPion", "TriggersPion", kTH2F, {axisPtQA, axisMult});

    // mixing QA
    histos.add("MixingQA/hSECollisionBins", ";bin;Entries", kTH1F, {{140, -0.5, 139.5}});
    histos.add("MixingQA/hMECollisionBins", ";bin;Entries", kTH1F, {{140, -0.5, 139.5}});
    histos.add("MixingQA/hMEpvz1", ";pvz;Entries", kTH1F, {{30, -15, 15}});
    histos.add("MixingQA/hMEpvz2", ";pvz;Entries", kTH1F, {{30, -15, 15}});

    // Event QA
    histos.add("EventQA/hMixingQA", "mixing QA", kTH1F, {{2, -0.5, 1.5}});
    histos.add("EventQA/hMult", "Multiplicity", kTH1F, {axisMult});
    histos.add("EventQA/hPvz", ";pvz;Entries", kTH1F, {{30, -15, 15}});
  }

  void processSameEventHV0s(soa::Join<aod::Collisions, aod::EvSels, aod::CentFT0Ms>::iterator const& collision,
                            aod::AssocV0s const& associatedV0s, aod::TriggerTracks const& triggerTracks,
                            aod::V0Datas const&, aod::V0sLinked const&, TracksComplete const&)
  {
    // ________________________________________________
    // Perform basic event selection
    if (!collision.sel8()) {
      return;
    }
    if (TMath::Abs(collision.posZ()) > zVertexCut) {
      return;
    }
    // ________________________________________________
    if (!doprocessSameEventHCascades) {
      histos.fill(HIST("MixingQA/hSECollisionBins"), colBinning.getBin({collision.posZ(), collision.centFT0M()}));
      histos.fill(HIST("EventQA/hMult"), collision.centFT0M());
      histos.fill(HIST("EventQA/hPvz"), collision.posZ());
    }
    // Do basic QA
    for (auto const& v0 : associatedV0s) {
      auto v0Data = v0.v0Data();
      histos.fill(HIST("hV0EtaVsPtVsPhi"), v0Data.pt(), v0Data.eta(), v0Data.phi());
      static_for<0, 2>([&](auto i) {
        constexpr int index = i.value;
        if (v0.compatible(index) && bitcheck(doCorrelation, index))
          histos.fill(HIST("h2dMass") + HIST(v0names[index]), v0Data.pt(), v0Data.m(index));
      });
    }
    if (!doprocessSameEventHCascades) {
      for (auto const& triggerTrack : triggerTracks) {
        auto track = triggerTrack.track_as<TracksComplete>();
        histos.fill(HIST("hTrackEtaVsPtVsPhi"), track.pt(), track.eta(), track.phi());
      }
    }

    // ________________________________________________
    // Do hadron - V0 correlations
    fillCorrelationsV0(triggerTracks, associatedV0s, false, collision.posZ(), collision.centFT0M());
  }
  void processSameEventHCascades(soa::Join<aod::Collisions, aod::EvSels, aod::CentFT0Ms>::iterator const& collision,
                                 aod::AssocV0s const& associatedV0s, aod::AssocCascades const& associatedCascades, aod::TriggerTracks const& triggerTracks,
                                 aod::V0Datas const&, aod::V0sLinked const&, aod::CascDatas const&, TracksComplete const&)
  {
    // ________________________________________________
    // Perform basic event selection
    if (!collision.sel8()) {
      return;
    }
    if (TMath::Abs(collision.posZ()) > zVertexCut) {
      return;
    }
    // ________________________________________________
    histos.fill(HIST("MixingQA/hSECollisionBins"), colBinning.getBin({collision.posZ(), collision.centFT0M()}));
    histos.fill(HIST("EventQA/hMult"), collision.centFT0M());
    histos.fill(HIST("EventQA/hPvz"), collision.posZ());
    // Do basic QA
    for (auto const& casc : associatedCascades) {
      auto cascData = casc.cascData();
      histos.fill(HIST("hCascEtaVsPtVsPhi"), cascData.pt(), cascData.eta(), cascData.phi());
      static_for<0, 3>([&](auto i) {
        constexpr int index = i.value;
        if (casc.compatible(index) && bitcheck(doCorrelation, index + 3))
          histos.fill(HIST("h2dMass") + HIST(cascadenames[index]), cascData.pt(), cascData.m(index));
      });
    }
    for (auto const& triggerTrack : triggerTracks) {
      auto track = triggerTrack.track_as<TracksComplete>();
      histos.fill(HIST("hTrackEtaVsPtVsPhi"), track.pt(), track.eta(), track.phi());
    }

    // ________________________________________________
    // Do hadron - cascade correlations
    fillCorrelationsCascade(triggerTracks, associatedCascades, false, collision.posZ(), collision.centFT0M());
  }
  void processSameEventHPions(soa::Join<aod::Collisions, aod::EvSels, aod::CentFT0Ms>::iterator const& collision,
                              aod::AssocPions const& associatedPions, aod::TriggerTracks const& triggerTracks,
                              TracksComplete const&)
  {
    // ________________________________________________
    // Perform basic event selection
    if (!collision.sel8()) {
      return;
    }
    if (TMath::Abs(collision.posZ()) > zVertexCut) {
      return;
    }
    // ________________________________________________
    if (!doprocessSameEventHCascades && !doprocessSameEventHV0s) {
      histos.fill(HIST("MixingQA/hSECollisionBins"), colBinning.getBin({collision.posZ(), collision.centFT0M()}));
      histos.fill(HIST("EventQA/hMult"), collision.centFT0M());
      histos.fill(HIST("EventQA/hPvz"), collision.posZ());
    }
    // Do basic QA
    for (auto const& pion : associatedPions) {
      auto pionTrack = pion.track_as<TracksComplete>();
      histos.fill(HIST("hPionEtaVsPtVsPhi"), pionTrack.pt(), pionTrack.eta(), pionTrack.phi());
    }
    if (!doprocessSameEventHCascades && !doprocessSameEventHV0s) {
      for (auto const& triggerTrack : triggerTracks) {
        auto track = triggerTrack.track_as<TracksComplete>();
        histos.fill(HIST("hTrackEtaVsPtVsPhi"), track.pt(), track.eta(), track.phi());
      }
    }

    // ________________________________________________
    // Do hadron - Pion correlations
    fillCorrelationsPion(triggerTracks, associatedPions, false, collision.posZ(), collision.centFT0M());
  }
  void processMixedEventHV0s(soa::Join<aod::Collisions, aod::EvSels, aod::CentFT0Ms> const& collisions,
                             aod::AssocV0s const& associatedV0s, aod::TriggerTracks const& triggerTracks,
                             aod::V0Datas const&, aod::V0sLinked const&, TracksComplete const&)
  {
    for (auto& [collision1, collision2] : soa::selfCombinations(colBinning, 5, -1, collisions, collisions)) {
      // ________________________________________________
      // Perform basic event selection on both collisions
      if (!collision1.sel8() || !collision2.sel8())
        continue;
      if (!doprocessMixedEventHCascades) {
        if (collision1.globalIndex() == collision2.globalIndex()) {
          histos.fill(HIST("MixingQA/hMixingQA"), 0.0f); // same-collision pair counting
        }
        histos.fill(HIST("MixingQA/hMEpvz1"), collision1.posZ());
        histos.fill(HIST("MixingQA/hMEpvz2"), collision2.posZ());
        histos.fill(HIST("MixingQA/hMECollisionBins"), colBinning.getBin({collision1.posZ(), collision1.centFT0M()}));
      }
      // ________________________________________________
      // Do slicing
      auto slicedTriggerTracks = triggerTracks.sliceBy(collisionSliceTracks, collision1.globalIndex());
      auto slicedAssocV0s = associatedV0s.sliceBy(collisionSliceV0s, collision2.globalIndex());
      // ________________________________________________
      // Do hadron - V0 correlations
      fillCorrelationsV0(slicedTriggerTracks, slicedAssocV0s, true, collision1.posZ(), collision1.centFT0M());
    }
  }
  void processMixedEventHCascades(soa::Join<aod::Collisions, aod::EvSels, aod::CentFT0Ms> const& collisions,
                                  aod::AssocV0s const& associatedV0s, aod::AssocCascades const& associatedCascades, aod::TriggerTracks const& triggerTracks,
                                  aod::V0Datas const&, aod::V0sLinked const&, aod::CascDatas const&, TracksComplete const&)
  {
    for (auto& [collision1, collision2] : soa::selfCombinations(colBinning, 5, -1, collisions, collisions)) {
      // ________________________________________________
      // Perform basic event selection on both collisions
      if (!collision1.sel8() || !collision2.sel8())
        continue;

      if (collision1.globalIndex() == collision2.globalIndex()) {
        histos.fill(HIST("MixingQA/hMixingQA"), 0.0f); // same-collision pair counting
      }

      histos.fill(HIST("MixingQA/hMEpvz1"), collision1.posZ());
      histos.fill(HIST("MixingQA/hMEpvz2"), collision2.posZ());
      histos.fill(HIST("MixingQA/hMECollisionBins"), colBinning.getBin({collision1.posZ(), collision1.centFT0M()}));
      // ________________________________________________
      // Do slicing
      auto slicedTriggerTracks = triggerTracks.sliceBy(collisionSliceTracks, collision1.globalIndex());
      auto slicedAssocCascades = associatedCascades.sliceBy(collisionSliceCascades, collision2.globalIndex());
      // ________________________________________________
      // Do hadron - cascade correlations
      fillCorrelationsCascade(slicedTriggerTracks, slicedAssocCascades, true, collision1.posZ(), collision1.centFT0M());
    }
  }
  void processMixedEventHPions(soa::Join<aod::Collisions, aod::EvSels, aod::CentFT0Ms> const& collisions,
                               aod::AssocPions const& assocPions, aod::TriggerTracks const& triggerTracks,
                               TracksComplete const&)
  {
    for (auto& [collision1, collision2] : soa::selfCombinations(colBinning, 5, -1, collisions, collisions)) {
      // ________________________________________________
      // Perform basic event selection on both collisions
      if (!collision1.sel8() || !collision2.sel8())
        continue;

      if (collision1.globalIndex() == collision2.globalIndex()) {
        histos.fill(HIST("MixingQA/hMixingQA"), 0.0f); // same-collision pair counting
      }

      histos.fill(HIST("MixingQA/hMEpvz1"), collision1.posZ());
      histos.fill(HIST("MixingQA/hMEpvz2"), collision2.posZ());
      histos.fill(HIST("MixingQA/hMECollisionBins"), colBinning.getBin({collision1.posZ(), collision1.centFT0M()}));
      // ________________________________________________
      // Do slicing
      auto slicedTriggerTracks = triggerTracks.sliceBy(collisionSliceTracks, collision1.globalIndex());
      auto slicedAssocPions = assocPions.sliceBy(collisionSlicePions, collision2.globalIndex());
      // ________________________________________________
      // Do hadron - cascade correlations
      fillCorrelationsPion(slicedTriggerTracks, slicedAssocPions, true, collision1.posZ(), collision1.centFT0M());
    }
  }

  PROCESS_SWITCH(correlateStrangeness, processSameEventHV0s, "Process same events, h-V0s", true);
  PROCESS_SWITCH(correlateStrangeness, processSameEventHCascades, "Process same events, h-Cascades", true);
  PROCESS_SWITCH(correlateStrangeness, processSameEventHPions, "Process same events, h-Pion", true);
  PROCESS_SWITCH(correlateStrangeness, processMixedEventHV0s, "Process mixed events, h-V0s", true);
  PROCESS_SWITCH(correlateStrangeness, processMixedEventHCascades, "Process mixed events, h-Cascades", true);
  PROCESS_SWITCH(correlateStrangeness, processMixedEventHPions, "Process mixed events, h-Pion", true);
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{
    adaptAnalysisTask<correlateStrangeness>(cfgc)};
}
