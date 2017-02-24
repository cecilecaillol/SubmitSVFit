#include "PhysicsTools/FWLite/interface/CommandLineParser.h" 
#include "TFile.h"
#include "TROOT.h"
#include "TLorentzVector.h"
#include "TKey.h"
#include "TTree.h"
#include "TH1F.h"
#include "TF1.h"
#include <math.h> 
#include "TMath.h" 
#include <limits>
#include "TSystem.h"

#include "FWCore/ParameterSet/interface/FileInPath.h"

#include "TauAnalysis/SVfitStandalone/interface/SVfitStandaloneAlgorithm.h"
#include "HTT-utilities/RecoilCorrections/interface/RecoilCorrector.h"

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"

//If recoilType 0 then don't do recoil
//              FIXME amc@nlo is not ready yet!!! 1 then aMC@NLO DY and W+Jets MC samples
//                1 is not longer an option
//              2 MG5 DY and W+Jets MC samples or Higgs MC samples
//
//If doES       0 does not apply any ES shifts
//              1 applies ES shifts to TT channel, no effect on other channels
//
//If isWJets    0 no shift in number of jets used for recoil corrections
//              1 shifts njets + 1 for recoil corrections
//
//If metType    1 use mvamet
//        -1 use pf met

void copyFiles( optutl::CommandLineParser parser, TFile* fOld, TFile* fNew) ;
void readdir(TDirectory *dir, optutl::CommandLineParser parser, char TreeToUse[], int recoilType, int doES, int isWJets, int metType) ;
void CopyFile(const char *fname, optutl::CommandLineParser parser);
void CopyDir(TDirectory *source,optutl::CommandLineParser parser);
void runSVFit(std::vector<svFitStandalone::MeasuredTauLepton> & measuredTauLeptons, TFile * inputFile_visPtResolution, double measuredMETx, double measuredMETy, TMatrixD &covMET, float num, float &svFitMass, float& svFitPt, float &svFitEta, float &svFitPhi, float &svFitMET, float &svFitTransverseMass);

int main (int argc, char* argv[]) 
{
   optutl::CommandLineParser parser ("Sets Event Weights in the ntuple");
   parser.addOption("branch",optutl::CommandLineParser::kString,"Branch","__svFit__");
   parser.addOption("newFile",optutl::CommandLineParser::kString,"newFile","newFile.root");
   parser.addOption("inputFile",optutl::CommandLineParser::kString,"input File");
   parser.addOption("newOutputFile",optutl::CommandLineParser::kDouble,"New Output File",0.0);
   parser.addOption("recoilType",optutl::CommandLineParser::kDouble,"recoilType",0.0);
   parser.addOption("doES",optutl::CommandLineParser::kDouble,"doES",0.0);
   parser.addOption("isWJets",optutl::CommandLineParser::kDouble,"isWJets",0.0);
   parser.addOption("metType",optutl::CommandLineParser::kDouble,"metType",1.0); // 1 = mvamet, -1 = pf met

   parser.parseArguments (argc, argv);

   std::cout << "EXTRA COMMANDS:"
    << "\n --- recoilType: " << parser.doubleValue("recoilType")
    << "\n --- doES: " << parser.doubleValue("doES")
    << "\n --- isWJets: " << parser.doubleValue("isWJets")
    << "\n --- metType: " << parser.doubleValue("metType") << std::endl;

   // Make sure a proper Met Type is chosen
   assert (parser.doubleValue("metType") == 1.0 || parser.doubleValue("metType") == -1.0);
   
   char TreeToUse[80]="first" ;

   TFile *fProduce;//= new TFile(parser.stringValue("newFile").c_str(),"UPDATE");

   if(parser.doubleValue("newOutputFile")>0){
   TFile *f = new TFile(parser.stringValue("inputFile").c_str(),"READ");
     std::cout<<"Creating new outputfile"<<std::endl;
     std::string newFileName = parser.stringValue("newFile");

     fProduce = new TFile(newFileName.c_str(),"RECREATE");
     copyFiles(parser, f, fProduce);//new TFile(parser.stringValue("inputFile").c_str()+"SVFit","UPDATE");
     fProduce = new TFile(newFileName.c_str(),"UPDATE");
     std::cout<<"listing the directories================="<<std::endl;
     fProduce->ls();
     readdir(fProduce,parser,TreeToUse,parser.doubleValue("recoilType"),parser.doubleValue("doES"),
            parser.doubleValue("isWJets"),parser.doubleValue("metType"));

     fProduce->Close();
     f->Close();
   }
   else{
     TFile *f = new TFile(parser.stringValue("inputFile").c_str(),"UPDATE");
     readdir(f,parser,TreeToUse,parser.doubleValue("recoilType"),parser.doubleValue("doES"),
            parser.doubleValue("isWJets"),parser.doubleValue("metType"));
     f->Close();
   }


} 


