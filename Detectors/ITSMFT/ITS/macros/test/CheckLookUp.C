/// \file CheckTopologies.C
/// Macros to test the generation of a dictionary of topologies. Three dictionaries are generated: one with signal-cluster only, one with noise-clusters only and one with all the clusters.

#if !defined(__CLING__) || defined(__ROOTCLING__)
#include <TAxis.h>
#include <TCanvas.h>
#include <TSystem.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TNtuple.h>
#include <TString.h>
#include <TStyle.h>
#include <TTree.h>
#include <TStopwatch.h>
#include <fstream>
#include <string>

#include "ITSMFTReconstruction/LookUp.h"
#include "DataFormatsITSMFT/CompCluster.h"
#include "DataFormatsITSMFT/ClusterTopology.h"
#include "DataFormatsITSMFT/ROFRecord.h"
#include "ITSMFTSimulation/Hit.h"
#include "DetectorsCommonDataFormats/NameConf.h"
#include "Framework/Logger.h"
#include <unordered_map>
#endif

void CheckLookUp(std::string clusfile = "o2clus_its.root",
                     std::string dictionary_file_name = "")
{

  if (dictionary_file_name.empty()) {
    dictionary_file_name = o2::base::NameConf::getDictionaryFileName(o2::detectors::DetID::ITS, "", ".bin");
  }

  using o2::itsmft::LookUp;
  using o2::itsmft::TopologyDictionary;
  using o2::itsmft::ClusterTopology;
  using o2::itsmft::ClusterPattern;
  using o2::itsmft::CompClusterExt;
  using o2::itsmft::CompCluster;

  using ROFRec = o2::itsmft::ROFRecord;

  TStopwatch sw;
  sw.Start();

  TopologyDictionary dict;
  dict.readBinaryFile(dictionary_file_name.c_str());
  int dict_size = dict.getSize();
  LookUp finder(dictionary_file_name.c_str());

  std::ofstream check_file("check_file.txt");

  // Clusters
  TFile* fileCl = TFile::Open(clusfile.data());
  TTree* clusTree = (TTree*)fileCl->Get("o2sim");
  std::vector<CompClusterExt>* clusArr = nullptr;
  clusTree->SetBranchAddress("ITSClusterComp", &clusArr);
  std::vector<unsigned char>* patternsPtr = nullptr;
  auto pattBranch = clusTree->GetBranch("ITSClusterPatt");
  if (pattBranch) {
    pattBranch->SetAddress(&patternsPtr);
  }

  // ROFrecords
  std::vector<ROFRec> rofRecVec, *rofRecVecP = &rofRecVec;
  clusTree->SetBranchAddress("ITSClustersROF", &rofRecVecP);


  clusTree->GetEntry(0);

  int nROFRec = (int)rofRecVec.size();

  int mistake_counter = 0;

  auto pattIdx = patternsPtr->cbegin();
  for (auto )



  for (int irof = 0; irof < nROFRec; irof++) {
    const auto& rofRec = rofRecVec[irof];

    rofRec.print();

    for (int icl = 0; icl < rofRec.getNEntries(); icl++) {
      int clEntry = rofRec.getFirstEntry() + icl; // entry of icl-th cluster of this ROF in the vector of clusters

      const auto& cluster = (*clusArr)[clEntry];

      std::cout << "ID: " << cluster.getPatternID() << std::endl;

      ClusterTopology topology;
      o2::itsmft::ClusterPattern pattern(pattIdx);
      unsigned char patt_tmp[ClusterPattern::MaxPatternBytes] = {0};
      std::memcpy(&patt_tmp[0],&pattern.getPattern()[2],ClusterPattern::MaxPatternBytes);
      topology.setPattern(pattern);
      int ID = finder.findGroupID(topology.getRowSpan(),topology.getColumnSpan(),patt_tmp);
      if(topology.getHash() != dict.getHash(ID)){
        check_file << "INPUT" << std::endl;
        check_file << "ID: " << cluster.getPatternID() << " Hash: " << topology.getHash() << " " << topology << std::endl;
        check_file << "OUTPUT" << std::endl;
        dict.printEntry(ID, check_file) << std::endl;
        mistake_counter++;
      }      
    }
  }
  std::cout << "Number of mistakes: " << mistake_counter << std::endl;
}
