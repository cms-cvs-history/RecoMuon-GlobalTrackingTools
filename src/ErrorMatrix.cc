#include "RecoMuon/GlobalTrackingTools/interface/ErrorMatrix.h"
#include "TString.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include <sstream>

using namespace std;

const TString ErrorMatrix::vars[5]={"#frac{q}{|p|}","#lambda","#varphi_{0}","X_{T}","Y_{T}"};

ErrorMatrix::ErrorMatrix(const edm::ParameterSet & iConfig){
  theCategory="ErrorMatrix";
  std::string action = iConfig.getParameter<std::string>("action");
  std::string fileName = iConfig.getParameter<std::string>("rootFileName");
  const char * filename = fileName.c_str();
  ErrorMatrix::action a= use;

   int NPt=5;
   double minPt=1;
   double maxPt=200;
   int NEta=5;
   double minEta=0;
   double maxEta=2.5;
   int NPhi=1;
   double minPhi=-TMath::Pi();
   double maxPhi=TMath::Pi();

  if (action!="use"){
    a = constructor;
    
    NPt = iConfig.getParameter<int>("NPt");
    minPt = iConfig.getParameter<double>("minPt");
    maxPt = iConfig.getParameter<double>("maxPt");
    NEta = iConfig.getParameter<int>("NEta");
    minEta = iConfig.getParameter<double>("minEta");
    maxEta = iConfig.getParameter<double>("maxEta");
    NPhi = iConfig.getParameter<int>("NPhi");
    std::stringstream get(iConfig.getParameter<std::string>("minPhi"));
    if (get.str() =="-Pi")
      {	minPhi =-TMath::Pi();}
    else if(get.str() =="Pi")
      { minPhi =TMath::Pi();}
    else { get>>minPhi;}
    get.str(iConfig.getParameter<std::string>("maxPhi"));
        if (get.str() =="-Pi")
      {	maxPhi =-TMath::Pi();}
    else if(get.str() =="Pi")
      { maxPhi =TMath::Pi();}
    else { get>>maxPhi;}
  }

  edm::LogInfo(theCategory)<<((a==use)?"using":"creating")
					 <<" an error matrix object: "<<filename;
  theF = new TFile(filename,((a==use)?"":"recreate"));
  
  if (!theF->IsOpen()){
    edm::LogError(theCategory)<<" cannot read file "<<filename;}
  else{
    if (a==use){gROOT->cd();}
    else {theF->cd();}

    for (int i=0;i!=5;i++){for (int j=i;j!=5;j++){
	TString pfname(Form("pf3_V%1d%1d",i+1,j+1));
	TProfile3D * pf =0;
	if (a==use){
	  edm::LogVerbatim(theCategory)<<"getting "<<pfname<<" from "<<filename;
	  pf = (TProfile3D *)theF->Get(pfname);
	  //	  pf = new TProfile3D(*pf); //make a copy of it
	  theData[thePindex(i,j)]=pf;
	}
	else{
	  //	  curvilinear coordinate system
	  //need to make some input parameter to be to change the number of bins

	  TString pftitle;
	  if (i==j){pftitle="#sigma^{2}_{"+vars[i]+"}";}
	  else{pftitle="#rho("+vars[i]+","+vars[j]+")";}
	  edm::LogVerbatim(theCategory)<<"booking "<<pfname<<" into "<<filename;
	  pf = new TProfile3D(pfname,pftitle,NPt,minPt,maxPt,NEta,minEta,maxEta,NPhi,minPhi,maxPhi,"S");	    
	  pf->SetXTitle("muon p_{T} [GeV]");
	  pf->SetYTitle("muon |#eta|");
	  pf->SetZTitle("muon #varphi");
	}
	LogDebug(theCategory)<<" index "<<i<<":"<<j<<" -> "<<thePindex(i,j);
	theData[thePindex(i,j)]=pf;
	if (!pf){
	  edm::LogError(theCategory)<<" profile "<<pfname<<" in file "<<filename<<" is not valid. exiting.";
	  exit(1);
	}
      }}
    
    //verify it
    for (int i=0;i!=15;i++){ 
      if (theData[i]) {edm::LogVerbatim(theCategory)<<i<<" :"<<theData[i]->GetName()
								<<" "<< theData[i]->GetTitle()<<std::endl;}}
  }
}

void ErrorMatrix::close(){
  //close the file
  if (theF->IsOpen()){
    theF->cd();
    //write to it first if constructor
    if (theF->IsWritable()){
      for (int i=0;i!=15;i++){ if (theData[i]) { theData[i]->Write();}}}
    theF->Close();
  }}

ErrorMatrix::~ErrorMatrix()  {
  close();
}

CurvilinearTrajectoryError ErrorMatrix::get(GlobalVector momentum)  {
  AlgebraicSymMatrix55 V;
  for (int i=0;i!=5;i++){for (int j=i;j!=5;j++){
      V(i,j) = theValue(momentum,i,j);}}
  return CurvilinearTrajectoryError(V);}