void readdir(TDirectory *dir, optutl::CommandLineParser parser, char TreeToUse[], int recoilType, int doES, int isWJets, int metType) 
{
  std::string recoilFileName = "HTT-utilities/RecoilCorrections/data/TypeI-PFMet_Run2016BtoH.root";
  if(recoilType == 1) { //amc@nlo
    std::cout << "Alexei no long specified MG vs. AMC@NLO, so use recoilType = 2" << std::endl;
    return; }
  if(recoilType == 2 && metType == 1) { // mva met (Alexei no long specified MG vs. AMC@NLO)
    std::cout << "Alexei does not provide full 2016 data recoil corrections for Mva Met\n\n" << std::endl;
    std::cout << "Using ICHEP Mva Met corrections\n\n" << std::endl;
    recoilFileName = "HTT-utilities/RecoilCorrections/data/MvaMET_2016BCD.root";}
  if(recoilType == 2 && metType == -1) { // pf met (Alexei no long specified MG vs. AMC@NLO)
    recoilFileName = "HTT-utilities/RecoilCorrections/data/TypeI-PFMet_Run2016BtoH.root";}

  svFitStandalone::kDecayType decayType1 = svFitStandalone::kUndefinedDecayType; //svFitStandalone::kTauToElecDecay
  svFitStandalone::kDecayType decayType2 = svFitStandalone::kUndefinedDecayType; //svFitStandalone::kTauToElecDecay
  // Both masses should depend on decay mode and particle?
  float mass1;
  float mass2;
  std::string channel = "x";
  std::cout << "TreeToUse: " << TreeToUse << std::endl;

           decayType1 = svFitStandalone::kTauToElecDecay;
           decayType2 = svFitStandalone::kTauToMuDecay;
           mass1 = 0.00051100;
           mass2 = 0.105658;
           channel = "em";

  TDirectory *dirsav = gDirectory;
  TIter next(dir->GetListOfKeys());
  TKey *key;
  char stringA[80]="first";
  dir->cd();      
  int k=0;
  while ((key = (TKey*)next())) {
    printf("Found key=%s \n",key->GetName());

    TObject *obj = key->ReadObj();
    if (obj->IsA()->InheritsFrom(TDirectory::Class())) {
      dir->cd(key->GetName());
      TDirectory *subdir = gDirectory;
      sprintf(TreeToUse,"%s",key->GetName());
      readdir(subdir,parser,TreeToUse,parser.doubleValue("recoilType"),parser.doubleValue("doES"),
            parser.doubleValue("isWJets"),parser.doubleValue("metType"));

      dirsav->cd();
    }
    else if(k<1 && obj->IsA()->InheritsFrom(TTree::Class())) {
      ++k;
      TTree *t = (TTree*)obj;
      float svFitMass = -10;
      float svFitPt = -10;
      float svFitEta = -10;
      float svFitPhi = -10;
      float svFitMET = -10;
      float svFitTransverseMass = -10;

      float metcorr_ex = -10; // corrected met px (float)
      float metcorr_ey = -10;  // corrected met py (float)
      float metcor = -10; // corrected metcor
      float metcorphi = -10; // corrected metcorphi

      //TBranch *newBranch = t->Branch(parser.stringValue("branch").c_str(),&svFitMass,(parser.stringValue("branch")+"/F").c_str());
      TBranch *newBranch1 = t->Branch("m_sv", &svFitMass, "m_sv/F");
      TBranch *newBranch2 = t->Branch("pt_sv", &svFitPt, "pt_sv/F");
      TBranch *newBranch3 = t->Branch("eta_sv", &svFitEta, "eta_sv/F");
      TBranch *newBranch4 = t->Branch("phi_sv", &svFitPhi, "phi_sv/F");
      TBranch *newBranch5 = t->Branch("met_sv", &svFitMET, "met_sv/F");
      TBranch *newBranch6 = t->Branch("mt_sv", &svFitTransverseMass, "mt_sv/F");

      TBranch *newBranch7 = t->Branch("metcorr_ex", &metcorr_ex, "metcorr_ex/F");
      TBranch *newBranch8 = t->Branch("metcorr_ey", &metcorr_ey, "metcorr_ey/F");
      TBranch *newBranch9 = t->Branch("metcor", &metcor, "metcor/F");
      TBranch *newBranch10 = t->Branch("metcorphi", &metcorphi, "metcorphi/F");

      // If doing ES shifts, we need extra ouput branches
      //
      float svFitMass_UP = -10;
      float svFitPt_UP = -10;
      float svFitEta_UP = -10;
      float svFitPhi_UP = -10;
      float svFitMET_UP = -10;
      float svFitTransverseMass_UP = -10;
      float svFitMass_DOWN = -10;
      float svFitPt_DOWN = -10;
      float svFitEta_DOWN = -10;
      float svFitPhi_DOWN = -10;
      float svFitMET_DOWN = -10;
      float svFitTransverseMass_DOWN = -10;

      float svFitMass_DM0_UP = -10;
      float svFitPt_DM0_UP = -10;
      float svFitEta_DM0_UP = -10;
      float svFitPhi_DM0_UP = -10;
      float svFitMET_DM0_UP = -10;
      float svFitTransverseMass_DM0_UP = -10;
      float svFitMass_DM0_DOWN = -10;
      float svFitPt_DM0_DOWN = -10;
      float svFitEta_DM0_DOWN = -10;
      float svFitPhi_DM0_DOWN = -10;
      float svFitMET_DM0_DOWN = -10;
      float svFitTransverseMass_DM0_DOWN = -10;

      float svFitMass_DM1_UP = -10;
      float svFitPt_DM1_UP = -10;
      float svFitEta_DM1_UP = -10;
      float svFitPhi_DM1_UP = -10;
      float svFitMET_DM1_UP = -10;
      float svFitTransverseMass_DM1_UP = -10;
      float svFitMass_DM1_DOWN = -10;
      float svFitPt_DM1_DOWN = -10;
      float svFitEta_DM1_DOWN = -10;
      float svFitPhi_DM1_DOWN = -10;
      float svFitMET_DM1_DOWN = -10;
      float svFitTransverseMass_DM1_DOWN = -10;

      float svFitMass_DM10_UP = -10;
      float svFitPt_DM10_UP = -10;
      float svFitEta_DM10_UP = -10;
      float svFitPhi_DM10_UP = -10;
      float svFitMET_DM10_UP = -10;
      float svFitTransverseMass_DM10_UP = -10;
      float svFitMass_DM10_DOWN = -10;
      float svFitPt_DM10_DOWN = -10;
      float svFitEta_DM10_DOWN = -10;
      float svFitPhi_DM10_DOWN = -10;
      float svFitMET_DM10_DOWN = -10;
      float svFitTransverseMass_DM10_DOWN = -10;

      TBranch *newBranch11 = t->Branch("m_sv_UP", &svFitMass_UP, "m_sv_UP/F");
      TBranch *newBranch12 = t->Branch("pt_sv_UP", &svFitPt_UP, "pt_sv_UP/F");
      TBranch *newBranch13 = t->Branch("eta_sv_UP", &svFitEta_UP, "eta_sv_UP/F");
      TBranch *newBranch14 = t->Branch("phi_sv_UP", &svFitPhi_UP, "phi_sv_UP/F");
      TBranch *newBranch15 = t->Branch("met_sv_UP", &svFitMET_UP, "met_sv_UP/F");
      TBranch *newBranch16 = t->Branch("mt_sv_UP", &svFitTransverseMass_UP, "mt_sv_UP/F");

      TBranch *newBranch17 = t->Branch("m_sv_DOWN", &svFitMass_DOWN, "m_sv_DOWN/F");
      TBranch *newBranch18 = t->Branch("pt_sv_DOWN", &svFitPt_DOWN, "pt_sv_DOWN/F");
      TBranch *newBranch19 = t->Branch("eta_sv_DOWN", &svFitEta_DOWN, "eta_sv_DOWN/F");
      TBranch *newBranch20 = t->Branch("phi_sv_DOWN", &svFitPhi_DOWN, "phi_sv_DOWN/F");
      TBranch *newBranch21 = t->Branch("met_sv_DOWN", &svFitMET_DOWN, "met_sv_DOWN/F");
      TBranch *newBranch22 = t->Branch("mt_sv_DOWN", &svFitTransverseMass_DOWN, "mt_sv_DOWN/F");

      /*TBranch *newBranch23 = t->Branch("m_sv_DM0_UP", &svFitMass_DM0_UP, "m_sv_DM0_UP/F");
      TBranch *newBranch24 = t->Branch("pt_sv_DM0_UP", &svFitPt_DM0_UP, "pt_sv_DM0_UP/F");
      TBranch *newBranch25 = t->Branch("eta_sv_DM0_UP", &svFitEta_DM0_UP, "eta_sv_DM0_UP/F");
      TBranch *newBranch26 = t->Branch("phi_sv_DM0_UP", &svFitPhi_DM0_UP, "phi_sv_DM0_UP/F");
      TBranch *newBranch27 = t->Branch("met_sv_DM0_UP", &svFitMET_DM0_UP, "met_sv_DM0_UP/F");
      TBranch *newBranch28 = t->Branch("mt_sv_DM0_UP", &svFitTransverseMass_DM0_UP, "mt_sv_DM0_UP/F");

      TBranch *newBranch29 = t->Branch("m_sv_DM0_DOWN", &svFitMass_DM0_DOWN, "m_sv_DM0_DOWN/F");
      TBranch *newBranch30 = t->Branch("pt_sv_DM0_DOWN", &svFitPt_DM0_DOWN, "pt_sv_DM0_DOWN/F");
      TBranch *newBranch31 = t->Branch("eta_sv_DM0_DOWN", &svFitEta_DM0_DOWN, "eta_sv_DM0_DOWN/F");
      TBranch *newBranch32 = t->Branch("phi_sv_DM0_DOWN", &svFitPhi_DM0_DOWN, "phi_sv_DM0_DOWN/F");
      TBranch *newBranch33 = t->Branch("met_sv_DM0_DOWN", &svFitMET_DM0_DOWN, "met_sv_DM0_DOWN/F");
      TBranch *newBranch34 = t->Branch("mt_sv_DM0_DOWN", &svFitTransverseMass_DM0_DOWN, "mt_sv_DM0_DOWN/F");

      TBranch *newBranch35 = t->Branch("m_sv_DM1_UP", &svFitMass_DM1_UP, "m_sv_DM1_UP/F");
      TBranch *newBranch36 = t->Branch("pt_sv_DM1_UP", &svFitPt_DM1_UP, "pt_sv_DM1_UP/F");
      TBranch *newBranch37 = t->Branch("eta_sv_DM1_UP", &svFitEta_DM1_UP, "eta_sv_DM1_UP/F");
      TBranch *newBranch38 = t->Branch("phi_sv_DM1_UP", &svFitPhi_DM1_UP, "phi_sv_DM1_UP/F");
      TBranch *newBranch39 = t->Branch("met_sv_DM1_UP", &svFitMET_DM1_UP, "met_sv_DM1_UP/F");
      TBranch *newBranch40 = t->Branch("mt_sv_DM1_UP", &svFitTransverseMass_DM1_UP, "mt_sv_DM1_UP/F");

      TBranch *newBranch41 = t->Branch("m_sv_DM1_DOWN", &svFitMass_DM1_DOWN, "m_sv_DM1_DOWN/F");
      TBranch *newBranch42 = t->Branch("pt_sv_DM1_DOWN", &svFitPt_DM1_DOWN, "pt_sv_DM1_DOWN/F");
      TBranch *newBranch43 = t->Branch("eta_sv_DM1_DOWN", &svFitEta_DM1_DOWN, "eta_sv_DM1_DOWN/F");
      TBranch *newBranch44 = t->Branch("phi_sv_DM1_DOWN", &svFitPhi_DM1_DOWN, "phi_sv_DM1_DOWN/F");
      TBranch *newBranch45 = t->Branch("met_sv_DM1_DOWN", &svFitMET_DM1_DOWN, "met_sv_DM1_DOWN/F");
      TBranch *newBranch46 = t->Branch("mt_sv_DM1_DOWN", &svFitTransverseMass_DM1_DOWN, "mt_sv_DM1_DOWN/F");

      TBranch *newBranch47 = t->Branch("m_sv_DM10_UP", &svFitMass_DM10_UP, "m_sv_DM10_UP/F");
      TBranch *newBranch48 = t->Branch("pt_sv_DM10_UP", &svFitPt_DM10_UP, "pt_sv_DM10_UP/F");
      TBranch *newBranch49 = t->Branch("eta_sv_DM10_UP", &svFitEta_DM10_UP, "eta_sv_DM10_UP/F");
      TBranch *newBranch50 = t->Branch("phi_sv_DM10_UP", &svFitPhi_DM10_UP, "phi_sv_DM10_UP/F");
      TBranch *newBranch51 = t->Branch("met_sv_DM10_UP", &svFitMET_DM10_UP, "met_sv_DM10_UP/F");
      TBranch *newBranch52 = t->Branch("mt_sv_DM10_UP", &svFitTransverseMass_DM10_UP, "mt_sv_DM10_UP/F");

      TBranch *newBranch53 = t->Branch("m_sv_DM10_DOWN", &svFitMass_DM10_DOWN, "m_sv_DM10_DOWN/F");
      TBranch *newBranch54 = t->Branch("pt_sv_DM10_DOWN", &svFitPt_DM10_DOWN, "pt_sv_DM10_DOWN/F");
      TBranch *newBranch55 = t->Branch("eta_sv_DM10_DOWN", &svFitEta_DM10_DOWN, "eta_sv_DM10_DOWN/F");
      TBranch *newBranch56 = t->Branch("phi_sv_DM10_DOWN", &svFitPhi_DM10_DOWN, "phi_sv_DM10_DOWN/F");
      TBranch *newBranch57 = t->Branch("met_sv_DM10_DOWN", &svFitMET_DM10_DOWN, "met_sv_DM10_DOWN/F");
      TBranch *newBranch58 = t->Branch("mt_sv_DM10_DOWN", &svFitTransverseMass_DM10_DOWN, "mt_sv_DM10_DOWN/F");
*/
      int evt;
      int run, lumi;
      float pt1;
      float eta1;
      float phi1;
      float gen_match_1;
      float pt2;
      float eta2;
      float phi2;
      float m2;
      int gen_match_2;
      float decayMode=-999.;
      float decayMode2;
      float mvaCovMatrix00;
      float mvaCovMatrix10;
      float mvaCovMatrix01;
      float mvaCovMatrix11;
      float pfCovMatrix00;
      float pfCovMatrix10;
      float pfCovMatrix01;
      float pfCovMatrix11;
      //float mvamet_ex, // uncorrected mva met px (float)
      //  mvamet_ey, // uncorrected mva met py (float)
      float  genPx=-999.    , // generator Z/W/Higgs px (float)
        genPy =-999.   , // generator Z/W/Higgs py (float)
        visPx =-999.   , // generator visible Z/W/Higgs px (float)
        visPy =-999.   ; // generator visible Z/W/Higgs py (float)
      int njets =-999.   ;  // number of jets (hadronic jet multiplicity) (int)

      // define MET
      double measuredMETx = 0.;
      double measuredMETy = 0.;
      float mvamet;
      float mvametphi;
      float pfmet;
      float pfmetphi;
      TLorentzVector TMet(0,0,0,0);
      // define MET covariance
      TMatrixD covMET(2, 2);
      //ele/mu variables
      TBranch *pt1branch;

      t->SetBranchAddress("evt",&evt);
      t->SetBranchAddress("run",&run);
      t->SetBranchAddress("lumi",&lumi);
      if(channel=="tt") {
        t->SetBranchAddress("t1Pt",&pt1,&pt1branch);
        t->SetBranchAddress("t1Eta",&eta1);
        t->SetBranchAddress("t1Phi",&phi1);
        t->SetBranchAddress("t1Mass",&mass1);
        t->SetBranchAddress("t1ZTTGenMatching",&gen_match_1);
        t->SetBranchAddress("t2Pt",&pt2);
        t->SetBranchAddress("t2Eta",&eta2);
        t->SetBranchAddress("t2Phi",&phi2);
        t->SetBranchAddress("t2Mass",&mass2);
        t->SetBranchAddress("t2ZTTGenMatching",&gen_match_2);
        t->SetBranchAddress("t1DecayMode",&decayMode);
        t->SetBranchAddress("t2DecayMode",&decayMode2);
        t->SetBranchAddress("t1_t2_MvaMetCovMatrix00",&mvaCovMatrix00);
        t->SetBranchAddress("t1_t2_MvaMetCovMatrix01",&mvaCovMatrix01);
        t->SetBranchAddress("t1_t2_MvaMetCovMatrix10",&mvaCovMatrix10);
        t->SetBranchAddress("t1_t2_MvaMetCovMatrix11",&mvaCovMatrix11);
        t->SetBranchAddress("t1_t2_MvaMet",&mvamet);
        t->SetBranchAddress("t1_t2_MvaMetPhi",&mvametphi);
        t->SetBranchAddress("jetVeto30", &njets);
        t->SetBranchAddress("type1_pfMetEt",&pfmet);
        t->SetBranchAddress("type1_pfMetPhi",&pfmetphi);
      }
      else {
        t->SetBranchAddress("pt_1",&pt1,&pt1branch);
        t->SetBranchAddress("eta_1",&eta1);
        t->SetBranchAddress("phi_1",&phi1);
        t->SetBranchAddress("pt_2",&pt2);
        t->SetBranchAddress("eta_2",&eta2);
        t->SetBranchAddress("phi_2",&phi2);
        t->SetBranchAddress("m_2",&m2);
        t->SetBranchAddress("gen_match_2",&gen_match_2);
        //t->SetBranchAddress("l1_decayMode",&decayMode);
        t->SetBranchAddress("l2_decayMode",&decayMode2);
        t->SetBranchAddress("mvacov00",&mvaCovMatrix00);
        t->SetBranchAddress("mvacov01",&mvaCovMatrix01);
        t->SetBranchAddress("mvacov10",&mvaCovMatrix10);
        t->SetBranchAddress("mvacov11",&mvaCovMatrix11);
        t->SetBranchAddress("mvamet",&mvamet);
        t->SetBranchAddress("mvametphi",&mvametphi);
        t->SetBranchAddress("njets", &njets);
        t->SetBranchAddress("met",&pfmet);
        t->SetBranchAddress("metphi",&pfmetphi);
      }
      // Recoil variables below
      t->SetBranchAddress( "genpX", &genPx);
      t->SetBranchAddress( "genpY", &genPy);
      t->SetBranchAddress( "vispX", &visPx);
      t->SetBranchAddress( "vispY", &visPy);
      // FOR PF MET ANALYSIS
      t->SetBranchAddress("metcov00",&pfCovMatrix00);
      t->SetBranchAddress("metcov01",&pfCovMatrix01);
      t->SetBranchAddress("metcov10",&pfCovMatrix10);
      t->SetBranchAddress("metcov11",&pfCovMatrix11);

      // use this RooT file when running on aMC@NLO DY and W+Jets MC samples
      RecoilCorrector* recoilCorrector = new RecoilCorrector(recoilFileName);
      if (metType == 1) std::cout<<"MetType: MvaMet"<<std::endl;
      if (metType == -1) std::cout<<"MetType: PF Met"<<std::endl;
      std::cout<<"recoiltype "<<recoilType<<" recoilFileName "<<recoilFileName<<std::endl;

      printf("Found tree -> weighting\n");

      // Only open this once!
      edm::FileInPath inputFileName_visPtResolution("TauAnalysis/SVfitStandalone/data/svFitVisMassAndPtResolutionPDF.root");
      TH1::AddDirectory(false);  
      TFile* inputFile_visPtResolution = new TFile(inputFileName_visPtResolution.fullPath().data());  

      for(Int_t i=0;i<t->GetEntries();++i){
         t->GetEntry(i);

         if (metType == -1) { // -1 = PF Met
             TMet.SetPtEtaPhiM(pfmet,0,pfmetphi,0);
             measuredMETx = pfmet*TMath::Cos(pfmetphi);
             measuredMETy = pfmet*TMath::Sin(pfmetphi);

             covMET[0][0] =  pfCovMatrix00;
             covMET[1][0] =  pfCovMatrix10;
             covMET[0][1] =  pfCovMatrix01;
             covMET[1][1] =  pfCovMatrix11;
         } // pf met

         metcorr_ex = measuredMETx;
         metcorr_ey = measuredMETy;

         metcor = TMath::Sqrt( metcorr_ex*metcorr_ex + metcorr_ey*metcorr_ey);
         metcorphi = TMath::ATan2( metcorr_ey, metcorr_ex );
         std::cout << " - metcor "<<metcor<<" metcorphi "<<metcorphi<<std::endl;

         if(channel=="mt"||channel=="et"){

            /*float shiftTau=1.0;
            if (gen_match_2==5 && decayMode2==0) shiftTau=0.982;
            if (gen_match_2==5 && decayMode2==1) shiftTau=1.010;
            if (gen_match_2==5 && decayMode2==10) shiftTau=1.004;
            pt2 = pt2 * shiftTau;
            double dx2, dy2;
            dx2 = pt2 * TMath::Cos( phi2 ) * (( 1. / shiftTau ) - 1.);
            dy2 = pt2 * TMath::Sin( phi2 ) * (( 1. / shiftTau ) - 1.);
            metcorr_ex = metcorr_ex + dx2;
            metcorr_ey = metcorr_ey + dy2;*/

            mass2 = m2;
            std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptons;
            measuredTauLeptons.push_back(
              svFitStandalone::MeasuredTauLepton(decayType1, pt1, eta1,  phi1, mass1));
           
           measuredTauLeptons.push_back(
              svFitStandalone::MeasuredTauLepton(decayType2,  pt2, eta2, phi2,  mass2, decayMode2)); 
           std::cout<< "evt: "<<evt<<" run: "<<run<<" lumi: "<<lumi<< " pt1 " << pt1 << " mass1 " << mass1 << " pt2: "<< pt2<< " mass2: "<< mass2 <<std::endl;        
           //modified
           runSVFit(measuredTauLeptons, inputFile_visPtResolution, metcorr_ex, metcorr_ey, covMET, 0, svFitMass, svFitPt, svFitEta, svFitPhi, svFitMET, svFitTransverseMass);
           std::cout<<"finished runningSVFit"<<std::endl;

           if(doES) {
             if (gen_match_2<=5){
                float ES_UP_scale=1.0; // this value is for jet -> tau fakes
                if (gen_match_2<5) ES_UP_scale=1.01; // for gen matched ele/muon
                if (gen_match_2==5) ES_UP_scale=1.006; // for real taus
                double pt2_UP;
                pt2_UP = pt2 * ES_UP_scale;
                double metcorr_ex_UP, metcorr_ey_UP;
                double dx2_UP, dy2_UP;
                dx2_UP = pt2 * TMath::Cos( phi2 ) * (( 1. / ES_UP_scale ) - 1.);
                dy2_UP = pt2 * TMath::Sin( phi2 ) * (( 1. / ES_UP_scale ) - 1.);
                metcorr_ex_UP = metcorr_ex + dx2_UP;
                metcorr_ey_UP = metcorr_ey + dy2_UP;
                
                std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptonsUP;
                measuredTauLeptonsUP.push_back(
                 svFitStandalone::MeasuredTauLepton(decayType1, pt1, eta1,  phi1, mass1));
                measuredTauLeptonsUP.push_back(
                 svFitStandalone::MeasuredTauLepton(decayType2,  pt2_UP, eta2, phi2,  mass2, decayMode2));

                runSVFit(measuredTauLeptonsUP, inputFile_visPtResolution, metcorr_ex_UP, metcorr_ey_UP, covMET, 0, svFitMass_UP, svFitPt_UP, svFitEta_UP, svFitPhi_UP, svFitMET_UP, svFitTransverseMass_UP);
           }
           else {
               svFitMass_UP=svFitMass;
               svFitPt_UP=svFitPt;
               svFitEta_UP=svFitEta;
               svFitPhi_UP=svFitPhi;
               svFitMET_UP=svFitMET;
               svFitTransverseMass_UP=svFitTransverseMass;
           }

          // Second ES Down, x 0.97
          if (gen_match_2<=5){
             float ES_DOWN_scale=1.0; // jet
             if (gen_match_2==5) ES_DOWN_scale=0.994; // tau
             if (gen_match_2<5) ES_DOWN_scale=0.990;  // elec/mu
             double pt2_DOWN;
             pt2_DOWN = pt2 * ES_DOWN_scale;
             double metcorr_ex_DOWN, metcorr_ey_DOWN;
             double dx2_DOWN, dy2_DOWN;
             dx2_DOWN = pt2 * TMath::Cos( phi2 ) * (( 1. / ES_DOWN_scale ) - 1.);
             dy2_DOWN = pt2 * TMath::Sin( phi2 ) * (( 1. / ES_DOWN_scale ) - 1.);
             metcorr_ex_DOWN = metcorr_ex + dx2_DOWN;
             metcorr_ey_DOWN = metcorr_ey + dy2_DOWN;

             std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptonsDOWN;          
             measuredTauLeptonsDOWN.push_back(
              svFitStandalone::MeasuredTauLepton(decayType1, pt1, eta1,  phi1, mass1));
             measuredTauLeptonsDOWN.push_back(
              svFitStandalone::MeasuredTauLepton(decayType2,  pt2_DOWN, eta2, phi2,  mass2, decayMode2));

             runSVFit(measuredTauLeptonsDOWN, inputFile_visPtResolution, metcorr_ex_DOWN, metcorr_ey_DOWN, covMET, 0, svFitMass_DOWN, svFitPt_DOWN, svFitEta_DOWN, svFitPhi_DOWN, svFitMET_DOWN, svFitTransverseMass_DOWN);
          }
          else {
              svFitMass_DOWN=svFitMass;
              svFitPt_DOWN=svFitPt;
              svFitEta_DOWN=svFitEta;
              svFitPhi_DOWN=svFitPhi;
              svFitMET_DOWN=svFitMET;
              svFitTransverseMass_DOWN=svFitTransverseMass;
          }
        } // end doES
      } // eTau / muTau

      else if(channel=="em"){
        // define lepton four vectors
        std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptons;
        measuredTauLeptons.push_back(
         svFitStandalone::MeasuredTauLepton(decayType1, pt1, eta1,  phi1, mass1));
        measuredTauLeptons.push_back(
         svFitStandalone::MeasuredTauLepton(decayType2,  pt2, eta2, phi2, mass2));

        std::cout<< "evt: "<<evt<<" run: "<<run<<" lumi: "<<lumi<< " pt1 " << pt1 << " mass1 " << mass1 << " pt2: "<< pt2<< " mass2: "<< mass2 <<std::endl;        
        runSVFit(measuredTauLeptons, inputFile_visPtResolution, metcorr_ex, metcorr_ey, covMET, 0, svFitMass, svFitPt, svFitEta, svFitPhi, svFitMET, svFitTransverseMass);
        std::cout<<"finished running non-EES SVFit in EMu"<<std::endl;
        if(doES) {

          // Only shift Electrons
          // 1% shift in barrel and 2.5% shift in endcap
          // applied to electrons in emu channel
          float etaBarrelEndcap  = 1.479;
          float ES_UP_scale;
          if (abs(eta1) < etaBarrelEndcap) ES_UP_scale = 1.01;
          else ES_UP_scale = 1.025;
          double pt1_UP = pt1 * ES_UP_scale;
          std::cout << "E eta: " << eta1 << " ees SF: " << ES_UP_scale << std::endl;
          double metcorr_ex_UP, metcorr_ey_UP;
          double dx1_UP, dy1_UP;
          dx1_UP = pt1_UP * TMath::Cos( phi1 ) * (( 1. / ES_UP_scale ) - 1.);
          dy1_UP = pt1_UP * TMath::Sin( phi1 ) * (( 1. / ES_UP_scale ) - 1.);
          metcorr_ex_UP = metcorr_ex + dx1_UP;
          metcorr_ey_UP = metcorr_ey + dy1_UP;
          std::cout << "px1 " << pt1 * TMath::Cos( phi1 ) << "  met px1 cor " << dx1_UP <<std::endl;
          std::cout << "py1 " << pt1 * TMath::Sin( phi1 ) << "  met py1 cor " << dy1_UP <<std::endl;
          std::cout << "pt1 " << pt1 << "  pt1_up " << pt1_UP <<std::endl;
          std::cout << "metcor_ex " << metcorr_ex << " ees: " << metcorr_ex_UP << std::endl;
          std::cout << "metcor_ey " << metcorr_ey << " ees: " << metcorr_ey_UP << std::endl;
          
          std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptonsUP;
          measuredTauLeptonsUP.push_back(
           svFitStandalone::MeasuredTauLepton(decayType1, pt1_UP, eta1,  phi1, mass1));
          measuredTauLeptonsUP.push_back(
           svFitStandalone::MeasuredTauLepton(decayType2,  pt2, eta2, phi2, mass2));

          std::cout<< "evt: "<<evt<<" run: "<<run<<" lumi: "<<lumi<< " pt1 " << pt1_UP << " mass1 " << mass1 << " pt2: "<< pt2 << " mass2: "<< mass2 <<std::endl;        
          runSVFit(measuredTauLeptonsUP, inputFile_visPtResolution, metcorr_ex_UP, metcorr_ey_UP, covMET, 0, svFitMass_UP, svFitPt_UP, svFitEta_UP, svFitPhi_UP, svFitMET_UP, svFitTransverseMass_UP);
          std::cout<<"finished runningSVFit in EMu EES Up"<<std::endl;


          // EES Down
          float ES_DOWN_scale;
          if (abs(eta1) < etaBarrelEndcap) ES_DOWN_scale = 0.99;
          else ES_DOWN_scale = 0.975;
          std::cout << "E eta: " << eta1 << " ees SF: " << ES_DOWN_scale << std::endl;
          double pt1_DOWN;
          pt1_DOWN = pt1 * ES_DOWN_scale;
          double metcorr_ex_DOWN, metcorr_ey_DOWN;
          double dx1_DOWN, dy1_DOWN;
          dx1_DOWN = pt1_DOWN * TMath::Cos( phi1 ) * (( 1. / ES_DOWN_scale ) - 1.);
          dy1_DOWN = pt1_DOWN * TMath::Sin( phi1 ) * (( 1. / ES_DOWN_scale ) - 1.);
          metcorr_ex_DOWN = metcorr_ex + dx1_DOWN;
          metcorr_ey_DOWN = metcorr_ey + dy1_DOWN;
          std::cout << "px1 " << pt1 * TMath::Cos( phi1 ) << "  met px1 cor " << dx1_DOWN <<std::endl;
          std::cout << "py1 " << pt1 * TMath::Sin( phi1 ) << "  met py1 cor " << dy1_DOWN <<std::endl;
          std::cout << "metcor_ex " << metcorr_ex << " ees: " << metcorr_ex_DOWN << std::endl;
          std::cout << "metcor_ey " << metcorr_ey << " ees: " << metcorr_ey_DOWN << std::endl;

          std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptonsDOWN;
          measuredTauLeptonsDOWN.push_back(
           svFitStandalone::MeasuredTauLepton(decayType1, pt1_DOWN, eta1,  phi1, mass1));
          measuredTauLeptonsDOWN.push_back(
           svFitStandalone::MeasuredTauLepton(decayType2,  pt2, eta2, phi2,  mass2));

          runSVFit(measuredTauLeptonsDOWN, inputFile_visPtResolution, metcorr_ex_DOWN, metcorr_ey_DOWN, covMET, 0, svFitMass_DOWN, svFitPt_DOWN, svFitEta_DOWN, svFitPhi_DOWN, svFitMET_DOWN, svFitTransverseMass_DOWN);


        } // end doES
      } // eMu



      else if(channel=="tt"){

         float shiftTau1=1.0;
         float shiftTau2=1.0;
         if (gen_match_1==5 && decayMode==0) shiftTau1=0.982;
         if (gen_match_1==5 && decayMode==1) shiftTau1=1.010;
         if (gen_match_1==5 && decayMode==10) shiftTau1=1.004;
         if (gen_match_2==5 && decayMode2==0) shiftTau2=0.982;
         if (gen_match_2==5 && decayMode2==1) shiftTau2=1.010;
         if (gen_match_2==5 && decayMode2==10) shiftTau2=1.004;
         pt1 = pt1 * shiftTau1;
         pt2 = pt2 * shiftTau2;
         double dx1, dy1;
         double dx2, dy2;
         dx1 = pt1 * TMath::Cos( phi1 ) * (( 1. / shiftTau1 ) - 1.);
         dy1 = pt1 * TMath::Sin( phi1 ) * (( 1. / shiftTau1 ) - 1.);
         dx2 = pt2 * TMath::Cos( phi2 ) * (( 1. / shiftTau2 ) - 1.);
         dy2 = pt2 * TMath::Sin( phi2 ) * (( 1. / shiftTau2 ) - 1.);
         metcorr_ex = metcorr_ex + dx1;
         metcorr_ey = metcorr_ey + dy1;
         metcorr_ex = metcorr_ex + dx2;
         metcorr_ey = metcorr_ey + dy2;

        // define lepton four vectors
        std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptons;
        // Add Tau of higest Pt first
        if (pt1 > pt2) {
          measuredTauLeptons.push_back(
           svFitStandalone::MeasuredTauLepton(decayType1, pt1, eta1,  phi1, mass1, decayMode));
        
          measuredTauLeptons.push_back(
           svFitStandalone::MeasuredTauLepton(decayType2,  pt2, eta2, phi2,  mass2, decayMode2));
        }
        else {
          measuredTauLeptons.push_back(
           svFitStandalone::MeasuredTauLepton(decayType2,  pt2, eta2, phi2,  mass2, decayMode2));

          measuredTauLeptons.push_back(
           svFitStandalone::MeasuredTauLepton(decayType1, pt1, eta1,  phi1, mass1, decayMode));
        }

        std::cout<< "evt: "<<evt<<" run: "<<run<<" lumi: "<<lumi<< " pt1 " << pt1 << " mass1 " << mass1 << " pt2: "<< pt2<< " mass2: "<< mass2 <<std::endl;        
        runSVFit(measuredTauLeptons, inputFile_visPtResolution, metcorr_ex, metcorr_ey, covMET, 0, svFitMass, svFitPt, svFitEta, svFitPhi, svFitMET, svFitTransverseMass);
        std::cout<<"finished running non-TES SVFit in TT"<<std::endl;

        if(doES) {

      //***************************************************************************
      //********************* Two taus shifted up *********************************
      //***************************************************************************
      
      if (gen_match_2==5 or gen_match_1==5){
             std::cout << "Two UP    ---  ";
             float ES_UP_scale1 = 1.0;
             float ES_UP_scale2 = 1.0;
             if(gen_match_1==5) ES_UP_scale1 = 1.006;
             if(gen_match_2==5) ES_UP_scale2 = 1.006;
             std::cout << "TES values: gen1: " << gen_match_1 << "   dm_1: " << decayMode;
             std::cout << "   tes1: " << ES_UP_scale1;
             std::cout << "   gen2: " << gen_match_2 << "   dm_2: " << decayMode2;
             std::cout << "   tes2: " << ES_UP_scale2 << std::endl;
             double pt1_UP, pt2_UP;
             pt1_UP = pt1 * ES_UP_scale1;
             pt2_UP = pt2 * ES_UP_scale2;
             double metcorr_ex_UP, metcorr_ey_UP;
             double dx1_UP, dy1_UP, dx2_UP, dy2_UP;
             dx1_UP = pt1 * TMath::Cos( phi1 ) * (( 1. / ES_UP_scale1 ) - 1.);
             dy1_UP = pt1 * TMath::Sin( phi1 ) * (( 1. / ES_UP_scale1 ) - 1.);
             dx2_UP = pt2 * TMath::Cos( phi2 ) * (( 1. / ES_UP_scale2 ) - 1.);
             dy2_UP = pt2 * TMath::Sin( phi2 ) * (( 1. / ES_UP_scale2 ) - 1.);
             metcorr_ex_UP = metcorr_ex + dx1_UP + dx2_UP;
             metcorr_ey_UP = metcorr_ey + dy1_UP + dy2_UP;
             
             std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptonsUP;
             
             // Add Tau of higest Pt first
             if (pt1 > pt2) {
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_UP, eta1,  phi1, mass1, decayMode));
             
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_UP, eta2, phi2,  mass2, decayMode2));
             }
             else {
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_UP, eta2, phi2,  mass2, decayMode2));

               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_UP, eta1,  phi1, mass1, decayMode));
             }

             runSVFit(measuredTauLeptonsUP, inputFile_visPtResolution, metcorr_ex_UP, metcorr_ey_UP, covMET, 0, svFitMass_UP, svFitPt_UP, svFitEta_UP, svFitPhi_UP, svFitMET_UP, svFitTransverseMass_UP);
          }
          else {
              svFitMass_UP=svFitMass;
              svFitPt_UP=svFitPt;
              svFitEta_UP=svFitEta;
              svFitPhi_UP=svFitPhi;
              svFitMET_UP=svFitMET;
              svFitTransverseMass_UP=svFitTransverseMass;
          }

          //***************************************************************************
          //********************** Tau DM0 shifted up *********************************
          //***************************************************************************

          if ((gen_match_2==5 && decayMode2==0) or (gen_match_1==5 && decayMode==0)){
             std::cout << "DM0 UP    ---  ";
             float ES_UP_scale1 = 1.0;
             float ES_UP_scale2 = 1.0;
             if(gen_match_1==5 && decayMode==0) ES_UP_scale1 = 1.006;
             if(gen_match_2==5 && decayMode2==0) ES_UP_scale2 = 1.006;
             double pt1_UP, pt2_UP;
             pt1_UP = pt1 * ES_UP_scale1;
             pt2_UP = pt2 * ES_UP_scale2;
             double metcorr_ex_UP, metcorr_ey_UP;
             double dx1_UP, dy1_UP, dx2_UP, dy2_UP;
             dx1_UP = pt1 * TMath::Cos( phi1 ) * (( 1. / ES_UP_scale1 ) - 1.);
             dy1_UP = pt1 * TMath::Sin( phi1 ) * (( 1. / ES_UP_scale1 ) - 1.);
             dx2_UP = pt2 * TMath::Cos( phi2 ) * (( 1. / ES_UP_scale2 ) - 1.);
             dy2_UP = pt2 * TMath::Sin( phi2 ) * (( 1. / ES_UP_scale2 ) - 1.);
             metcorr_ex_UP = metcorr_ex + dx1_UP + dx2_UP;
             metcorr_ey_UP = metcorr_ey + dy1_UP + dy2_UP;
          
             std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptonsUP;
             // Add Tau of higest Pt first
             if (pt1 > pt2) {
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_UP, eta1,  phi1, mass1, decayMode));
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_UP, eta2, phi2,  mass2, decayMode2));
             }
             else {
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_UP, eta2, phi2,  mass2, decayMode2));
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_UP, eta1,  phi1, mass1, decayMode));
             }

             runSVFit(measuredTauLeptonsUP, inputFile_visPtResolution, metcorr_ex_UP, metcorr_ey_UP, covMET, 0, svFitMass_DM0_UP, svFitPt_DM0_UP, svFitEta_DM0_UP, svFitPhi_DM0_UP, svFitMET_DM0_UP, svFitTransverseMass_DM0_UP);
          }
          else {
              svFitMass_DM0_UP=svFitMass;
              svFitPt_DM0_UP=svFitPt;
              svFitEta_DM0_UP=svFitEta;
              svFitPhi_DM0_UP=svFitPhi;
              svFitMET_DM0_UP=svFitMET;
              svFitTransverseMass_DM0_UP=svFitTransverseMass;
          }

          //***************************************************************************
          //********************** Tau DM1 shifted up *********************************
          //***************************************************************************

          if ((decayMode==1 && gen_match_1==5) or (decayMode2==1 && gen_match_2==5)){
             std::cout << "DM1 UP    ---  ";
             float ES_UP_scale1 = 1.0;
             float ES_UP_scale2 = 1.0;
             if (decayMode==1 && gen_match_1==5) ES_UP_scale1 = 1.006;
             if (decayMode2==1 && gen_match_2==5) ES_UP_scale2 = 1.006;
             double pt1_UP, pt2_UP;
             pt1_UP = pt1 * ES_UP_scale1;
             pt2_UP = pt2 * ES_UP_scale2;
             double metcorr_ex_UP, metcorr_ey_UP;
             double dx1_UP, dy1_UP, dx2_UP, dy2_UP;
             dx1_UP = pt1 * TMath::Cos( phi1 ) * (( 1. / ES_UP_scale1 ) - 1.);
             dy1_UP = pt1 * TMath::Sin( phi1 ) * (( 1. / ES_UP_scale1 ) - 1.);
             dx2_UP = pt2 * TMath::Cos( phi2 ) * (( 1. / ES_UP_scale2 ) - 1.);
             dy2_UP = pt2 * TMath::Sin( phi2 ) * (( 1. / ES_UP_scale2 ) - 1.);
             metcorr_ex_UP = metcorr_ex + dx1_UP + dx2_UP;
             metcorr_ey_UP = metcorr_ey + dy1_UP + dy2_UP;
             std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptonsUP;
             // Add Tau of higest Pt first
             if (pt1 > pt2) {
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_UP, eta1,  phi1, mass1, decayMode));
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_UP, eta2, phi2,  mass2, decayMode2));
             }
             else {
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_UP, eta2, phi2,  mass2, decayMode2));
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_UP, eta1,  phi1, mass1, decayMode));
             }

             runSVFit(measuredTauLeptonsUP, inputFile_visPtResolution, metcorr_ex_UP, metcorr_ey_UP, covMET, 0, svFitMass_DM1_UP, svFitPt_DM1_UP, svFitEta_DM1_UP, svFitPhi_DM1_UP, svFitMET_DM1_UP, svFitTransverseMass_DM1_UP);
          }
          else {
              svFitMass_DM1_UP=svFitMass;
              svFitPt_DM1_UP=svFitPt;
              svFitEta_DM1_UP=svFitEta;
              svFitPhi_DM1_UP=svFitPhi;
              svFitMET_DM1_UP=svFitMET;
              svFitTransverseMass_DM1_UP=svFitTransverseMass;
          }

          //***************************************************************************
          //********************* Tau DM10 shifted up *********************************
          //***************************************************************************

          if ((decayMode2==10 && gen_match_2==5) or (decayMode==10 && gen_match_1==5)){
             std::cout << "DM10 UP    ---  ";
             float ES_UP_scale1 = 1.0;
             float ES_UP_scale2 = 1.0;
             if(decayMode==10 && gen_match_1==5) ES_UP_scale1 = 1.006;
             if(decayMode2==10 && gen_match_2==5) ES_UP_scale2 = 1.006;
             double pt1_UP, pt2_UP;
             pt1_UP = pt1 * ES_UP_scale1;
             pt2_UP = pt2 * ES_UP_scale2;
             double metcorr_ex_UP, metcorr_ey_UP;
             double dx1_UP, dy1_UP, dx2_UP, dy2_UP;
             dx1_UP = pt1 * TMath::Cos( phi1 ) * (( 1. / ES_UP_scale1 ) - 1.);
             dy1_UP = pt1 * TMath::Sin( phi1 ) * (( 1. / ES_UP_scale1 ) - 1.);
             dx2_UP = pt2 * TMath::Cos( phi2 ) * (( 1. / ES_UP_scale2 ) - 1.);
             dy2_UP = pt2 * TMath::Sin( phi2 ) * (( 1. / ES_UP_scale2 ) - 1.);
             metcorr_ex_UP = metcorr_ex + dx1_UP + dx2_UP;
             metcorr_ey_UP = metcorr_ey + dy1_UP + dy2_UP;
          
             std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptonsUP;
          
             // Add Tau of higest Pt first
             if (pt1 > pt2) {
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_UP, eta1,  phi1, mass1, decayMode));
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_UP, eta2, phi2,  mass2, decayMode2));
             }
             else {
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_UP, eta2, phi2,  mass2, decayMode2));
               measuredTauLeptonsUP.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_UP, eta1,  phi1, mass1, decayMode));
             }

             runSVFit(measuredTauLeptonsUP, inputFile_visPtResolution, metcorr_ex_UP, metcorr_ey_UP, covMET, 0, svFitMass_DM10_UP, svFitPt_DM10_UP, svFitEta_DM10_UP, svFitPhi_DM10_UP, svFitMET_DM10_UP, svFitTransverseMass_DM10_UP);
          }
          else {
              svFitMass_DM10_UP=svFitMass;
              svFitPt_DM10_UP=svFitPt;
              svFitEta_DM10_UP=svFitEta;
              svFitPhi_DM10_UP=svFitPhi;
              svFitMET_DM10_UP=svFitMET;
              svFitTransverseMass_DM10_UP=svFitTransverseMass;
          }

      //*****************************************************
      //************* Two taus shifted down *****************
      //*****************************************************

          if (gen_match_1==5 or gen_match_2==5){
             std::cout << "Two DOWN  ---  ";
             float ES_DOWN_scale1 = 1.0;
             float ES_DOWN_scale2 = 1.0;
             if (gen_match_1==5) ES_DOWN_scale1 = 0.994;
             if (gen_match_2==5) ES_DOWN_scale2 = 0.994;
             double pt1_DOWN, pt2_DOWN;
             pt1_DOWN = pt1 * ES_DOWN_scale1;
             pt2_DOWN = pt2 * ES_DOWN_scale2;
             double metcorr_ex_DOWN, metcorr_ey_DOWN;
             double dx1_DOWN, dy1_DOWN, dx2_DOWN, dy2_DOWN;
             dx1_DOWN = pt1 * TMath::Cos( phi1 ) * (( 1. / ES_DOWN_scale1 ) - 1.);
             dy1_DOWN = pt1 * TMath::Sin( phi1 ) * (( 1. / ES_DOWN_scale1 ) - 1.);
             dx2_DOWN = pt2 * TMath::Cos( phi2 ) * (( 1. / ES_DOWN_scale2 ) - 1.);
             dy2_DOWN = pt2 * TMath::Sin( phi2 ) * (( 1. / ES_DOWN_scale2 ) - 1.);
             metcorr_ex_DOWN = metcorr_ex + dx1_DOWN + dx2_DOWN;
             metcorr_ey_DOWN = metcorr_ey + dy1_DOWN + dy2_DOWN;

             std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptonsDOWN;
             
             if (pt1 > pt2) {
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_DOWN, eta1,  phi1, mass1, decayMode));
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_DOWN, eta2, phi2,  mass2, decayMode2));
             }
             else {
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_DOWN, eta2, phi2,  mass2, decayMode2));
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_DOWN, eta1,  phi1, mass1, decayMode));
             }

             runSVFit(measuredTauLeptonsDOWN, inputFile_visPtResolution, metcorr_ex_DOWN, metcorr_ey_DOWN, covMET, 0, svFitMass_DOWN, svFitPt_DOWN, svFitEta_DOWN, svFitPhi_DOWN, svFitMET_DOWN, svFitTransverseMass_DOWN);
          }
          else {
              svFitMass_DOWN=svFitMass;
              svFitPt_DOWN=svFitPt;
              svFitEta_DOWN=svFitEta;
              svFitPhi_DOWN=svFitPhi;
              svFitMET_DOWN=svFitMET;
              svFitTransverseMass_DOWN=svFitTransverseMass;
          }

          //*****************************************************
          //************* Tau DM0 shifted down  *****************
          //*****************************************************

          if ((decayMode==0 && gen_match_1==5) or (decayMode2==0 && gen_match_2==5)){
             std::cout << "DM0 DOWN  ---  ";
             float ES_DOWN_scale1 = 1.0;
             float ES_DOWN_scale2 = 1.0;
             if (decayMode==0 && gen_match_1==5) ES_DOWN_scale1 = 0.994;
             if (decayMode2==0 && gen_match_2==5) ES_DOWN_scale2 = 0.994;
             double pt1_DOWN, pt2_DOWN;
             pt1_DOWN = pt1 * ES_DOWN_scale1;
             pt2_DOWN = pt2 * ES_DOWN_scale2;
             double metcorr_ex_DOWN, metcorr_ey_DOWN;
             double dx1_DOWN, dy1_DOWN, dx2_DOWN, dy2_DOWN;
             dx1_DOWN = pt1 * TMath::Cos( phi1 ) * (( 1. / ES_DOWN_scale1 ) - 1.);
             dy1_DOWN = pt1 * TMath::Sin( phi1 ) * (( 1. / ES_DOWN_scale1 ) - 1.);
             dx2_DOWN = pt2 * TMath::Cos( phi2 ) * (( 1. / ES_DOWN_scale2 ) - 1.);
             dy2_DOWN = pt2 * TMath::Sin( phi2 ) * (( 1. / ES_DOWN_scale2 ) - 1.);
             metcorr_ex_DOWN = metcorr_ex + dx1_DOWN + dx2_DOWN;
             metcorr_ey_DOWN = metcorr_ey + dy1_DOWN + dy2_DOWN;

             std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptonsDOWN;
             if (pt1 > pt2) {
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_DOWN, eta1,  phi1, mass1, decayMode));
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_DOWN, eta2, phi2,  mass2, decayMode2));
             }
             else {
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_DOWN, eta2, phi2,  mass2, decayMode2));
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_DOWN, eta1,  phi1, mass1, decayMode));
             }

             runSVFit(measuredTauLeptonsDOWN, inputFile_visPtResolution, metcorr_ex_DOWN, metcorr_ey_DOWN, covMET, 0, svFitMass_DM0_DOWN, svFitPt_DM0_DOWN, svFitEta_DM0_DOWN, svFitPhi_DM0_DOWN, svFitMET_DM0_DOWN, svFitTransverseMass_DM0_DOWN);
          }
          else {
              svFitMass_DM0_DOWN=svFitMass;
              svFitPt_DM0_DOWN=svFitPt;
              svFitEta_DM0_DOWN=svFitEta;
              svFitPhi_DM0_DOWN=svFitPhi;
              svFitMET_DM0_DOWN=svFitMET;
              svFitTransverseMass_DM0_DOWN=svFitTransverseMass;
          }

          //*****************************************************
          //************** Tau DM1 shifted down *****************
          //*****************************************************

          if ((decayMode==1 && gen_match_1==5) or (decayMode2==1 && gen_match_2==5)){
             std::cout << "DM1 DOWN  ---  ";
             float ES_DOWN_scale1 = 1.0;
             float ES_DOWN_scale2 = 1.0;
             if (decayMode==1 && gen_match_1==5) ES_DOWN_scale1 = 0.994;
             if (decayMode2==1 && gen_match_2==5) ES_DOWN_scale2 = 0.994;
             double pt1_DOWN, pt2_DOWN;
             pt1_DOWN = pt1 * ES_DOWN_scale1;
             pt2_DOWN = pt2 * ES_DOWN_scale2;
             double metcorr_ex_DOWN, metcorr_ey_DOWN;
             double dx1_DOWN, dy1_DOWN, dx2_DOWN, dy2_DOWN;
             dx1_DOWN = pt1 * TMath::Cos( phi1 ) * (( 1. / ES_DOWN_scale1 ) - 1.);
             dy1_DOWN = pt1 * TMath::Sin( phi1 ) * (( 1. / ES_DOWN_scale1 ) - 1.);
             dx2_DOWN = pt2 * TMath::Cos( phi2 ) * (( 1. / ES_DOWN_scale2 ) - 1.);
             dy2_DOWN = pt2 * TMath::Sin( phi2 ) * (( 1. / ES_DOWN_scale2 ) - 1.);
             metcorr_ex_DOWN = metcorr_ex + dx1_DOWN + dx2_DOWN;
             metcorr_ey_DOWN = metcorr_ey + dy1_DOWN + dy2_DOWN;

             std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptonsDOWN;

             if (pt1 > pt2) {
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_DOWN, eta1,  phi1, mass1, decayMode));
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_DOWN, eta2, phi2,  mass2, decayMode2));
             }
             else {
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_DOWN, eta2, phi2,  mass2, decayMode2));
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_DOWN, eta1,  phi1, mass1, decayMode));
             }

             runSVFit(measuredTauLeptonsDOWN, inputFile_visPtResolution, metcorr_ex_DOWN, metcorr_ey_DOWN, covMET, 0, svFitMass_DM1_DOWN, svFitPt_DM1_DOWN, svFitEta_DM1_DOWN, svFitPhi_DM1_DOWN, svFitMET_DM1_DOWN, svFitTransverseMass_DM1_DOWN);
          }
          else {
              svFitMass_DM1_DOWN=svFitMass;
              svFitPt_DM1_DOWN=svFitPt;
              svFitEta_DM1_DOWN=svFitEta;
              svFitPhi_DM1_DOWN=svFitPhi;
              svFitMET_DM1_DOWN=svFitMET;
              svFitTransverseMass_DM1_DOWN=svFitTransverseMass;
          }

          //*****************************************************
          //************* Tau DM10 shifted down *****************
          //*****************************************************

          if ((decayMode==10 && gen_match_1==5) or (decayMode2==10 && gen_match_2==5)){
             std::cout << "DM10 DOWN  ---  ";
             float ES_DOWN_scale1 = 1.0;
             float ES_DOWN_scale2 = 1.0;
             if (decayMode==10 && gen_match_1==5) ES_DOWN_scale1 = 0.994;
             if (decayMode2==10 && gen_match_2==5) ES_DOWN_scale2 = 0.994;
             double pt1_DOWN, pt2_DOWN;
             pt1_DOWN = pt1 * ES_DOWN_scale1;
             pt2_DOWN = pt2 * ES_DOWN_scale2;
             double metcorr_ex_DOWN, metcorr_ey_DOWN;
             double dx1_DOWN, dy1_DOWN, dx2_DOWN, dy2_DOWN;
             dx1_DOWN = pt1 * TMath::Cos( phi1 ) * (( 1. / ES_DOWN_scale1 ) - 1.);
             dy1_DOWN = pt1 * TMath::Sin( phi1 ) * (( 1. / ES_DOWN_scale1 ) - 1.);
             dx2_DOWN = pt2 * TMath::Cos( phi2 ) * (( 1. / ES_DOWN_scale2 ) - 1.);
             dy2_DOWN = pt2 * TMath::Sin( phi2 ) * (( 1. / ES_DOWN_scale2 ) - 1.);
             metcorr_ex_DOWN = metcorr_ex + dx1_DOWN + dx2_DOWN;
             metcorr_ey_DOWN = metcorr_ey + dy1_DOWN + dy2_DOWN;

             std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptonsDOWN;
             if (pt1 > pt2) {
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_DOWN, eta1,  phi1, mass1, decayMode));
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_DOWN, eta2, phi2,  mass2, decayMode2));
             }
             else {
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType2,  pt2_DOWN, eta2, phi2,  mass2, decayMode2));
               measuredTauLeptonsDOWN.push_back(
                svFitStandalone::MeasuredTauLepton(decayType1, pt1_DOWN, eta1,  phi1, mass1, decayMode));
             }

             runSVFit(measuredTauLeptonsDOWN, inputFile_visPtResolution, metcorr_ex_DOWN, metcorr_ey_DOWN, covMET, 0, svFitMass_DM10_DOWN, svFitPt_DM10_DOWN, svFitEta_DM10_DOWN, svFitPhi_DM10_DOWN, svFitMET_DM10_DOWN, svFitTransverseMass_DM10_DOWN);
          }
          else {
              svFitMass_DM10_DOWN=svFitMass;
              svFitPt_DM10_DOWN=svFitPt;
              svFitEta_DM10_DOWN=svFitEta;
              svFitPhi_DM10_DOWN=svFitPhi;
              svFitMET_DM10_DOWN=svFitMET;
              svFitTransverseMass_DM10_DOWN=svFitTransverseMass;
          }
        }// Do ES (TT)

      } // Double Hadronic (TT)

      else {
        svFitMass = -100;
        svFitPt = -100;
        svFitEta = -100;
        svFitPhi = -100;
        svFitMET = -100;
        svFitTransverseMass = -100;
      }
      std::cout << "\n\n" << std::endl;
      //std::cout << "\n\nex: " << metcorr_ex << "   ey: " << metcorr_ey <<  " phi: " << metcorphi<<"\n"<<std::endl; 
      std::cout<<svFitMass<<std::endl;
      newBranch1->Fill();
      newBranch2->Fill();
      newBranch3->Fill();
      newBranch4->Fill();
      newBranch5->Fill();
      newBranch6->Fill();
      newBranch7->Fill();
      newBranch8->Fill();
      newBranch9->Fill();
      newBranch10->Fill();

      newBranch11->Fill();
      newBranch12->Fill();
      newBranch13->Fill();
      newBranch14->Fill();
      newBranch15->Fill();
      newBranch16->Fill();
      newBranch17->Fill();
      newBranch18->Fill();
      newBranch19->Fill();
      newBranch20->Fill();
      newBranch21->Fill();
      newBranch22->Fill();

      /*newBranch23->Fill();
      newBranch24->Fill();
      newBranch25->Fill();
      newBranch26->Fill();
      newBranch27->Fill();
      newBranch28->Fill();
      newBranch29->Fill();
      newBranch30->Fill();
      newBranch31->Fill();
      newBranch32->Fill();
      newBranch33->Fill();
      newBranch34->Fill();

      newBranch35->Fill();
      newBranch36->Fill();
      newBranch37->Fill();
      newBranch38->Fill();
      newBranch39->Fill();
      newBranch40->Fill();
      newBranch41->Fill();
      newBranch42->Fill();
      newBranch43->Fill();
      newBranch44->Fill();
      newBranch45->Fill();
      newBranch46->Fill();

      newBranch47->Fill();
      newBranch48->Fill();
      newBranch49->Fill();
      newBranch50->Fill();
      newBranch51->Fill();
      newBranch52->Fill();
      newBranch53->Fill();
      newBranch54->Fill();
      newBranch55->Fill();
      newBranch56->Fill();
      newBranch57->Fill();
      newBranch58->Fill();*/

    }
      delete inputFile_visPtResolution;
      dir->cd();
      t->Write("",TObject::kOverwrite);
      strcpy(TreeToUse,stringA) ;

    }
  }
}

