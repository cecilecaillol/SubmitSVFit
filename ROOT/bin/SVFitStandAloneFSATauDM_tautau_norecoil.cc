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

#include "TauAnalysis/ClassicSVfit/interface/ClassicSVfit.h"
#include "TauAnalysis/ClassicSVfit/interface/MeasuredTauLepton.h"
#include "TauAnalysis/ClassicSVfit/interface/svFitHistogramAdapter.h"
#include "TauAnalysis/ClassicSVfit/interface/FastMTT.h"

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"

//If doES       0 does not apply any ES shifts
//              1 applies ES shifts to TT channel, no effect on other channels
//

void copyFiles( optutl::CommandLineParser parser, TFile* fOld, TFile* fNew) ;
void readdir(TDirectory *dir, optutl::CommandLineParser parser, char TreeToUse[], int doES, int doUES, int doRES, int doJES, int year) ;
void CopyFile(const char *fname, optutl::CommandLineParser parser);
void CopyDir(TDirectory *source,optutl::CommandLineParser parser);
void runSVFit(std::vector<classic_svFit::MeasuredTauLepton> & measuredTauLeptons, double measuredMETx, double measuredMETy, TMatrixD &covMET, float num, float &svFitMass, float& svFitPt, float &svFitEta, float &svFitPhi);

int main (int argc, char* argv[]) 
{
   optutl::CommandLineParser parser ("Sets Event Weights in the ntuple");
   parser.addOption("branch",optutl::CommandLineParser::kString,"Branch","__svFit__");
   parser.addOption("newFile",optutl::CommandLineParser::kString,"newFile","newFile.root");
   parser.addOption("inputFile",optutl::CommandLineParser::kString,"input File");
   parser.addOption("newOutputFile",optutl::CommandLineParser::kDouble,"New Output File",0.0);
   parser.addOption("doES",optutl::CommandLineParser::kDouble,"doES",0.0);
   parser.addOption("doUES",optutl::CommandLineParser::kDouble,"doUES",0.0);
   parser.addOption("doRES",optutl::CommandLineParser::kDouble,"doRES",0.0);
   parser.addOption("doJES",optutl::CommandLineParser::kDouble,"doJES",0.0);
   parser.addOption("year",optutl::CommandLineParser::kDouble,"year",0.0);

   parser.parseArguments (argc, argv);

   std::cout << "EXTRA COMMANDS:"
    << "\n --- doES: " << parser.doubleValue("doES") 
    << "\n --- doUES: " << parser.doubleValue("doUES")
    << "\n --- doRES: " << parser.doubleValue("doRES") 
    << "\n --- doJES: " << parser.doubleValue("doJES") << std::endl;

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
     readdir(fProduce,parser,TreeToUse,parser.doubleValue("doES"),parser.doubleValue("doUES"),parser.doubleValue("doRES"),parser.doubleValue("doJES"),parser.doubleValue("year"));

     fProduce->Close();
     f->Close();
   }
   else{
     TFile *f = new TFile(parser.stringValue("inputFile").c_str(),"UPDATE");
     readdir(f,parser,TreeToUse,parser.doubleValue("doES"),parser.doubleValue("doUES"),parser.doubleValue("doRES"),parser.doubleValue("doJES"),parser.doubleValue("year"));
     f->Close();
   }


} 