/*CurvilinearTrajectoryError ErrorMatrix::get_random(GlobalVector momentum)  { 
  static TRandom2 rand;
  AlgebraicSymMatrix55 V;//result
  //first proceed with diagonal elements
  for (int i=0;i!=5;i++){
  V(i,i)=rand.Gaus(theValue(momentum,i,i),theRms(momentum,i,i));}

  //now proceed with the correlations
  for (int i=0;i!=5;i++){for (int j=i+1;j<5;j++){
      double corr = rand.Gaus(theValue(momentum,i,j),theRms(momentum,i,j));
      //assign the covariance from correlation and sigmas
      V(i,j)= corr * sqrt( V[i][i] * V[j][j]);}}
  return CurvilinearTrajectoryError(V);  }
*/


double ErrorMatrix::theValue(GlobalVector & momentum, int i, int j)  {
  double result=0;
  TProfile3D * ij = theIndex(i,j);
  if (!ij) {edm::LogError(theCategory)<<"cannot get the profile ("<<i<<":"<<j<<")"; return result;}
  
  double pT = momentum.perp();
  double eta = fabs(momentum.eta());
  double phi = momentum.phi();


  //  int iBin= ij->FindBin(pT,eta,phi);
  int iBin_x= ij->GetXaxis()->FindBin(pT);
  int iBin_y= ij->GetYaxis()->FindBin(eta);
  int iBin_z= ij->GetZaxis()->FindBin(phi);

  if (i!=j){
    //return the covariance = correlation*sigma_1 *sigma_2;
    TProfile3D * ii = theIndex(i,i);
    TProfile3D * jj = theIndex(j,j);
    if (!ii){edm::LogError(theCategory)<<"cannot get the profile ("<<i<<":"<<i<<")"; return result;}
    if (!jj){edm::LogError(theCategory)<<"cannot get the profile ("<<j<<":"<<j<<")"; return result;}


    int iBin_i_x = ii->GetXaxis()->FindBin(pT);
    int iBin_i_y = ii->GetYaxis()->FindBin(eta);
    int iBin_i_z = ii->GetZaxis()->FindBin(phi);
    int iBin_j_x = jj->GetXaxis()->FindBin(pT);
    int iBin_j_y = jj->GetYaxis()->FindBin(eta);
    int iBin_j_z = jj->GetZaxis()->FindBin(phi);

    double corr = ij->GetBinContent(iBin_x,iBin_y,iBin_z);
    double sigma2_1 = (ii->GetBinContent(iBin_i_x,iBin_i_y,iBin_i_z));
    double sigma2_2 = (jj->GetBinContent(iBin_j_x,iBin_j_y,iBin_j_z));
    double sigma_1 = sqrt(sigma2_1);
    double sigma_2 = sqrt(sigma2_2);

    LogDebug(theCategory)<<"for: (pT,eta,phi)=("<<pT<<", "<<eta<<", "<<phi<<") nterms are"
		       <<"\nrho["<<i<<","<<j<<"]: "<<corr<<" ["<< iBin_x<<", "<<iBin_y<<", "<<iBin_z<<"]"
		       <<"\nsigma^2["<<i<<","<<i<<"]: "<< sigma2_1<<" ["<< iBin_i_x<<", "<<iBin_i_y<<", "<<iBin_i_z<<"]"
		       <<"\nsigma^2["<<j<<","<<j<<"]: "<< sigma2_2<<" ["<< iBin_i_x<<", "<<iBin_i_y<<", "<<iBin_i_z<<"]"
		       <<"\nsigma["<<i<<","<<i<<"]: "<< sigma_1
		       <<"\nsigma["<<j<<","<<j<<"]: "<< sigma_2;
    
    result=corr*sigma_1*sigma_2;
    LogDebug(theCategory)<<"for: (pT,eta,phi)=("<<pT<<", "<<eta<<", "<<phi<<") Covariance["<<i<<","<<j<<"] is: "<<result;
    return result;
  }
  else{
    //return the variance = sigma_1 **2
    //    result=ij->GetBinContent(iBin);
    result=ij->GetBinContent(iBin_x,iBin_y,iBin_z);
    LogDebug(theCategory)<<"for: (pT,eta,phi)=("<<pT<<", "<<eta<<", "<<phi<<") sigma^2["<<i<<","<<j<<"] is: "<<result;
    return result;
  }
}

double ErrorMatrix::theRms(GlobalVector & momentum, int i, int j)  {
  double result=0;
  TProfile3D * ij = theIndex(i,j);
  if (!ij){edm::LogError(theCategory)<<"cannot get the profile ("<<i<<":"<<i<<")"; return result;} 
  double pT = momentum.perp();
  double eta = fabs(momentum.eta());
  double phi = momentum.phi();
  
  //  int iBin= ij->FindBin(pT,eta,phi);
  //  result=ij->GetBinError(iBin);
  int iBin_x= ij->GetXaxis()->FindBin(pT);
  int iBin_y= ij->GetYaxis()->FindBin(eta);
  int iBin_z= ij->GetZaxis()->FindBin(phi);
  result=ij->GetBinError(iBin_x,iBin_y,iBin_z);

  LogDebug(theCategory)<<"for: (pT,eta,phi)=("<<pT<<", "<<eta<<", "<<phi<<") error["<<i<<","<<j<<"] is: "<<result;
  return result;
}