void runSVFit(std::vector<svFitStandalone::MeasuredTauLepton> & measuredTauLeptons, TFile * inputFile_visPtResolution, double measuredMETx, double measuredMETy, TMatrixD &covMET, float num, float &svFitMass, float& svFitPt, float &svFitEta, float &svFitPhi, float &svFitMET, float &svFitTransverseMass){

  //edm::FileInPath inputFileName_visPtResolution("TauAnalysis/SVfitStandalone/data/svFitVisMassAndPtResolutionPDF.root");
  //TH1::AddDirectory(false);  
  //TFile* inputFile_visPtResolution = new TFile(inputFileName_visPtResolution.fullPath().data());  
  //float svFitMass = 0;
  SVfitStandaloneAlgorithm algo(measuredTauLeptons, measuredMETx, measuredMETy, covMET, 0);
  algo.addLogM(false);  
  algo.shiftVisPt(true, inputFile_visPtResolution);
  algo.integrateMarkovChain();
  svFitMass = algo.getMass(); // return value is in units of GeV
  svFitPt = algo.pt();
  svFitEta = algo.eta();
  svFitPhi = algo.phi();
  svFitMET = algo.fittedMET().Rho();
  svFitTransverseMass = algo.transverseMass();
  if ( algo.isValidSolution() ) {
    std::cout << "found mass = " << svFitMass << std::endl;
  } else {
    std::cout << "sorry -- status of NLL is not valid [" << algo.isValidSolution() << "]" << std::endl;
  }
  //delete inputFile_visPtResolution;

}