void readdir(TDirectory *dir, optutl::CommandLineParser parser, char TreeToUse[], int doES, int doUES, int doRES, int doJES, int year) 
{

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
      if (std::string(key->GetName()).find("_tree")){ readdir(subdir,parser,TreeToUse,parser.doubleValue("doES"),parser.doubleValue("doUES"),parser.doubleValue("doRES"),parser.doubleValue("doJES"),parser.doubleValue("year"));
      }

      dirsav->cd();
    }
    else if(k<1 && obj->IsA()->InheritsFrom(TTree::Class())) {
	k++;
	std::cout<<"ici!!!!"<<std::endl;

      TTree *t = (TTree*)obj;
      float svFitMass = -10;
      float svFitPt = -10;
      float svFitEta = -10;
      float svFitPhi = -10;
      float svFitMass_01UP = -10;
      float svFitPt_01UP = -10;
      float svFitEta_01UP = -10;
      float svFitPhi_01UP = -10;
      float svFitMass_01DOWN = -10;
      float svFitPt_01DOWN = -10;
      float svFitEta_01DOWN = -10;
      float svFitPhi_01DOWN = -10;
      float svFitMass_10UP = -10;
      float svFitPt_10UP = -10;
      float svFitEta_10UP = -10;
      float svFitPhi_10UP = -10;
      float svFitMass_10DOWN = -10;
      float svFitPt_10DOWN = -10;
      float svFitEta_10DOWN = -10;
      float svFitPhi_10DOWN = -10;
      float svFitMass_11UP = -10;
      float svFitPt_11UP = -10;
      float svFitEta_11UP = -10;
      float svFitPhi_11UP = -10;
      float svFitMass_11DOWN = -10;
      float svFitPt_11DOWN = -10;
      float svFitEta_11DOWN = -10;
      float svFitPhi_11DOWN = -10;
      float svFitMass_UESUp = -10;
      float svFitPt_UESUp = -10;
      float svFitEta_UESUp = -10;
      float svFitPhi_UESUp = -10;
      float svFitMass_UESDown = -10;
      float svFitPt_UESDown = -10;
      float svFitEta_UESDown = -10;
      float svFitPhi_UESDown = -10;

      float svFitMass_ResolutionUp = -10;
      float svFitPt_ResolutionUp = -10;
      float svFitEta_ResolutionUp = -10;
      float svFitPhi_ResolutionUp = -10;
      float svFitMass_ResolutionDown = -10;
      float svFitPt_ResolutionDown = -10;
      float svFitEta_ResolutionDown = -10;
      float svFitPhi_ResolutionDown = -10;
      float svFitMass_ResponseUp = -10;
      float svFitPt_ResponseUp = -10;
      float svFitEta_ResponseUp = -10;
      float svFitPhi_ResponseUp = -10;
      float svFitMass_ResponseDown = -10;
      float svFitPt_ResponseDown = -10;
      float svFitEta_ResponseDown = -10;
      float svFitPhi_ResponseDown = -10;

      float svFitMass_JetEta0to3Up = -10;
      float svFitPt_JetEta0to3Up = -10;
      float svFitEta_JetEta0to3Up = -10;
      float svFitPhi_JetEta0to3Up = -10;
      float svFitMass_JetEta0to3Down = -10;
      float svFitPt_JetEta0to3Down = -10;
      float svFitEta_JetEta0to3Down = -10;
      float svFitPhi_JetEta0to3Down = -10;
      float svFitMass_JetEC2Up = -10;
      float svFitPt_JetEC2Up = -10;
      float svFitEta_JetEC2Up = -10;
      float svFitPhi_JetEC2Up = -10;
      float svFitMass_JetEC2Down = -10;
      float svFitPt_JetEC2Down = -10;
      float svFitEta_JetEC2Down = -10;
      float svFitPhi_JetEC2Down = -10;
      float svFitMass_JetRelativeSampleUp = -10;
      float svFitPt_JetRelativeSampleUp = -10;
      float svFitEta_JetRelativeSampleUp = -10;
      float svFitPhi_JetRelativeSampleUp = -10;
      float svFitMass_JetRelativeSampleDown = -10;
      float svFitPt_JetRelativeSampleDown = -10;
      float svFitEta_JetRelativeSampleDown = -10;
      float svFitPhi_JetRelativeSampleDown = -10;
      float svFitMass_JetRelativeBalUp = -10;
      float svFitPt_JetRelativeBalUp = -10;
      float svFitEta_JetRelativeBalUp = -10;
      float svFitPhi_JetRelativeBalUp = -10;
      float svFitMass_JetRelativeBalDown = -10;
      float svFitPt_JetRelativeBalDown = -10;
      float svFitEta_JetRelativeBalDown = -10;
      float svFitPhi_JetRelativeBalDown = -10;
      float svFitMass_JetEta0to5Up = -10;
      float svFitPt_JetEta0to5Up = -10;
      float svFitEta_JetEta0to5Up = -10;
      float svFitPhi_JetEta0to5Up = -10;
      float svFitMass_JetEta0to5Down = -10;
      float svFitPt_JetEta0to5Down = -10;
      float svFitEta_JetEta0to5Down = -10;
      float svFitPhi_JetEta0to5Down = -10;
      float svFitMass_JetEta3to5Up = -10;
      float svFitPt_JetEta3to5Up = -10;
      float svFitEta_JetEta3to5Up = -10;
      float svFitPhi_JetEta3to5Up = -10;
      float svFitMass_JetEta3to5Down = -10;
      float svFitPt_JetEta3to5Down = -10;
      float svFitEta_JetEta3to5Down = -10;
      float svFitPhi_JetEta3to5Down = -10;

      TBranch *newBranch1 = t->Branch("m_sv", &svFitMass, "m_sv/F");
      TBranch *newBranch2 = t->Branch("pt_sv", &svFitPt, "pt_sv/F");
      TBranch *newBranch3 = t->Branch("phi_sv", &svFitPhi, "phi_sv/F");
      TBranch *newBranch4 = t->Branch("eta_sv", &svFitEta, "eta_sv/F");
      TBranch *newBranch1D01 = t->Branch("m_sv_01DOWN", &svFitMass_01DOWN, "m_sv_01DOWN/F");
      TBranch *newBranch2D01 = t->Branch("pt_sv_01DOWN", &svFitPt_01DOWN, "pt_sv_01DOWN/F");
      TBranch *newBranch3D01 = t->Branch("phi_sv_01DOWN", &svFitPhi_01DOWN, "phi_sv_01DOWN/F");
      TBranch *newBranch4D01 = t->Branch("eta_sv_01DOWN", &svFitEta_01DOWN, "eta_sv_01DOWN/F");
      TBranch *newBranch1U01 = t->Branch("m_sv_01UP", &svFitMass_01UP, "m_sv_01UP/F");
      TBranch *newBranch2U01 = t->Branch("pt_sv_01UP", &svFitPt_01UP, "pt_sv_01UP/F");
      TBranch *newBranch3U01 = t->Branch("phi_sv_01UP", &svFitPhi_01UP, "phi_sv_01UP/F");
      TBranch *newBranch4U01 = t->Branch("eta_sv_01UP", &svFitEta_01UP, "eta_sv_01UP/F");
      TBranch *newBranch1D11 = t->Branch("m_sv_11DOWN", &svFitMass_11DOWN, "m_sv_11DOWN/F");
      TBranch *newBranch2D11 = t->Branch("pt_sv_11DOWN", &svFitPt_11DOWN, "pt_sv_11DOWN/F");
      TBranch *newBranch3D11 = t->Branch("phi_sv_11DOWN", &svFitPhi_11DOWN, "phi_sv_11DOWN/F");
      TBranch *newBranch4D11 = t->Branch("eta_sv_11DOWN", &svFitEta_11DOWN, "eta_sv_11DOWN/F");
      TBranch *newBranch1U11 = t->Branch("m_sv_11UP", &svFitMass_11UP, "m_sv_11UP/F");
      TBranch *newBranch2U11 = t->Branch("pt_sv_11UP", &svFitPt_11UP, "pt_sv_11UP/F");
      TBranch *newBranch3U11 = t->Branch("phi_sv_11UP", &svFitPhi_11UP, "phi_sv_11UP/F");
      TBranch *newBranch4U11 = t->Branch("eta_sv_11UP", &svFitEta_11UP, "eta_sv_11UP/F");
      TBranch *newBranch1D10 = t->Branch("m_sv_10DOWN", &svFitMass_10DOWN, "m_sv_10DOWN/F");
      TBranch *newBranch2D10 = t->Branch("pt_sv_10DOWN", &svFitPt_10DOWN, "pt_sv_10DOWN/F");
      TBranch *newBranch3D10 = t->Branch("phi_sv_10DOWN", &svFitPhi_10DOWN, "phi_sv_10DOWN/F");
      TBranch *newBranch4D10 = t->Branch("eta_sv_10DOWN", &svFitEta_10DOWN, "eta_sv_10DOWN/F");
      TBranch *newBranch1U10 = t->Branch("m_sv_10UP", &svFitMass_10UP, "m_sv_10UP/F");
      TBranch *newBranch2U10 = t->Branch("pt_sv_10UP", &svFitPt_10UP, "pt_sv_10UP/F");
      TBranch *newBranch3U10 = t->Branch("phi_sv_10UP", &svFitPhi_10UP, "phi_sv_10UP/F");
      TBranch *newBranch4U10 = t->Branch("eta_sv_10UP", &svFitEta_10UP, "eta_sv_10UP/F");
      TBranch *newBranch1UU = t->Branch("m_sv_UESUp", &svFitMass_UESUp, "m_sv_UESUp/F");
      TBranch *newBranch2UU = t->Branch("pt_sv_UESUp", &svFitPt_UESUp, "pt_sv_UESUp/F");
      TBranch *newBranch3UU = t->Branch("phi_sv_UESUp", &svFitPhi_UESUp, "phi_sv_UESUp/F");
      TBranch *newBranch4UU = t->Branch("eta_sv_UESUp", &svFitEta_UESUp, "eta_sv_UESUp/F");
      TBranch *newBranch1UD = t->Branch("m_sv_UESDown", &svFitMass_UESDown, "m_sv_UESDown/F");
      TBranch *newBranch2UD = t->Branch("pt_sv_UESDown", &svFitPt_UESDown, "pt_sv_UESDown/F");
      TBranch *newBranch3UD = t->Branch("phi_sv_UESDown", &svFitPhi_UESDown, "phi_sv_UESDown/F");
      TBranch *newBranch4UD = t->Branch("eta_sv_UESDown", &svFitEta_UESDown, "eta_sv_UESDown/F");
      TBranch *newBranch1ResponseU = t->Branch("m_sv_ResponseUp", &svFitMass_ResponseUp, "m_sv_ResponseUp/F");
      TBranch *newBranch1ResponseD = t->Branch("m_sv_ResponseDown", &svFitMass_ResponseDown, "m_sv_ResponseDown/F");
      TBranch *newBranch1ResolutionU = t->Branch("m_sv_ResolutionUp", &svFitMass_ResolutionUp, "m_sv_ResolutionUp/F");
      TBranch *newBranch1ResolutionD = t->Branch("m_sv_ResolutionDown", &svFitMass_ResolutionDown, "m_sv_ResolutionDown/F");
      TBranch *newBranch1JetEta0to3U = t->Branch("m_sv_JetEta0to3Up", &svFitMass_JetEta0to3Up, "m_sv_JetEta0to3Up/F");
      TBranch *newBranch1JetEta0to3D = t->Branch("m_sv_JetEta0to3Down", &svFitMass_JetEta0to3Down, "m_sv_JetEta0to3Down/F");
      TBranch *newBranch1JetEC2U = t->Branch("m_sv_JetEC2Up", &svFitMass_JetEC2Up, "m_sv_JetEC2Up/F");
      TBranch *newBranch1JetEC2D = t->Branch("m_sv_JetEC2Down", &svFitMass_JetEC2Down, "m_sv_JetEC2Down/F");
      TBranch *newBranch1JetEta3to5U = t->Branch("m_sv_JetEta3to5Up", &svFitMass_JetEta3to5Up, "m_sv_JetEta3to5Up/F");
      TBranch *newBranch1JetEta3to5D = t->Branch("m_sv_JetEta3to5Down", &svFitMass_JetEta3to5Down, "m_sv_JetEta3to5Down/F");
      TBranch *newBranch1JetEta0to5U = t->Branch("m_sv_JetEta0to5Up", &svFitMass_JetEta0to5Up, "m_sv_JetEta0to5Up/F");
      TBranch *newBranch1JetEta0to5D = t->Branch("m_sv_JetEta0to5Down", &svFitMass_JetEta0to5Down, "m_sv_JetEta0to5Down/F");
      TBranch *newBranch1JetRelativeBalU = t->Branch("m_sv_JetRelativeBalUp", &svFitMass_JetRelativeBalUp, "m_sv_JetRelativeBalUp/F");
      TBranch *newBranch1JetRelativeBalD = t->Branch("m_sv_JetRelativeBalDown", &svFitMass_JetRelativeBalDown, "m_sv_JetRelativeBalDown/F");
      TBranch *newBranch1JetRelativeSampleU = t->Branch("m_sv_JetRelativeSampleUp", &svFitMass_JetRelativeSampleUp, "m_sv_JetRelativeSampleUp/F");
      TBranch *newBranch1JetRelativeSampleD = t->Branch("m_sv_JetRelativeSampleDown", &svFitMass_JetRelativeSampleDown, "m_sv_JetRelativeSampleDown/F");

      int evt;
      int run, lumi;
      float pt1;
      float eta1;
      float phi1;
      float pt2;
      float eta2;
      float phi2;
      float m2;
      int gen_match_2;
      float decayMode2;
      float m1;
      int gen_match_1;
      float decayMode1;
      float pfCovMatrix00;
      float pfCovMatrix10;
      float pfCovMatrix01;
      float pfCovMatrix11;

      // define MET
      double measuredMETx = 0.;
      double measuredMETy = 0.;
      double measuredMETx_UESDown = 0.;
      double measuredMETy_UESDown = 0.;
      double measuredMETx_UESUp = 0.;
      double measuredMETy_UESUp = 0.;
      double measuredMETx_ResolutionDown = 0.;
      double measuredMETy_ResolutionDown = 0.;
      double measuredMETx_ResolutionUp = 0.;
      double measuredMETy_ResolutionUp = 0.;
      double measuredMETx_ResponseDown = 0.;
      double measuredMETy_ResponseDown = 0.;
      double measuredMETx_ResponseUp = 0.;
      double measuredMETy_ResponseUp = 0.;
      double measuredMETx_JetEta0to3Down = 0.;
      double measuredMETy_JetEta0to3Down = 0.;
      double measuredMETx_JetEta0to3Up = 0.;
      double measuredMETy_JetEta0to3Up = 0.;
      double measuredMETx_JetEC2Down = 0.;
      double measuredMETy_JetEC2Down = 0.;
      double measuredMETx_JetEC2Up = 0.;
      double measuredMETy_JetEC2Up = 0.;
      double measuredMETx_JetRelativeSampleDown = 0.;
      double measuredMETy_JetRelativeSampleDown = 0.;
      double measuredMETx_JetRelativeSampleUp = 0.;
      double measuredMETy_JetRelativeSampleUp = 0.;
      double measuredMETx_JetRelativeBalDown = 0.;
      double measuredMETy_JetRelativeBalDown = 0.;
      double measuredMETx_JetRelativeBalUp = 0.;
      double measuredMETy_JetRelativeBalUp = 0.;
      double measuredMETx_JetEta0to5Down = 0.;
      double measuredMETy_JetEta0to5Down = 0.;
      double measuredMETx_JetEta0to5Up = 0.;
      double measuredMETy_JetEta0to5Up = 0.;
      double measuredMETx_JetEta3to5Down = 0.;
      double measuredMETy_JetEta3to5Down = 0.;
      double measuredMETx_JetEta3to5Up = 0.;
      double measuredMETy_JetEta3to5Up = 0.;
      float pfmet;
      float pfmetphi;
      float pfmet_UESDown;
      float pfmetphi_UESDown;
      float pfmet_UESUp;
      float pfmetphi_UESUp;
      float pfmet_ResponseDown;
      float pfmetphi_ResponseDown;
      float pfmet_ResponseUp;
      float pfmetphi_ResponseUp;
      float pfmet_ResolutionDown;
      float pfmetphi_ResolutionDown;
      float pfmet_ResolutionUp;
      float pfmetphi_ResolutionUp;
      float pfmet_JetEta0to3Up;
      float pfmetphi_JetEta0to3Up;
      float pfmet_JetEta0to3Down;
      float pfmetphi_JetEta0to3Down;
      float pfmet_JetEC2Up;
      float pfmetphi_JetEC2Up;
      float pfmet_JetEC2Down;
      float pfmetphi_JetEC2Down;
      float pfmet_JetRelativeSampleUp;
      float pfmetphi_JetRelativeSampleUp;
      float pfmet_JetRelativeSampleDown;
      float pfmetphi_JetRelativeSampleDown;
      float pfmet_JetRelativeBalUp;
      float pfmetphi_JetRelativeBalUp;
      float pfmet_JetRelativeBalDown;
      float pfmetphi_JetRelativeBalDown;
      float pfmet_JetEta3to5Up;
      float pfmetphi_JetEta3to5Up;
      float pfmet_JetEta3to5Down;
      float pfmetphi_JetEta3to5Down;
      float pfmet_JetEta0to5Up;
      float pfmetphi_JetEta0to5Up;
      float pfmet_JetEta0to5Down;
      float pfmetphi_JetEta0to5Down;
      TLorentzVector TMet(0,0,0,0);
      TLorentzVector TMet_UESDown(0,0,0,0);
      TLorentzVector TMet_UESUp(0,0,0,0);
      TLorentzVector TMet_ResolutionDown(0,0,0,0);
      TLorentzVector TMet_ResolutionUp(0,0,0,0);
      TLorentzVector TMet_ResponseDown(0,0,0,0);
      TLorentzVector TMet_ResponseUp(0,0,0,0);
      TLorentzVector TMet_JetEta0to3Up(0,0,0,0);
      TLorentzVector TMet_JetEta0to3Down(0,0,0,0);
      TLorentzVector TMet_JetEC2Up(0,0,0,0);
      TLorentzVector TMet_JetEC2Down(0,0,0,0);
      TLorentzVector TMet_JetEta3to5Up(0,0,0,0);
      TLorentzVector TMet_JetEta3to5Down(0,0,0,0);
      TLorentzVector TMet_JetEta0to5Up(0,0,0,0);
      TLorentzVector TMet_JetEta0to5Down(0,0,0,0);
      TLorentzVector TMet_JetRelativeBalUp(0,0,0,0);
      TLorentzVector TMet_JetRelativeBalDown(0,0,0,0);
      TLorentzVector TMet_JetRelativeSampleUp(0,0,0,0);
      TLorentzVector TMet_JetRelativeSampleDown(0,0,0,0);
      TMatrixD covMET(2, 2);
      TBranch *pt1branch;

      t->SetBranchAddress("evt",&evt);
      t->SetBranchAddress("run",&run);
      t->SetBranchAddress("lumi",&lumi);
      t->SetBranchAddress("pt_1",&pt1,&pt1branch);
      t->SetBranchAddress("eta_1",&eta1);
      t->SetBranchAddress("phi_1",&phi1);
      t->SetBranchAddress("pt_2",&pt2);
      t->SetBranchAddress("eta_2",&eta2);
      t->SetBranchAddress("phi_2",&phi2);
      t->SetBranchAddress("m_2",&m2);
      t->SetBranchAddress("gen_match_2",&gen_match_2);
      t->SetBranchAddress("l2_decayMode",&decayMode2);
      t->SetBranchAddress("m_1",&m1);
      t->SetBranchAddress("gen_match_1",&gen_match_1);
      t->SetBranchAddress("l1_decayMode",&decayMode1);
      t->SetBranchAddress("met",&pfmet);
      t->SetBranchAddress("metphi",&pfmetphi);
      t->SetBranchAddress("met_UESUp",&pfmet_UESUp);
      t->SetBranchAddress("metphi_UESUp",&pfmetphi_UESUp);
      t->SetBranchAddress("met_UESDown",&pfmet_UESDown);
      t->SetBranchAddress("metphi_UESDown",&pfmetphi_UESDown);
      t->SetBranchAddress("met_responseUp",&pfmet_ResponseUp);
      t->SetBranchAddress("metphi_responseUp",&pfmetphi_ResponseUp);
      t->SetBranchAddress("met_responseDown",&pfmet_ResponseDown);
      t->SetBranchAddress("metphi_responseDown",&pfmetphi_ResponseDown);
      t->SetBranchAddress("met_resolutionUp",&pfmet_ResolutionUp);
      t->SetBranchAddress("metphi_resolutionUp",&pfmetphi_ResolutionUp);
      t->SetBranchAddress("met_resolutionDown",&pfmet_ResolutionDown);
      t->SetBranchAddress("metphi_resolutionDown",&pfmetphi_ResolutionDown);
      t->SetBranchAddress("met_JetEta0to3Up",&pfmet_JetEta0to3Up);
      t->SetBranchAddress("met_JetEta0to3Down",&pfmet_JetEta0to3Down);
      t->SetBranchAddress("metphi_JetEta0to3Up",&pfmetphi_JetEta0to3Up);
      t->SetBranchAddress("metphi_JetEta0to3Down",&pfmetphi_JetEta0to3Down);
      t->SetBranchAddress("met_JetEC2Up",&pfmet_JetEC2Up);
      t->SetBranchAddress("met_JetEC2Down",&pfmet_JetEC2Down);
      t->SetBranchAddress("metphi_JetEC2Up",&pfmetphi_JetEC2Up);
      t->SetBranchAddress("metphi_JetEC2Down",&pfmetphi_JetEC2Down);
      t->SetBranchAddress("met_JetRelativeSampleUp",&pfmet_JetRelativeSampleUp);
      t->SetBranchAddress("met_JetRelativeSampleDown",&pfmet_JetRelativeSampleDown);
      t->SetBranchAddress("metphi_JetRelativeSampleUp",&pfmetphi_JetRelativeSampleUp);
      t->SetBranchAddress("metphi_JetRelativeSampleDown",&pfmetphi_JetRelativeSampleDown);
      t->SetBranchAddress("met_JetRelativeBalUp",&pfmet_JetRelativeBalUp);
      t->SetBranchAddress("met_JetRelativeBalDown",&pfmet_JetRelativeBalDown);
      t->SetBranchAddress("metphi_JetRelativeBalUp",&pfmetphi_JetRelativeBalUp);
      t->SetBranchAddress("metphi_JetRelativeBalDown",&pfmetphi_JetRelativeBalDown);
      t->SetBranchAddress("met_JetEta0to5Up",&pfmet_JetEta0to5Up);
      t->SetBranchAddress("met_JetEta0to5Down",&pfmet_JetEta0to5Down);
      t->SetBranchAddress("metphi_JetEta0to5Up",&pfmetphi_JetEta0to5Up);
      t->SetBranchAddress("metphi_JetEta0to5Down",&pfmetphi_JetEta0to5Down);
      t->SetBranchAddress("met_JetEta3to5Up",&pfmet_JetEta3to5Up);
      t->SetBranchAddress("met_JetEta3to5Down",&pfmet_JetEta3to5Down);
      t->SetBranchAddress("metphi_JetEta3to5Up",&pfmetphi_JetEta3to5Up);
      t->SetBranchAddress("metphi_JetEta3to5Down",&pfmetphi_JetEta3to5Down);
      // FOR PF MET ANALYSIS
      t->SetBranchAddress("metcov00",&pfCovMatrix00);
      t->SetBranchAddress("metcov01",&pfCovMatrix01);
      t->SetBranchAddress("metcov10",&pfCovMatrix10);
      t->SetBranchAddress("metcov11",&pfCovMatrix11);

      printf("Found tree -> weighting\n");

      /*int verbosity = 1;
      ClassicSVfit svFitAlgo(verbosity);
      svFitAlgo.addLogM_fixed(true, 6.);
      svFitAlgo.setLikelihoodFileName("testClassicSVfit.root");*/

      for(Int_t i=0;i<t->GetEntries();++i){
         t->GetEntry(i);

         TMet.SetPtEtaPhiM(pfmet,0,pfmetphi,0);
         measuredMETx = pfmet*TMath::Cos(pfmetphi);
         measuredMETy = pfmet*TMath::Sin(pfmetphi);

         TMet_UESUp.SetPtEtaPhiM(pfmet_UESUp,0,pfmetphi_UESUp,0);
         measuredMETx_UESUp = pfmet_UESUp*TMath::Cos(pfmetphi_UESUp);
         measuredMETy_UESUp = pfmet_UESUp*TMath::Sin(pfmetphi_UESUp);
         TMet_UESDown.SetPtEtaPhiM(pfmet_UESDown,0,pfmetphi_UESDown,0);
         measuredMETx_UESDown = pfmet_UESDown*TMath::Cos(pfmetphi_UESDown);
         measuredMETy_UESDown = pfmet_UESDown*TMath::Sin(pfmetphi_UESDown);

         TMet_ResolutionUp.SetPtEtaPhiM(pfmet_ResolutionUp,0,pfmetphi_ResolutionUp,0);
         measuredMETx_ResolutionUp = pfmet_ResolutionUp*TMath::Cos(pfmetphi_ResolutionUp);
         measuredMETy_ResolutionUp = pfmet_ResolutionUp*TMath::Sin(pfmetphi_ResolutionUp);
         TMet_ResolutionDown.SetPtEtaPhiM(pfmet_ResolutionDown,0,pfmetphi_ResolutionDown,0);
         measuredMETx_ResolutionDown = pfmet_ResolutionDown*TMath::Cos(pfmetphi_ResolutionDown);
         measuredMETy_ResolutionDown = pfmet_ResolutionDown*TMath::Sin(pfmetphi_ResolutionDown);
         TMet_ResponseUp.SetPtEtaPhiM(pfmet_ResponseUp,0,pfmetphi_ResponseUp,0);
         measuredMETx_ResponseUp = pfmet_ResponseUp*TMath::Cos(pfmetphi_ResponseUp);
         measuredMETy_ResponseUp = pfmet_ResponseUp*TMath::Sin(pfmetphi_ResponseUp);
         TMet_ResponseDown.SetPtEtaPhiM(pfmet_ResponseDown,0,pfmetphi_ResponseDown,0);
         measuredMETx_ResponseDown = pfmet_ResponseDown*TMath::Cos(pfmetphi_ResponseDown);
         measuredMETy_ResponseDown = pfmet_ResponseDown*TMath::Sin(pfmetphi_ResponseDown);

         TMet_JetEta0to3Up.SetPtEtaPhiM(pfmet_JetEta0to3Up,0,pfmetphi_JetEta0to3Up,0);
         measuredMETx_JetEta0to3Up = pfmet_JetEta0to3Up*TMath::Cos(pfmetphi_JetEta0to3Up);
         measuredMETy_JetEta0to3Up = pfmet_JetEta0to3Up*TMath::Sin(pfmetphi_JetEta0to3Up);
         TMet_JetEta0to3Down.SetPtEtaPhiM(pfmet_JetEta0to3Down,0,pfmetphi_JetEta0to3Down,0);
         measuredMETx_JetEta0to3Down = pfmet_JetEta0to3Down*TMath::Cos(pfmetphi_JetEta0to3Down);
         measuredMETy_JetEta0to3Down = pfmet_JetEta0to3Down*TMath::Sin(pfmetphi_JetEta0to3Down);
         TMet_JetEC2Up.SetPtEtaPhiM(pfmet_JetEC2Up,0,pfmetphi_JetEC2Up,0);
         measuredMETx_JetEC2Up = pfmet_JetEC2Up*TMath::Cos(pfmetphi_JetEC2Up);
         measuredMETy_JetEC2Up = pfmet_JetEC2Up*TMath::Sin(pfmetphi_JetEC2Up);
         TMet_JetEC2Down.SetPtEtaPhiM(pfmet_JetEC2Down,0,pfmetphi_JetEC2Down,0);
         measuredMETx_JetEC2Down = pfmet_JetEC2Down*TMath::Cos(pfmetphi_JetEC2Down);
         measuredMETy_JetEC2Down = pfmet_JetEC2Down*TMath::Sin(pfmetphi_JetEC2Down);
         TMet_JetEta3to5Up.SetPtEtaPhiM(pfmet_JetEta3to5Up,0,pfmetphi_JetEta3to5Up,0);
         measuredMETx_JetEta3to5Up = pfmet_JetEta3to5Up*TMath::Cos(pfmetphi_JetEta3to5Up);
         measuredMETy_JetEta3to5Up = pfmet_JetEta3to5Up*TMath::Sin(pfmetphi_JetEta3to5Up);
         TMet_JetEta3to5Down.SetPtEtaPhiM(pfmet_JetEta3to5Down,0,pfmetphi_JetEta3to5Down,0);
         measuredMETx_JetEta3to5Down = pfmet_JetEta3to5Down*TMath::Cos(pfmetphi_JetEta3to5Down);
         measuredMETy_JetEta3to5Down = pfmet_JetEta3to5Down*TMath::Sin(pfmetphi_JetEta3to5Down);
         TMet_JetEta0to5Up.SetPtEtaPhiM(pfmet_JetEta0to5Up,0,pfmetphi_JetEta0to5Up,0);
         measuredMETx_JetEta0to5Up = pfmet_JetEta0to5Up*TMath::Cos(pfmetphi_JetEta0to5Up);
         measuredMETy_JetEta0to5Up = pfmet_JetEta0to5Up*TMath::Sin(pfmetphi_JetEta0to5Up);
         TMet_JetEta0to5Down.SetPtEtaPhiM(pfmet_JetEta0to5Down,0,pfmetphi_JetEta0to5Down,0);
         measuredMETx_JetEta0to5Down = pfmet_JetEta0to5Down*TMath::Cos(pfmetphi_JetEta0to5Down);
         measuredMETy_JetEta0to5Down = pfmet_JetEta0to5Down*TMath::Sin(pfmetphi_JetEta0to5Down);
         TMet_JetRelativeBalUp.SetPtEtaPhiM(pfmet_JetRelativeBalUp,0,pfmetphi_JetRelativeBalUp,0);
         measuredMETx_JetRelativeBalUp = pfmet_JetRelativeBalUp*TMath::Cos(pfmetphi_JetRelativeBalUp);
         measuredMETy_JetRelativeBalUp = pfmet_JetRelativeBalUp*TMath::Sin(pfmetphi_JetRelativeBalUp);
         TMet_JetRelativeBalDown.SetPtEtaPhiM(pfmet_JetRelativeBalDown,0,pfmetphi_JetRelativeBalDown,0);
         measuredMETx_JetRelativeBalDown = pfmet_JetRelativeBalDown*TMath::Cos(pfmetphi_JetRelativeBalDown);
         measuredMETy_JetRelativeBalDown = pfmet_JetRelativeBalDown*TMath::Sin(pfmetphi_JetRelativeBalDown);
         TMet_JetRelativeSampleUp.SetPtEtaPhiM(pfmet_JetRelativeSampleUp,0,pfmetphi_JetRelativeSampleUp,0);
         measuredMETx_JetRelativeSampleUp = pfmet_JetRelativeSampleUp*TMath::Cos(pfmetphi_JetRelativeSampleUp);
         measuredMETy_JetRelativeSampleUp = pfmet_JetRelativeSampleUp*TMath::Sin(pfmetphi_JetRelativeSampleUp);
         TMet_JetRelativeSampleDown.SetPtEtaPhiM(pfmet_JetRelativeSampleDown,0,pfmetphi_JetRelativeSampleDown,0);
         measuredMETx_JetRelativeSampleDown = pfmet_JetRelativeSampleDown*TMath::Cos(pfmetphi_JetRelativeSampleDown);
         measuredMETy_JetRelativeSampleDown = pfmet_JetRelativeSampleDown*TMath::Sin(pfmetphi_JetRelativeSampleDown);

         covMET[0][0] =  pfCovMatrix00;
         covMET[1][0] =  pfCovMatrix10;
         covMET[0][1] =  pfCovMatrix01;
         covMET[1][1] =  pfCovMatrix11;

	 std::vector<classic_svFit::MeasuredTauLepton> measuredTauLeptons;
	 measuredTauLeptons.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay, pt1, eta1,  phi1, m1, decayMode1)); 
         measuredTauLeptons.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay,  pt2, eta2, phi2, m2, decayMode2));
         runSVFit(measuredTauLeptons, measuredMETx, measuredMETy, covMET, 0, svFitMass, svFitPt, svFitEta, svFitPhi);

	 if (doUES){
	     runSVFit(measuredTauLeptons, measuredMETx_UESUp, measuredMETy_UESUp, covMET, 0, svFitMass_UESUp, svFitPt_UESUp, svFitEta_UESUp, svFitPhi_UESUp);
             runSVFit(measuredTauLeptons, measuredMETx_UESDown, measuredMETy_UESDown, covMET, 0, svFitMass_UESDown, svFitPt_UESDown, svFitEta_UESDown, svFitPhi_UESDown);
	 }
	 else{
	    svFitMass_UESUp=svFitMass;
            svFitEta_UESUp=svFitEta;
            svFitPt_UESUp=svFitPt;
            svFitPhi_UESUp=svFitPhi;
            svFitMass_UESDown=svFitMass;
            svFitEta_UESDown=svFitEta;
            svFitPt_UESDown=svFitPt;
            svFitPhi_UESDown=svFitPhi;
	 }

         if (doJES){
             runSVFit(measuredTauLeptons, measuredMETx_JetEta0to3Up, measuredMETy_JetEta0to3Up, covMET, 0, svFitMass_JetEta0to3Up, svFitPt_JetEta0to3Up, svFitEta_JetEta0to3Up, svFitPhi_JetEta0to3Up);
             runSVFit(measuredTauLeptons, measuredMETx_JetEta0to3Down, measuredMETy_JetEta0to3Down, covMET, 0, svFitMass_JetEta0to3Down, svFitPt_JetEta0to3Down, svFitEta_JetEta0to3Down, svFitPhi_JetEta0to3Down);
             runSVFit(measuredTauLeptons, measuredMETx_JetEC2Up, measuredMETy_JetEC2Up, covMET, 0, svFitMass_JetEC2Up, svFitPt_JetEC2Up, svFitEta_JetEC2Up, svFitPhi_JetEC2Up);
             runSVFit(measuredTauLeptons, measuredMETx_JetEC2Down, measuredMETy_JetEC2Down, covMET, 0, svFitMass_JetEC2Down, svFitPt_JetEC2Down, svFitEta_JetEC2Down, svFitPhi_JetEC2Down);
             runSVFit(measuredTauLeptons, measuredMETx_JetEta3to5Up, measuredMETy_JetEta3to5Up, covMET, 0, svFitMass_JetEta3to5Up, svFitPt_JetEta3to5Up, svFitEta_JetEta3to5Up, svFitPhi_JetEta3to5Up);
             runSVFit(measuredTauLeptons, measuredMETx_JetEta3to5Down, measuredMETy_JetEta3to5Down, covMET, 0, svFitMass_JetEta3to5Down, svFitPt_JetEta3to5Down, svFitEta_JetEta3to5Down, svFitPhi_JetEta3to5Down);
             runSVFit(measuredTauLeptons, measuredMETx_JetEta0to5Up, measuredMETy_JetEta0to5Up, covMET, 0, svFitMass_JetEta0to5Up, svFitPt_JetEta0to5Up, svFitEta_JetEta0to5Up, svFitPhi_JetEta0to5Up);
             runSVFit(measuredTauLeptons, measuredMETx_JetEta0to5Down, measuredMETy_JetEta0to5Down, covMET, 0, svFitMass_JetEta0to5Down, svFitPt_JetEta0to5Down, svFitEta_JetEta0to5Down, svFitPhi_JetEta0to5Down);
             runSVFit(measuredTauLeptons, measuredMETx_JetRelativeBalUp, measuredMETy_JetRelativeBalUp, covMET, 0, svFitMass_JetRelativeBalUp, svFitPt_JetRelativeBalUp, svFitEta_JetRelativeBalUp, svFitPhi_JetRelativeBalUp);
             runSVFit(measuredTauLeptons, measuredMETx_JetRelativeBalDown, measuredMETy_JetRelativeBalDown, covMET, 0, svFitMass_JetRelativeBalDown, svFitPt_JetRelativeBalDown, svFitEta_JetRelativeBalDown, svFitPhi_JetRelativeBalDown);
	     if (year>2016){
                runSVFit(measuredTauLeptons, measuredMETx_JetRelativeSampleUp, measuredMETy_JetRelativeSampleUp, covMET, 0, svFitMass_JetRelativeSampleUp, svFitPt_JetRelativeSampleUp, svFitEta_JetRelativeSampleUp, svFitPhi_JetRelativeSampleUp);
                runSVFit(measuredTauLeptons, measuredMETx_JetRelativeSampleDown, measuredMETy_JetRelativeSampleDown, covMET, 0, svFitMass_JetRelativeSampleDown, svFitPt_JetRelativeSampleDown, svFitEta_JetRelativeSampleDown, svFitPhi_JetRelativeSampleDown);
	    }
	    else {
              svFitMass_JetRelativeSampleUp=svFitMass;
              svFitEta_JetRelativeSampleUp=svFitEta;
              svFitPt_JetRelativeSampleUp=svFitPt;
              svFitPhi_JetRelativeSampleUp=svFitPhi;
              svFitMass_JetRelativeSampleDown=svFitMass;
              svFitEta_JetRelativeSampleDown=svFitEta;
              svFitPt_JetRelativeSampleDown=svFitPt;
              svFitPhi_JetRelativeSampleDown=svFitPhi;
	    }
         }
         else{
            svFitMass_JetEta0to3Up=svFitMass;
            svFitEta_JetEta0to3Up=svFitEta;
            svFitPt_JetEta0to3Up=svFitPt;
            svFitPhi_JetEta0to3Up=svFitPhi;
            svFitMass_JetEta0to3Down=svFitMass;
            svFitEta_JetEta0to3Down=svFitEta;
            svFitPt_JetEta0to3Down=svFitPt;
            svFitPhi_JetEta0to3Down=svFitPhi;
            svFitMass_JetEC2Up=svFitMass;
            svFitEta_JetEC2Up=svFitEta;
            svFitPt_JetEC2Up=svFitPt;
            svFitPhi_JetEC2Up=svFitPhi;
            svFitMass_JetEC2Down=svFitMass;
            svFitEta_JetEC2Down=svFitEta;
            svFitPt_JetEC2Down=svFitPt;
            svFitPhi_JetEC2Down=svFitPhi;
            svFitMass_JetRelativeSampleUp=svFitMass;
            svFitEta_JetRelativeSampleUp=svFitEta;
            svFitPt_JetRelativeSampleUp=svFitPt;
            svFitPhi_JetRelativeSampleUp=svFitPhi;
            svFitMass_JetRelativeSampleDown=svFitMass;
            svFitEta_JetRelativeSampleDown=svFitEta;
            svFitPt_JetRelativeSampleDown=svFitPt;
            svFitPhi_JetRelativeSampleDown=svFitPhi;
            svFitMass_JetRelativeBalUp=svFitMass;
            svFitEta_JetRelativeBalUp=svFitEta;
            svFitPt_JetRelativeBalUp=svFitPt;
            svFitPhi_JetRelativeBalUp=svFitPhi;
            svFitMass_JetRelativeBalDown=svFitMass;
            svFitEta_JetRelativeBalDown=svFitEta;
            svFitPt_JetRelativeBalDown=svFitPt;
            svFitPhi_JetRelativeBalDown=svFitPhi;
            svFitMass_JetEta0to5Up=svFitMass;
            svFitEta_JetEta0to5Up=svFitEta;
            svFitPt_JetEta0to5Up=svFitPt;
            svFitPhi_JetEta0to5Up=svFitPhi;
            svFitMass_JetEta0to5Down=svFitMass;
            svFitEta_JetEta0to5Down=svFitEta;
            svFitPt_JetEta0to5Down=svFitPt;
            svFitPhi_JetEta0to5Down=svFitPhi;
            svFitMass_JetEta3to5Up=svFitMass;
            svFitEta_JetEta3to5Up=svFitEta;
            svFitPt_JetEta3to5Up=svFitPt;
            svFitPhi_JetEta3to5Up=svFitPhi;
            svFitMass_JetEta3to5Down=svFitMass;
            svFitEta_JetEta3to5Down=svFitEta;
            svFitPt_JetEta3to5Down=svFitPt;
            svFitPhi_JetEta3to5Down=svFitPhi;
         }

	 // Recoil uncertainties
         if (doRES){
             runSVFit(measuredTauLeptons, measuredMETx_ResolutionUp, measuredMETy_ResolutionUp, covMET, 0, svFitMass_ResolutionUp, svFitPt_ResolutionUp, svFitEta_ResolutionUp, svFitPhi_ResolutionUp);
             runSVFit(measuredTauLeptons, measuredMETx_ResolutionDown, measuredMETy_ResolutionDown, covMET, 0, svFitMass_ResolutionDown, svFitPt_ResolutionDown, svFitEta_ResolutionDown, svFitPhi_ResolutionDown);
             runSVFit(measuredTauLeptons, measuredMETx_ResponseUp, measuredMETy_ResponseUp, covMET, 0, svFitMass_ResponseUp, svFitPt_ResponseUp, svFitEta_ResponseUp, svFitPhi_ResponseUp);
             runSVFit(measuredTauLeptons, measuredMETx_ResponseDown, measuredMETy_ResponseDown, covMET, 0, svFitMass_ResponseDown, svFitPt_ResponseDown, svFitEta_ResponseDown, svFitPhi_ResponseDown);
         }
         else{
            svFitMass_ResponseUp=svFitMass;
            svFitEta_ResponseUp=svFitEta;
            svFitPt_ResponseUp=svFitPt;
            svFitPhi_ResponseUp=svFitPhi;
            svFitMass_ResponseDown=svFitMass;
            svFitEta_ResponseDown=svFitEta;
            svFitPt_ResponseDown=svFitPt;
            svFitPhi_ResponseDown=svFitPhi;
            svFitMass_ResolutionUp=svFitMass;
            svFitEta_ResolutionUp=svFitEta;
            svFitPt_ResolutionUp=svFitPt;
            svFitPhi_ResolutionUp=svFitPhi;
            svFitMass_ResolutionDown=svFitMass;
            svFitEta_ResolutionDown=svFitEta;
            svFitPt_ResolutionDown=svFitPt;
            svFitPhi_ResolutionDown=svFitPhi;
         }

	 if (doES){
                float ES_UP_scale=1.008; 
                if (year==2016){
                    if (gen_match_2==5 && decayMode2==0) ES_UP_scale=1.010; // for real taus
                    if (gen_match_2==5 && decayMode2==0) ES_UP_scale=1.009; // for real taus
                    if (gen_match_2==5 && decayMode2==0) ES_UP_scale=1.011; // for real taus
                }
                else if (year==2017){
                    if (gen_match_2==5 && decayMode2==0) ES_UP_scale=1.008; // for real taus
                    if (gen_match_2==5 && decayMode2==0) ES_UP_scale=1.008; // for real taus
                    if (gen_match_2==5 && decayMode2==0) ES_UP_scale=1.009; // for real taus
                }
                else if (year==2018){
                    if (gen_match_2==5 && decayMode2==0) ES_UP_scale=1.011; // for real taus
                    if (gen_match_2==5 && decayMode2==0) ES_UP_scale=1.008; // for real taus
                    if (gen_match_2==5 && decayMode2==0) ES_UP_scale=1.009; // for real taus
                }
                double pt2_UP;
                double mass2_UP=m2;
                if (decayMode2!=0) mass2_UP = m2 * ES_UP_scale;
                pt2_UP = pt2 * ES_UP_scale;
                double dx2_UP, dy2_UP;
                dx2_UP = pt2_UP * TMath::Cos( phi2 ) * (( 1. / ES_UP_scale ) - 1.);
                dy2_UP = pt2_UP * TMath::Sin( phi2 ) * (( 1. / ES_UP_scale ) - 1.);
                double pt1_UP;
                double mass1_UP=m1;
                if (decayMode1!=0) mass1_UP = m1 * ES_UP_scale;
                pt1_UP = pt1 * ES_UP_scale;
                double metcorr_ex_01UP, metcorr_ey_01UP;
                double metcorr_ex_10UP, metcorr_ey_10UP;
                double metcorr_ex_11UP, metcorr_ey_11UP;
                double dx1_UP, dy1_UP;
                dx1_UP = pt1_UP * TMath::Cos( phi1 ) * (( 1. / ES_UP_scale ) - 1.);
                dy1_UP = pt1_UP * TMath::Sin( phi1 ) * (( 1. / ES_UP_scale ) - 1.);
                metcorr_ex_01UP = measuredMETx + dx2_UP;
                metcorr_ey_01UP = measuredMETy + dy2_UP;
                metcorr_ex_10UP = measuredMETx + dx1_UP;
                metcorr_ey_10UP = measuredMETy + dy1_UP;
                metcorr_ex_11UP = measuredMETx + dx2_UP + dx1_UP;
                metcorr_ey_11UP = measuredMETy + dy2_UP + dy1_UP;

                float ES_DOWN_scale=1.0; // jet
                if (year==2016){
                    if (gen_match_2==5 && decayMode2==0) ES_DOWN_scale=0.990; // for real taus
                    if (gen_match_2==5 && decayMode2==0) ES_DOWN_scale=0.991; // for real taus
                    if (gen_match_2==5 && decayMode2==0) ES_DOWN_scale=0.989; // for real taus
                }
                else if (year==2017){
                    if (gen_match_2==5 && decayMode2==0) ES_DOWN_scale=0.992; // for real taus
                    if (gen_match_2==5 && decayMode2==0) ES_DOWN_scale=0.992; // for real taus
                    if (gen_match_2==5 && decayMode2==0) ES_DOWN_scale=0.991; // for real taus
                }
                else if (year==2018){
                    if (gen_match_2==5 && decayMode2==0) ES_DOWN_scale=0.989; // for real taus
                    if (gen_match_2==5 && decayMode2==0) ES_DOWN_scale=0.992; // for real taus
                    if (gen_match_2==5 && decayMode2==0) ES_DOWN_scale=0.991; // for real taus
                }
                double pt2_DOWN;
                double mass2_DOWN = m2;
                pt2_DOWN = pt2 * ES_DOWN_scale;
                if (decayMode2!=0) mass2_DOWN = m2 * ES_DOWN_scale;
                double pt1_DOWN;
                double mass1_DOWN = m1;
                pt1_DOWN = pt1 * ES_DOWN_scale;
                if (decayMode1!=0) mass1_DOWN = m1 * ES_DOWN_scale;
                double metcorr_ex_01DOWN, metcorr_ey_01DOWN;
                double metcorr_ex_10DOWN, metcorr_ey_10DOWN;
                double metcorr_ex_11DOWN, metcorr_ey_11DOWN;
                double dx1_DOWN, dy1_DOWN;
                double dx2_DOWN, dy2_DOWN;
                dx2_DOWN = pt2_DOWN * TMath::Cos( phi2 ) * (( 1. / ES_DOWN_scale ) - 1.);
                dy2_DOWN = pt2_DOWN * TMath::Sin( phi2 ) * (( 1. / ES_DOWN_scale ) - 1.);
                dx1_DOWN = pt1_DOWN * TMath::Cos( phi1 ) * (( 1. / ES_DOWN_scale ) - 1.);
                dy1_DOWN = pt1_DOWN * TMath::Sin( phi1 ) * (( 1. / ES_DOWN_scale ) - 1.);
                metcorr_ex_01DOWN = measuredMETx + dx2_DOWN;
                metcorr_ey_01DOWN = measuredMETy + dy2_DOWN;
                metcorr_ex_10DOWN = measuredMETx + dx1_DOWN;
                metcorr_ey_10DOWN = measuredMETy + dy1_DOWN;
                metcorr_ex_11DOWN = measuredMETx + dx2_DOWN + dx1_DOWN;
                metcorr_ey_11DOWN = measuredMETy + dy2_DOWN + dy1_DOWN;

                std::vector<classic_svFit::MeasuredTauLepton> measuredTauLeptons01UP;
                measuredTauLeptons01UP.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay, pt1, eta1,  phi1, m1, decayMode1));
                measuredTauLeptons01UP.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay,  pt2_UP, eta2, phi2,  mass2_UP, decayMode2));

                std::vector<classic_svFit::MeasuredTauLepton> measuredTauLeptons01DOWN;
                measuredTauLeptons01DOWN.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay, pt1, eta1,  phi1, m1, decayMode1));
                measuredTauLeptons01DOWN.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay,  pt2_DOWN, eta2, phi2,  mass2_DOWN, decayMode2));

                std::vector<classic_svFit::MeasuredTauLepton> measuredTauLeptons10UP;
                measuredTauLeptons10UP.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay, pt1_UP, eta1,  phi1, mass1_UP, decayMode1));
                measuredTauLeptons10UP.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay,  pt2, eta2, phi2,  m2, decayMode2));

                std::vector<classic_svFit::MeasuredTauLepton> measuredTauLeptons10DOWN;
                measuredTauLeptons10DOWN.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay, pt1_DOWN, eta1,  phi1, mass1_DOWN, decayMode1));
                measuredTauLeptons10DOWN.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay,  pt2, eta2, phi2,  m2, decayMode2));

                std::vector<classic_svFit::MeasuredTauLepton> measuredTauLeptons11UP;
                measuredTauLeptons11UP.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay, pt1_UP, eta1,  phi1, mass1_UP, decayMode1));
                measuredTauLeptons11UP.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay,  pt2_UP, eta2, phi2,  mass2_UP, decayMode2));

                std::vector<classic_svFit::MeasuredTauLepton> measuredTauLeptons11DOWN;
                measuredTauLeptons11DOWN.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay, pt1_DOWN, eta1,  phi1, mass1_DOWN, decayMode1));
                measuredTauLeptons11DOWN.push_back(classic_svFit::MeasuredTauLepton(classic_svFit::MeasuredTauLepton::kTauToHadDecay,  pt2_DOWN, eta2, phi2,  mass2_DOWN, decayMode2));

	     if (gen_match_2<6 && gen_match_1<6){
	       runSVFit(measuredTauLeptons11DOWN, metcorr_ex_11DOWN, metcorr_ey_11DOWN, covMET, 0, svFitMass_11DOWN, svFitPt_11DOWN, svFitEta_11DOWN, svFitPhi_11DOWN);
               runSVFit(measuredTauLeptons11UP, metcorr_ex_11UP, metcorr_ey_11UP, covMET, 0, svFitMass_11UP, svFitPt_11UP, svFitEta_11UP, svFitPhi_11UP);
	    }
	    else{
	       svFitMass_11DOWN=svFitMass;
               svFitPt_11DOWN=svFitPt;
               svFitEta_11DOWN=svFitEta;
               svFitPhi_11DOWN=svFitPhi;
               svFitMass_11UP=svFitMass;
               svFitPt_11UP=svFitPt;
               svFitEta_11UP=svFitEta;
               svFitPhi_11UP=svFitPhi;
	    }
             if (gen_match_2<6){
               runSVFit(measuredTauLeptons01DOWN, metcorr_ex_01DOWN, metcorr_ey_01DOWN, covMET, 0, svFitMass_01DOWN, svFitPt_01DOWN, svFitEta_01DOWN, svFitPhi_01DOWN);
               runSVFit(measuredTauLeptons01UP, metcorr_ex_01UP, metcorr_ey_01UP, covMET, 0, svFitMass_01UP, svFitPt_01UP, svFitEta_01UP, svFitPhi_01UP);
            }
            else{
               svFitMass_01DOWN=svFitMass;
               svFitPt_01DOWN=svFitPt;
               svFitEta_01DOWN=svFitEta;
               svFitPhi_01DOWN=svFitPhi;
               svFitMass_01UP=svFitMass;
               svFitPt_01UP=svFitPt;
               svFitEta_01UP=svFitEta;
               svFitPhi_01UP=svFitPhi;
            }
             if (gen_match_1<6){
               runSVFit(measuredTauLeptons10DOWN, metcorr_ex_10DOWN, metcorr_ey_10DOWN, covMET, 0, svFitMass_10DOWN, svFitPt_10DOWN, svFitEta_10DOWN, svFitPhi_10DOWN);
               runSVFit(measuredTauLeptons10UP, metcorr_ex_10UP, metcorr_ey_10UP, covMET, 0, svFitMass_10UP, svFitPt_10UP, svFitEta_10UP, svFitPhi_10UP);
            }
            else{
               svFitMass_10DOWN=svFitMass;
               svFitPt_10DOWN=svFitPt;
               svFitEta_10DOWN=svFitEta;
               svFitPhi_10DOWN=svFitPhi;
               svFitMass_10UP=svFitMass;
               svFitPt_10UP=svFitPt;
               svFitEta_10UP=svFitEta;
               svFitPhi_10UP=svFitPhi;
            }
	 }

         newBranch1->Fill();
         newBranch2->Fill();
         newBranch3->Fill();
         newBranch4->Fill();
         newBranch1U01->Fill();
         newBranch2U01->Fill();
         newBranch3U01->Fill();
         newBranch4U01->Fill();
         newBranch1D01->Fill();
         newBranch2D01->Fill();
         newBranch3D01->Fill();
         newBranch4D01->Fill();
         newBranch1U10->Fill();
         newBranch2U10->Fill();
         newBranch3U10->Fill();
         newBranch4U10->Fill();
         newBranch1D10->Fill();
         newBranch2D10->Fill();
         newBranch3D10->Fill();
         newBranch4D10->Fill();
         newBranch1U11->Fill();
         newBranch2U11->Fill();
         newBranch3U11->Fill();
         newBranch4U11->Fill();
         newBranch1D11->Fill();
         newBranch2D11->Fill();
         newBranch3D11->Fill();
         newBranch4D11->Fill();
         newBranch1UU->Fill();
         newBranch2UU->Fill();
         newBranch3UU->Fill();
         newBranch4UU->Fill();
         newBranch1UD->Fill();
         newBranch2UD->Fill();
         newBranch3UD->Fill();
         newBranch4UD->Fill();
         newBranch1ResponseU->Fill();
         newBranch1ResponseD->Fill();
         newBranch1ResolutionU->Fill();
         newBranch1ResolutionD->Fill();
         newBranch1JetEta0to3U->Fill();
         newBranch1JetEta0to3D->Fill();
         newBranch1JetEC2U->Fill();
         newBranch1JetEC2D->Fill();
         newBranch1JetEta3to5U->Fill();
         newBranch1JetEta3to5D->Fill();
         newBranch1JetEta0to5U->Fill();
         newBranch1JetEta0to5D->Fill();
         newBranch1JetRelativeBalU->Fill();
         newBranch1JetRelativeBalD->Fill();
         newBranch1JetRelativeSampleU->Fill();
         newBranch1JetRelativeSampleD->Fill();
    }
      dir->cd();
      t->Write("",TObject::kOverwrite);
      strcpy(TreeToUse,stringA) ;

  }
 }
}