//Thank you Renee Brun :)
void CopyDir(TDirectory *source, optutl::CommandLineParser parser) {
  //copy all objects and subdirs of directory source as a subdir of the current directory
  TDirectory *savdir = gDirectory;
  TDirectory *adir = savdir; 
  if(source->GetName()!=parser.stringValue("inputFile")){
    adir = savdir->mkdir(source->GetName());
    std::cout<<"Source name is not outputfile name"<<std::endl;
    adir->cd();    
  }
  else{
    //adir = savdir->mkdir("input");
    adir->cd();    
  }

  //loop on all entries of this directory
  TKey *key;
  TIter nextkey(source->GetListOfKeys());
  while ((key = (TKey*)nextkey())) {
    const char *classname = key->GetClassName();
    TClass *cl = gROOT->GetClass(classname);
    if (!cl) continue;
    if (cl->InheritsFrom(TDirectory::Class())) {
      source->cd(key->GetName());
      TDirectory *subdir = gDirectory;
      adir->cd();
      CopyDir(subdir,parser);
      adir->cd();
    } else if (cl->InheritsFrom(TTree::Class())) {
      TTree *T = (TTree*)source->Get(key->GetName());
      adir->cd();
      TTree *newT = T->CloneTree(-1,"fast");
      newT->Write();
    } else {
      source->cd();
      TObject *obj = key->ReadObj();
      adir->cd();
      obj->Write();
      delete obj;
    }
  }
  adir->SaveSelf(kTRUE);
  savdir->cd();
}
void CopyFile(const char *fname, optutl::CommandLineParser parser) {
  //Copy all objects and subdirs of file fname as a subdir of the current directory
  TDirectory *target = gDirectory;
  TFile *f = TFile::Open(fname);
  if (!f || f->IsZombie()) {
    printf("Cannot copy file: %s\n",fname);
    target->cd();
    return;
  }
  target->cd();
  CopyDir(f,parser);
  delete f;
  target->cd();
}
void copyFiles( optutl::CommandLineParser parser, TFile* fOld, TFile* fNew) 
{
  //prepare files to be copied
  if(gSystem->AccessPathName(parser.stringValue("inputFile").c_str())) {
    gSystem->CopyFile("hsimple.root", parser.stringValue("inputFile").c_str());
  }

  fNew->cd();
  CopyFile(parser.stringValue("inputFile").c_str(),parser);
  fNew->ls();
  fNew->Close();

}