void runSVFit(std::vector<classic_svFit::MeasuredTauLepton> & measuredTauLeptons, double measuredMETx, double measuredMETy, TMatrixD &covMET, float num, float &svFitMass, float& svFitPt, float &svFitEta, float &svFitPhi){
  FastMTT aFastMTTAlgo;
  aFastMTTAlgo.run(measuredTauLeptons,measuredMETx,measuredMETy,covMET);
  LorentzVector ttP4 = aFastMTTAlgo.getBestP4();
  svFitMass = ttP4.M(); // return value is in units of GeV
  svFitPt = ttP4.Pt();
  svFitEta = ttP4.Eta();
  svFitPhi = ttP4.Phi();
  std::cout << "found mass = " << svFitMass << std::endl;

}

void CopyDir(TDirectory *source, optutl::CommandLineParser parser) {
  TDirectory *savdir = gDirectory;
  TDirectory *adir = savdir; 
  if(source->GetName()!=parser.stringValue("inputFile")){
    adir = savdir->mkdir(source->GetName());
    std::cout<<"Source name is not outputfile name"<<std::endl;
    adir->cd();    
  }
  else{
    adir->cd();    
  }

  //loop on all entries of this directory
  TKey *key;
  TIter nextkey(source->GetListOfKeys());
  while ((key = (TKey*)nextkey())) {
    std::cout<<"My key is: "<<key->GetName()<<std::endl;
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

