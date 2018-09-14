#include <iostream>
#include "stdio.h"
#include <string.h>
#include <math.h>

#include "DLM_WfModel.h"
#include "CATS.h"

using namespace std;

const complex<float>i(0,1);

DLM_WfModels::DLM_WfModels(WfModel Model):MODEL(Model),
    NumChannels(Model==ProtonLambdaHaideNLO?4:Model==KminProtonHaide?4:Model==KplusProtonHaide?2:1){
    //NumChannels(Model==ProtonLambdaHaideNLO?4:Model==KminProtonHaide?4:Model==KplusProtonFiles?2:1){

    WaveFunctionU = NULL;
    PhaseShifts = NULL;
    RadBins = NULL;
    NumRadBins = 0;

}

DLM_WfModels::~DLM_WfModels(){
    if(WaveFunctionU){
        delete [] WaveFunctionU;
        WaveFunctionU = NULL;
    }
    if(PhaseShifts){
        delete [] PhaseShifts;
        PhaseShifts = NULL;
    }
    if(RadBins){
        delete [] RadBins;
        RadBins = NULL;
    }

}

unsigned DLM_WfModels::GetNumRadBins(){
    return NumRadBins;
}

void DLM_WfModels::Init(const CATS& Kitty, const char* inFileName){

}

//_0 = rotated WF onto the real axis (as done originally)
//_1 = using the complex WF
//0_ = NLO
//1_ = LO
void InitHaidenbauerNLO(const char* InputFolder, CATS& Kitty, complex<double>***** WaveFunctionU, double**** PhaseShifts, double** RadBins, unsigned& NumRadBins, const int& TYPE, const int& CUTOFF){

    unsigned NumMomBins=Kitty.GetNumMomBins();
    //double kMin = Kitty.GetMomBinLowEdge(0);
    //double kMax = Kitty.GetMomBinUpEdge(NumMomBins-1);

    const unsigned short NumChannels = 4;
    const unsigned short NumPwPerCh = 2;
    const unsigned short NumFiles = 6;
    enum HaideFiles {f1S0, f1P1, f3S1, f3P0, f3P1, f3P2};

    char** InputFileName = new char* [NumFiles];
    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        InputFileName[uFile] = new char[256];
    }

    strcpy(InputFileName[f1S0],InputFolder);
    strcpy(InputFileName[f1P1],InputFolder);
    strcpy(InputFileName[f3S1],InputFolder);
    strcpy(InputFileName[f3P0],InputFolder);
    strcpy(InputFileName[f3P1],InputFolder);
    strcpy(InputFileName[f3P2],InputFolder);

    if(TYPE%10==0 && CUTOFF==600){
        strcat(InputFileName[f1S0], "N1s0.data");
        strcat(InputFileName[f1P1], "N1p1.data");
        strcat(InputFileName[f3S1], "N3s1.data");
        strcat(InputFileName[f3P0], "N3p0.data");
        strcat(InputFileName[f3P1], "N3p1.data");
        strcat(InputFileName[f3P2], "N3p2.data");
    }
    else if(TYPE%10==0 && CUTOFF==500){
        strcat(InputFileName[f1S0], "Y1s05.data");
        strcat(InputFileName[f3S1], "Y3s15.data");
    }
    else if(TYPE%10==1 && CUTOFF==600){
        strcat(InputFileName[f1S0], "W1s06.data");
        strcat(InputFileName[f3S1], "W3s16.data");
    }
    else{
        printf("YOU BROKE SOMETHING in InitHaidenbauerNLO\n");
    }


    const unsigned NumBlankTransitionLines = 1;
    const double MinRad = 0.2;
    const double RadStep = 0.1;
    const double MaxRad = 16.1;
    NumRadBins = round((MaxRad-MinRad)/RadStep) + 1;
    bool* RadBinLoaded = new bool [NumRadBins+1];

    WaveFunctionU[0] = new complex<double>*** [NumMomBins];
    PhaseShifts[0] = new double** [NumMomBins];
    for(unsigned uMomBin=0; uMomBin<NumMomBins; uMomBin++){
        WaveFunctionU[0][uMomBin] = new complex<double>** [NumChannels];
        PhaseShifts[0][uMomBin] = new double* [NumChannels];
        for(unsigned usCh=0; usCh<NumChannels; usCh++){
            WaveFunctionU[0][uMomBin][usCh] = new complex<double>* [NumPwPerCh];
            PhaseShifts[0][uMomBin][usCh] = new double [NumPwPerCh];
            for(unsigned usPw=0; usPw<NumPwPerCh; usPw++){
                WaveFunctionU[0][uMomBin][usCh][usPw] = new complex<double> [NumRadBins];
                PhaseShifts[0][uMomBin][usCh][usPw] = 0;
            }
        }
    }

    RadBins[0] = new double [NumRadBins+1];
    for(unsigned uRad=0; uRad<=NumRadBins; uRad++){
        RadBins[0][uRad] = MinRad-RadStep*0.5 + RadStep*double(uRad);
        RadBinLoaded[uRad] = false;
    }

    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        FILE *InFile;
        InFile = fopen(InputFileName[uFile], "r");
        if(!InFile){
            printf("\033[1;31mERROR:\033[0m The file\033[0m %s cannot be opened!\n", InputFileName[uFile]);
            return;
        }
        //we have the p-waves only for NLO with cutoff of 600 MeV
        if(uFile!=f1S0 && uFile!=f3S1 && (TYPE/10!=0 || CUTOFF!=600)) continue;

        fseek ( InFile , 0 , SEEK_END );
        //long EndPos;
        //EndPos = ftell (InFile);
        fseek ( InFile , 0 , SEEK_SET );
        //long CurPos;

        char* cdummy = new char [256];

        //Read the header lines
        for(unsigned short us=0; us<NumBlankTransitionLines; us++){
            fgets(cdummy, 255, InFile);
        }

        if(feof(InFile)){
            printf("\033[1;31mERROR:\033[0m Trying to read past end of file %s\n", InputFileName[uFile]);
            return;
        }

        float fRadius;
        float fMomentum;
        float fReWf;
        float fImWf;
        float fReAsWf;
        float fImAsWf;
        float fCatsWf;
        float fDummy;
        float fPhaseShift;

        unsigned RadBin;
        unsigned LastRadBin;
        unsigned MomBin;
        //!---Iteration over all events---
        while(!feof(InFile)){
            fgets(cdummy, 255, InFile);
//printf("I am here - %i!\n", strlen(cdummy));
            if(strlen(cdummy)!=105) continue;
            sscanf(cdummy, " %f %f %f %f %f %f %f %f %f",
                   &fRadius,&fMomentum,&fReWf,&fImWf,&fReAsWf,&fImAsWf,&fCatsWf,&fDummy,&fPhaseShift);
            LastRadBin = RadBin;
            RadBin = Kitty.GetBin(fRadius,RadBins[0],NumRadBins+1);
            //we filled up all rad bins
            if(RadBin<=LastRadBin){
                for(unsigned uRad=0; uRad<=NumRadBins; uRad++){
                    RadBinLoaded[uRad]=false;
                }

            }
            if(RadBin>=NumRadBins) continue;
            if(RadBinLoaded[RadBin]){
                printf("\033[1;33mWARNING:\033[0m Trying to load RadBin #%u twice!\n", RadBin);
                continue;
            }
            MomBin = Kitty.GetMomBin(fMomentum);
            if(MomBin>=NumMomBins) continue;

            //skip momentum bins that we do not have in the CATS object
            if(fabs(fMomentum-Kitty.GetMomentum(MomBin))>0.1){
//printf("fMomentum=%f <> Kitty.GetMomentum(MomBin)=%f\n",fMomentum,Kitty.GetMomentum(MomBin));

                continue;
            }
//printf("!!!!\n");
            switch(uFile){
            case f1S0:
                if(TYPE/10==0) WaveFunctionU[0][MomBin][0][0][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][0][0][RadBin] = (fReWf+i*fImWf)*fRadius;
                PhaseShifts[0][MomBin][0][0] = fPhaseShift*3.14159/180.;
//printf("MomBin=%u; RadBin=%u; WaveFunctionU[0][MomBin][0][0][RadBin]=%f\n",MomBin,RadBin,WaveFunctionU[0][MomBin][0][0][RadBin]);
                break;
            case f1P1:
                WaveFunctionU[0][MomBin][0][1][RadBin] = fCatsWf*fRadius;
                PhaseShifts[0][MomBin][0][1] = fPhaseShift*3.14159/180.;
                break;
            case f3S1:
                if(TYPE/10==0){
                    WaveFunctionU[0][MomBin][1][0][RadBin] = fCatsWf*fRadius;
                    WaveFunctionU[0][MomBin][2][0][RadBin] = fCatsWf*fRadius;
                    WaveFunctionU[0][MomBin][3][0][RadBin] = fCatsWf*fRadius;
                }
                else{
                    WaveFunctionU[0][MomBin][1][0][RadBin] = (fReWf+i*fImWf)*fRadius;
                    WaveFunctionU[0][MomBin][2][0][RadBin] = (fReWf+i*fImWf)*fRadius;
                    WaveFunctionU[0][MomBin][3][0][RadBin] = (fReWf+i*fImWf)*fRadius;
                }
                PhaseShifts[0][MomBin][1][0] = fPhaseShift*3.14159/180.;
                PhaseShifts[0][MomBin][2][0] = fPhaseShift*3.14159/180.;
                PhaseShifts[0][MomBin][3][0] = fPhaseShift*3.14159/180.;
                break;
            case f3P0:
                if(TYPE/10==0) WaveFunctionU[0][MomBin][1][1][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][3][1][RadBin] = (fReWf+i*fImWf)*fRadius;
                PhaseShifts[0][MomBin][1][1] = fPhaseShift*3.14159/180.;
                break;
            case f3P1:
                if(TYPE/10==0) WaveFunctionU[0][MomBin][2][1][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][3][1][RadBin] = (fReWf+i*fImWf)*fRadius;
                PhaseShifts[0][MomBin][2][1] = fPhaseShift*3.14159/180.;
                break;
            case f3P2:
                if(TYPE/10==0) WaveFunctionU[0][MomBin][3][1][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][3][1][RadBin] = (fReWf+i*fImWf)*fRadius;
                PhaseShifts[0][MomBin][3][1] = fPhaseShift*3.14159/180.;
                break;
            default:
                printf("\033[1;31mERROR:\033[0m SWITCH CHANNELS AND PWS\n");
                break;
            }
            RadBinLoaded[RadBin] = true;
        }
        delete [] cdummy;
        fclose(InFile);
    }//uFile

    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        delete [] InputFileName[uFile];
    }
    delete [] InputFileName;

    delete [] RadBinLoaded;
//printf("hhhhhhhhhhhh\n");
}

//TYPE==1 take the complex wf
void InitHaidenbauerKaonMinus(const char* InputFolder, CATS& Kitty, complex<double>***** WaveFunctionU, double**** PhaseShifts, double** RadBins, unsigned& NumRadBins, const int& TYPE){

    unsigned NumMomBins=Kitty.GetNumMomBins();
    //double kMin = Kitty.GetMomBinLowEdge(0);
    //double kMax = Kitty.GetMomBinUpEdge(NumMomBins-1);

    const unsigned short NumChannels = 4;
    const unsigned short NumPwPerCh = 2;
    const unsigned short NumFiles = 6;
    enum HaideFiles {fS01, fS11, fP01, fP03, fP11, fP13};

    char** InputFileName = new char* [NumFiles];
    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        InputFileName[uFile] = new char[256];
    }
//KaonMinus_SIonly
//KaonMinus10MeV

    strcpy(InputFileName[fS01],InputFolder);
    strcpy(InputFileName[fS11],InputFolder);
    strcpy(InputFileName[fP01],InputFolder);
    strcpy(InputFileName[fP03],InputFolder);
    strcpy(InputFileName[fP11],InputFolder);
    strcpy(InputFileName[fP13],InputFolder);

    strcat(InputFileName[fS01], "ws01.dat");
    strcat(InputFileName[fS11], "ws11.dat");
    strcat(InputFileName[fP01], "wp01.dat");
    strcat(InputFileName[fP03], "wp03.dat");
    strcat(InputFileName[fP11], "wp11.dat");
    strcat(InputFileName[fP13], "wp13.dat");

    const unsigned NumBlankTransitionLines = 1;
    const double MinRad = 0.1;
    const double RadStep = 0.1;
    const double MaxRad = 10.0;
    NumRadBins = round((MaxRad-MinRad)/RadStep) + 1;
    bool* RadBinLoaded = new bool [NumRadBins+1];

    WaveFunctionU[0] = new complex<double>*** [NumMomBins];
    PhaseShifts[0] = new double** [NumMomBins];
    for(unsigned uMomBin=0; uMomBin<NumMomBins; uMomBin++){
        WaveFunctionU[0][uMomBin] = new complex<double>** [NumChannels];
        PhaseShifts[0][uMomBin] = new double* [NumChannels];
        for(unsigned usCh=0; usCh<NumChannels; usCh++){
            WaveFunctionU[0][uMomBin][usCh] = new complex<double>* [NumPwPerCh];
            PhaseShifts[0][uMomBin][usCh] = new double [NumPwPerCh];
            for(unsigned usPw=0; usPw<NumPwPerCh; usPw++){
                WaveFunctionU[0][uMomBin][usCh][usPw] = new complex<double> [NumRadBins];
                PhaseShifts[0][uMomBin][usCh][usPw] = 0;
            }
        }
    }

    RadBins[0] = new double [NumRadBins+1];
    for(unsigned uRad=0; uRad<=NumRadBins; uRad++){
        RadBins[0][uRad] = MinRad-RadStep*0.5 + RadStep*double(uRad);
        RadBinLoaded[uRad] = false;
    }
//printf("Hey!\n");
    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        FILE *InFile;
        InFile = fopen(InputFileName[uFile], "r");
        if(!InFile){
            printf("\033[1;31mERROR:\033[0m The file\033[0m %s cannot be opened!\n", InputFileName[uFile]);
            return;
        }
//printf("uFile=%u\n",uFile);
        fseek ( InFile , 0 , SEEK_END );
        //long EndPos;
        //EndPos = ftell (InFile);
        fseek ( InFile , 0 , SEEK_SET );
        //long CurPos;

        char* cdummy = new char [256];

        //Read the header lines
        for(unsigned short us=0; us<NumBlankTransitionLines; us++){
            fgets(cdummy, 255, InFile);
        }

        if(feof(InFile)){
            printf("\033[1;31mERROR:\033[0m Trying to read past end of file %s\n", InputFileName[uFile]);
            return;
        }

        float fRadius;
        float fMomentum;
        float fReWf;
        float fImWf;
        float fReAsWf;
        float fImAsWf;
        float fCatsReWf;
        float fCatsImWf;
        float fCatsWf;
        float fPhaseShift;

        unsigned RadBin;
        unsigned LastRadBin;
        unsigned MomBin;
        //!---Iteration over all events---
        while(!feof(InFile)){
            fgets(cdummy, 255, InFile);
//printf("I am here - %i!\n", strlen(cdummy));
            if(strlen(cdummy)<105 || strlen(cdummy)>106) continue;
            sscanf(cdummy, " %f %f %f %f %f %f %f %f %f",
                   &fRadius,&fMomentum,&fReWf,&fImWf,&fReAsWf,&fImAsWf,&fCatsReWf,&fCatsImWf,&fPhaseShift);
            //printf("WF = %f + i*%f\n",fReWf,fImWf);
            //sscanf(cdummy, " %f %f %f %f",
            //       &fRadius,&fMomentum,&fCatsReWf,&fCatsImWf);
            fCatsWf = sqrt(fCatsReWf*fCatsReWf+fCatsImWf*fCatsImWf);
            //if(fCatsReWf<0) fCatsWf=-fCatsWf;
            //fCatsWf = fReWf;
            LastRadBin = RadBin;
            RadBin = Kitty.GetBin(fRadius,RadBins[0],NumRadBins+1);
            //we filled up all rad bins
            if(RadBin<=LastRadBin){
                for(unsigned uRad=0; uRad<=NumRadBins; uRad++){
                    RadBinLoaded[uRad]=false;
                }

            }
            if(RadBin>=NumRadBins) continue;
            if(RadBinLoaded[RadBin]){
                printf("\033[1;33mWARNING:\033[0m Trying to load RadBin #%u twice!\n", RadBin);
                continue;
            }
            MomBin = Kitty.GetMomBin(fMomentum);
            if(MomBin>=NumMomBins) continue;

            //skip momentum bins that we do not have in the CATS object
            if(fabs(fMomentum-Kitty.GetMomentum(MomBin))>0.1){
printf("fMomentum=%f <> Kitty.GetMomentum(MomBin)=%f\n",fMomentum,Kitty.GetMomentum(MomBin));

                continue;
            }
//printf("!!!!\n");

            switch(uFile){
            case fS01://Spin 0, Isospin 0, s-wave
                if(TYPE==0){
                    WaveFunctionU[0][MomBin][0][0][RadBin] = fCatsWf*fRadius;
                    WaveFunctionU[0][MomBin][1][0][RadBin] = fCatsWf*fRadius;
                }
                else{
                    WaveFunctionU[0][MomBin][0][0][RadBin] = (fReWf+i*fImWf)*fRadius;
                    WaveFunctionU[0][MomBin][1][0][RadBin] = (fReWf+i*fImWf)*fRadius;
                }
                PhaseShifts[0][MomBin][0][0] = fPhaseShift*3.14159/180.;
                PhaseShifts[0][MomBin][1][0] = fPhaseShift*3.14159/180.;
                break;
            case fS11:
                if(TYPE==0){
                    WaveFunctionU[0][MomBin][2][0][RadBin] = fCatsWf*fRadius;
                    WaveFunctionU[0][MomBin][3][0][RadBin] = fCatsWf*fRadius;
                }
                else{
                    WaveFunctionU[0][MomBin][2][0][RadBin] = (fReWf+i*fImWf)*fRadius;
                    WaveFunctionU[0][MomBin][3][0][RadBin] = (fReWf+i*fImWf)*fRadius;
                }
                PhaseShifts[0][MomBin][2][0] = fPhaseShift*3.14159/180.;
                PhaseShifts[0][MomBin][3][0] = fPhaseShift*3.14159/180.;
                break;
            case fP01:
                if(TYPE==0) WaveFunctionU[0][MomBin][0][1][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][0][1][RadBin] = (fReWf+i*fImWf)*fRadius;
                PhaseShifts[0][MomBin][0][1] = fPhaseShift*3.14159/180.;
                break;
            case fP03:
                if(TYPE==0) WaveFunctionU[0][MomBin][1][1][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][1][1][RadBin] = (fReWf+i*fImWf)*fRadius;
                PhaseShifts[0][MomBin][1][1] = fPhaseShift*3.14159/180.;
                break;
            case fP11:
                if(TYPE==0) WaveFunctionU[0][MomBin][2][1][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][2][1][RadBin] = (fReWf+i*fImWf)*fRadius;
                PhaseShifts[0][MomBin][2][1] = fPhaseShift*3.14159/180.;
                break;
            case fP13:
                if(TYPE==0) WaveFunctionU[0][MomBin][3][1][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][3][1][RadBin] = (fReWf+i*fImWf)*fRadius;
                PhaseShifts[0][MomBin][3][1] = fPhaseShift*3.14159/180.;
                break;
            default:
                printf("\033[1;31mERROR:\033[0m SWITCH CHANNELS AND PWS\n");
                break;
            }
            RadBinLoaded[RadBin] = true;
        }
        delete [] cdummy;
        fclose(InFile);
    }//uFile

    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        delete [] InputFileName[uFile];
    }
    delete [] InputFileName;

    delete [] RadBinLoaded;
//printf("hhhhhhhhhhhh\n");
}

//see email in may 2018, this is isospin averaged
//basically we have two channels: s1+p1 and s1+p3
//I am pretty sure this already includes the p n mass splitting
//TYPE==1 take the complex wf
void InitHaidenbauerKaonMinus_ver2(const char* InputFolder, CATS& Kitty, complex<double>***** WaveFunctionU, double**** PhaseShifts, double** RadBins, unsigned& NumRadBins, const int& TYPE){

    unsigned NumMomBins=Kitty.GetNumMomBins();
    //double kMin = Kitty.GetMomBinLowEdge(0);
    //double kMax = Kitty.GetMomBinUpEdge(NumMomBins-1);

    const unsigned short NumChannels = 4;
    const unsigned short NumPwPerCh = 2;
    const unsigned short NumFiles = 3;
    enum HaideFiles {fS1, fP1, fP3};

    char** InputFileName = new char* [NumFiles];
    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        InputFileName[uFile] = new char[256];
    }

    strcpy(InputFileName[fS1],InputFolder);
    strcpy(InputFileName[fP1],InputFolder);
    strcpy(InputFileName[fP3],InputFolder);

    strcat(InputFileName[fS1], "ws1.dat");
    strcat(InputFileName[fP1], "wp1.dat");
    strcat(InputFileName[fP3], "wp3.dat");

    const unsigned NumBlankTransitionLines = 1;
    const double MinRad = 0.1;
    const double RadStep = 0.1;
    const double MaxRad = 10.0;
    NumRadBins = round((MaxRad-MinRad)/RadStep) + 1;
    bool* RadBinLoaded = new bool [NumRadBins+1];

    WaveFunctionU[0] = new complex<double>*** [NumMomBins];
    PhaseShifts[0] = new double** [NumMomBins];
    for(unsigned uMomBin=0; uMomBin<NumMomBins; uMomBin++){
        WaveFunctionU[0][uMomBin] = new complex<double>** [NumChannels];
        PhaseShifts[0][uMomBin] = new double* [NumChannels];
        for(unsigned usCh=0; usCh<NumChannels; usCh++){
            WaveFunctionU[0][uMomBin][usCh] = new complex<double>* [NumPwPerCh];
            PhaseShifts[0][uMomBin][usCh] = new double [NumPwPerCh];
            for(unsigned usPw=0; usPw<NumPwPerCh; usPw++){
                WaveFunctionU[0][uMomBin][usCh][usPw] = new complex<double> [NumRadBins];
                PhaseShifts[0][uMomBin][usCh][usPw] = 0;
            }
        }
    }

    RadBins[0] = new double [NumRadBins+1];
    for(unsigned uRad=0; uRad<=NumRadBins; uRad++){
        RadBins[0][uRad] = MinRad-RadStep*0.5 + RadStep*double(uRad);
        RadBinLoaded[uRad] = false;
    }
//printf("Hey!\n");
    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        FILE *InFile;
        InFile = fopen(InputFileName[uFile], "r");
        if(!InFile){
            printf("\033[1;31mERROR:\033[0m The file\033[0m %s cannot be opened!\n", InputFileName[uFile]);
            return;
        }
//printf("uFile=%u\n",uFile);
        fseek ( InFile , 0 , SEEK_END );
        //long EndPos;
        //EndPos = ftell (InFile);
        fseek ( InFile , 0 , SEEK_SET );
        //long CurPos;

        char* cdummy = new char [256];

        //Read the header lines
        for(unsigned short us=0; us<NumBlankTransitionLines; us++){
            fgets(cdummy, 255, InFile);
        }

        if(feof(InFile)){
            printf("\033[1;31mERROR:\033[0m Trying to read past end of file %s\n", InputFileName[uFile]);
            return;
        }

        float fRadius;
        float fMomentum;
        float fReWf;
        float fImWf;
        float fReAsWf;
        float fImAsWf;
        float fCatsReWf;
        float fCatsImWf;
        float fCatsWf;
        float fPhaseShift;

        unsigned RadBin;
        unsigned LastRadBin;
        unsigned MomBin;
        //!---Iteration over all events---
        while(!feof(InFile)){
            fgets(cdummy, 255, InFile);
//printf("I am here - %i!\n", strlen(cdummy));
            if(strlen(cdummy)<105 || strlen(cdummy)>106) continue;
            sscanf(cdummy, " %f %f %f %f %f %f %f %f %f",
                   &fRadius,&fMomentum,&fReWf,&fImWf,&fReAsWf,&fImAsWf,&fCatsReWf,&fCatsImWf,&fPhaseShift);
            //sscanf(cdummy, " %f %f %f %f",
            //       &fRadius,&fMomentum,&fCatsReWf,&fCatsImWf);
            fCatsWf = sqrt(fCatsReWf*fCatsReWf+fCatsImWf*fCatsImWf);
            //if(fCatsReWf<0) fCatsWf=-fCatsWf;
            //fCatsWf = fReWf;
            LastRadBin = RadBin;
            RadBin = Kitty.GetBin(fRadius,RadBins[0],NumRadBins+1);
            //we filled up all rad bins
            if(RadBin<=LastRadBin){
                for(unsigned uRad=0; uRad<=NumRadBins; uRad++){
                    RadBinLoaded[uRad]=false;
                }

            }
            if(RadBin>=NumRadBins) continue;
            if(RadBinLoaded[RadBin]){
                printf("\033[1;33mWARNING:\033[0m Trying to load RadBin #%u twice!\n", RadBin);
                continue;
            }
            MomBin = Kitty.GetMomBin(fMomentum);
            if(MomBin>=NumMomBins) continue;

            //skip momentum bins that we do not have in the CATS object
            if(fabs(fMomentum-Kitty.GetMomentum(MomBin))>0.1){
printf("fMomentum=%f <> Kitty.GetMomentum(MomBin)=%f\n",fMomentum,Kitty.GetMomentum(MomBin));

                continue;
            }
//printf("!!!!\n");
            switch(uFile){
            case fS1://Spin 0, Isospin 0, s-wave
                if(TYPE==0){
                    WaveFunctionU[0][MomBin][0][0][RadBin] = fCatsWf*fRadius;
                    WaveFunctionU[0][MomBin][1][0][RadBin] = fCatsWf*fRadius;
                }
                else{
                    WaveFunctionU[0][MomBin][0][0][RadBin] = (fReWf+i*fImWf)*fRadius;
                    WaveFunctionU[0][MomBin][1][0][RadBin] = (fReWf+i*fImWf)*fRadius;
                }
                PhaseShifts[0][MomBin][0][0] = fPhaseShift*3.14159/180.;
                PhaseShifts[0][MomBin][1][0] = fPhaseShift*3.14159/180.;
                break;
            case fP1:
                if(TYPE==0) WaveFunctionU[0][MomBin][0][1][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][0][1][RadBin] = (fReWf+i*fImWf)*fRadius;
                PhaseShifts[0][MomBin][0][1] = fPhaseShift*3.14159/180.;
                break;
            case fP3:
                if(TYPE==0) WaveFunctionU[0][MomBin][1][1][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][1][1][RadBin] = (fReWf+i*fImWf)*fRadius;
                PhaseShifts[0][MomBin][1][1] = fPhaseShift*3.14159/180.;
                break;
            default:
                printf("\033[1;31mERROR:\033[0m SWITCH CHANNELS AND PWS\n");
                break;
            }
            RadBinLoaded[RadBin] = true;
        }
        delete [] cdummy;
        fclose(InFile);
    }//uFile

    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        delete [] InputFileName[uFile];
    }
    delete [] InputFileName;

    delete [] RadBinLoaded;
//printf("hhhhhhhhhhhh\n");
}


void InitHaidenbauerKaonPlus(const char* InputFolder, CATS& Kitty, complex<double>***** WaveFunctionU, double**** PhaseShifts, double** RadBins, unsigned& NumRadBins, const int& TYPE){

    unsigned NumMomBins=Kitty.GetNumMomBins();
    //double kMin = Kitty.GetMomBinLowEdge(0);
    //double kMax = Kitty.GetMomBinUpEdge(NumMomBins-1);

    const unsigned short NumChannels = 4;//they are two, but I simply keep this for mem management
    const unsigned short NumPwPerCh = 2;
    const unsigned short NumFiles = 3;
    enum HaideFiles {fS11, fP11, fP13};

    char** InputFileName = new char* [NumFiles];
    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        InputFileName[uFile] = new char[256];
    }

    strcpy(InputFileName[fS11],InputFolder);
    strcpy(InputFileName[fP11],InputFolder);
    strcpy(InputFileName[fP13],InputFolder);

    strcat(InputFileName[fS11], "ws11p.dat");
    strcat(InputFileName[fP11], "wp11p.dat");
    strcat(InputFileName[fP13], "wp13p.dat");

    const unsigned NumBlankTransitionLines = 1;
    const double MinRad = 0.1;
    const double RadStep = 0.1;
    const double MaxRad = 10.0;
    NumRadBins = round((MaxRad-MinRad)/RadStep) + 1;
    bool* RadBinLoaded = new bool [NumRadBins+1];

    WaveFunctionU[0] = new complex<double>*** [NumMomBins];
    PhaseShifts[0] = new double** [NumMomBins];
    for(unsigned uMomBin=0; uMomBin<NumMomBins; uMomBin++){
        WaveFunctionU[0][uMomBin] = new complex<double>** [NumChannels];
        PhaseShifts[0][uMomBin] = new double* [NumChannels];
        for(unsigned usCh=0; usCh<NumChannels; usCh++){
            WaveFunctionU[0][uMomBin][usCh] = new complex<double>* [NumPwPerCh];
            PhaseShifts[0][uMomBin][usCh] = new double [NumPwPerCh];
            for(unsigned usPw=0; usPw<NumPwPerCh; usPw++){
                WaveFunctionU[0][uMomBin][usCh][usPw] = new complex<double> [NumRadBins];
                PhaseShifts[0][uMomBin][usCh][usPw] = 0;
            }
        }
    }

    RadBins[0] = new double [NumRadBins+1];
    for(unsigned uRad=0; uRad<=NumRadBins; uRad++){
        RadBins[0][uRad] = MinRad-RadStep*0.5 + RadStep*double(uRad);
        RadBinLoaded[uRad] = false;
    }
printf("Hey!\n");
    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        FILE *InFile;
        InFile = fopen(InputFileName[uFile], "r");
        if(!InFile){
            printf("\033[1;31mERROR:\033[0m The file\033[0m %s cannot be opened!\n", InputFileName[uFile]);
            return;
        }
printf("uFile=%u\n",uFile);
        fseek ( InFile , 0 , SEEK_END );
        //long EndPos;
        //EndPos = ftell (InFile);
        fseek ( InFile , 0 , SEEK_SET );
        //long CurPos;

        char* cdummy = new char [256];

        //Read the header lines
        for(unsigned short us=0; us<NumBlankTransitionLines; us++){
            fgets(cdummy, 255, InFile);
        }

        if(feof(InFile)){
            printf("\033[1;31mERROR:\033[0m Trying to read past end of file %s\n", InputFileName[uFile]);
            return;
        }

        float fRadius;
        float fMomentum;
        float fReWf;
        float fImWf;
        float fReAsWf;
        float fImAsWf;
        float fCatsReWf;
        float fCatsImWf;
        float fCatsWf;
        float fPhaseShift;

        unsigned RadBin;
        unsigned LastRadBin;
        unsigned MomBin;
        //!---Iteration over all events---
        while(!feof(InFile)){
            fgets(cdummy, 255, InFile);
//printf("I am here - %i!\n", strlen(cdummy));
            if(strlen(cdummy)<105 || strlen(cdummy)>106) continue;
            sscanf(cdummy, " %f %f %f %f %f %f %f %f %f",
                   &fRadius,&fMomentum,&fReWf,&fImWf,&fReAsWf,&fImAsWf,&fCatsReWf,&fCatsImWf,&fPhaseShift);
            fCatsWf = sqrt(fCatsReWf*fCatsReWf+fCatsImWf*fCatsImWf);
            LastRadBin = RadBin;
            RadBin = Kitty.GetBin(fRadius,RadBins[0],NumRadBins+1);
            //we filled up all rad bins
            if(RadBin<=LastRadBin){
                for(unsigned uRad=0; uRad<=NumRadBins; uRad++){
                    RadBinLoaded[uRad]=false;
                }

            }
            if(RadBin>=NumRadBins) continue;
            if(RadBinLoaded[RadBin]){
                printf("\033[1;33mWARNING:\033[0m Trying to load RadBin #%u twice!\n", RadBin);
                continue;
            }
            MomBin = Kitty.GetMomBin(fMomentum);
            if(MomBin>=NumMomBins) continue;

            //skip momentum bins that we do not have in the CATS object
            if(fabs(fMomentum-Kitty.GetMomentum(MomBin))>0.1){
printf("fMomentum=%f <> Kitty.GetMomentum(MomBin)=%f\n",fMomentum,Kitty.GetMomentum(MomBin));

                continue;
            }
//printf("!!!!\n");
            switch(uFile){
            case fS11:
                if(TYPE==0){
                    WaveFunctionU[0][MomBin][0][0][RadBin] = fCatsWf*fRadius;
                    WaveFunctionU[0][MomBin][1][0][RadBin] = fCatsWf*fRadius;
                }
                else{
                    WaveFunctionU[0][MomBin][0][0][RadBin] = (fReWf+i*fImWf)*fRadius;
                    WaveFunctionU[0][MomBin][1][0][RadBin] = (fReWf+i*fImWf)*fRadius;
                }
                PhaseShifts[0][MomBin][0][0] = fPhaseShift*3.14159/180.;
                PhaseShifts[0][MomBin][1][0] = fPhaseShift*3.14159/180.;
                break;
            case fP11:
                if(TYPE==0) WaveFunctionU[0][MomBin][0][1][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][0][1][RadBin] = (fReWf+i*fImWf)*fRadius;
                PhaseShifts[0][MomBin][0][1] = fPhaseShift*3.14159/180.;
                break;
            case fP13:
                if(TYPE==0) WaveFunctionU[0][MomBin][1][1][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][1][1][RadBin] = (fReWf+i*fImWf)*fRadius;
                PhaseShifts[0][MomBin][1][1] = fPhaseShift*3.14159/180.;
                break;
            default:
                printf("\033[1;31mERROR:\033[0m SWITCH CHANNELS AND PWS\n");
                break;
            }
            RadBinLoaded[RadBin] = true;
        }
        delete [] cdummy;
        fclose(InFile);
    }//uFile

    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        delete [] InputFileName[uFile];
    }
    delete [] InputFileName;

    delete [] RadBinLoaded;
//printf("hhhhhhhhhhhh\n");
}


//[0][MomBin][uCh][uPw]
//TYPE = 0 => take the psi^2
//TYPE = 1 => the dirty trick with flipping the sign based on the sign of the real part
//TYPE = 2 = Real + Imag
void InitTetsuoKaonMinus(const char* InputFolder, CATS& Kitty, complex<double>***** WaveFunctionU, double**** PhaseShifts, double** RadBins, unsigned& NumRadBins, const int& TYPE){

    unsigned NumMomBins=Kitty.GetNumMomBins();
    //double kMin = Kitty.GetMomBinLowEdge(0);
    //double kMax = Kitty.GetMomBinUpEdge(NumMomBins-1);

    const unsigned short NumChannels = 4;
    const unsigned short NumPwPerCh = 2;
    const unsigned short NumFiles = 1;

    char** InputFileName = new char* [NumFiles];
    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        InputFileName[uFile] = new char[256];
    }
//wf_full
//wf_strong

    strcpy(InputFileName[0],InputFolder);

    strcat(InputFileName[0], "wf_full.dat");

    const unsigned NumBlankTransitionLines = 1;
    const double MinRad = 0.1;
    const double RadStep = 0.1;
    const double MaxRad = 10.0;
    NumRadBins = round((MaxRad-MinRad)/RadStep) + 1;
    bool* RadBinLoaded = new bool [NumRadBins+1];

    WaveFunctionU[0] = new complex<double>*** [NumMomBins];
    PhaseShifts[0] = new double** [NumMomBins];
    for(unsigned uMomBin=0; uMomBin<NumMomBins; uMomBin++){
        WaveFunctionU[0][uMomBin] = new complex<double>** [NumChannels];
        PhaseShifts[0][uMomBin] = new double* [NumChannels];
        for(unsigned usCh=0; usCh<NumChannels; usCh++){
            WaveFunctionU[0][uMomBin][usCh] = new complex<double>* [NumPwPerCh];
            PhaseShifts[0][uMomBin][usCh] = new double [NumPwPerCh];
            for(unsigned usPw=0; usPw<NumPwPerCh; usPw++){
                WaveFunctionU[0][uMomBin][usCh][usPw] = new complex<double> [NumRadBins];
                PhaseShifts[0][uMomBin][usCh][usPw] = 0;
            }
        }
    }

    RadBins[0] = new double [NumRadBins+1];
    for(unsigned uRad=0; uRad<=NumRadBins; uRad++){
        RadBins[0][uRad] = MinRad-RadStep*0.5 + RadStep*double(uRad);
        RadBinLoaded[uRad] = false;
    }

    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        FILE *InFile;
        InFile = fopen(InputFileName[uFile], "r");
        if(!InFile){
            printf("\033[1;31mERROR:\033[0m The file\033[0m %s cannot be opened!\n", InputFileName[uFile]);
            return;
        }
        fseek ( InFile , 0 , SEEK_END );
        fseek ( InFile , 0 , SEEK_SET );
        char* cdummy = new char [256];
        //Read the header lines
        for(unsigned short us=0; us<NumBlankTransitionLines; us++){
            fgets(cdummy, 255, InFile);
        }
        if(feof(InFile)){
            printf("\033[1;31mERROR:\033[0m Trying to read past end of file %s\n", InputFileName[uFile]);
            return;
        }
        float fRadius;
        float fMomentum;
        //float fReWf;
        //float fImWf;
        //float fReAsWf;
        //float fImAsWf;
        float fCatsReWf;
        float fCatsImWf;
        float fCatsWf;
        float fPhaseShift=0;

        unsigned RadBin;
        unsigned LastRadBin;
        unsigned MomBin;
        //!---Iteration over all events---
        while(!feof(InFile)){
            fgets(cdummy, 255, InFile);
            if(strlen(cdummy)<105 || strlen(cdummy)>106) continue;
            sscanf(cdummy, " %f %f %f %f",
                   &fRadius,&fMomentum,&fCatsReWf,&fCatsImWf);
            fCatsWf = sqrt(fCatsReWf*fCatsReWf+fCatsImWf*fCatsImWf);
            if(TYPE==1){
                if(fCatsReWf<0) fCatsWf=-fCatsWf;
                //fCatsWf = fReWf;
            }

            LastRadBin = RadBin;
            RadBin = Kitty.GetBin(fRadius,RadBins[0],NumRadBins+1);
            //we filled up all rad bins
            if(RadBin<=LastRadBin){
                for(unsigned uRad=0; uRad<=NumRadBins; uRad++){
                    RadBinLoaded[uRad]=false;
                }
            }
            if(RadBin>=NumRadBins) continue;
            if(RadBinLoaded[RadBin]){
                printf("\033[1;33mWARNING:\033[0m Trying to load RadBin #%u twice!\n", RadBin);
                continue;
            }
            MomBin = Kitty.GetMomBin(fMomentum);
            if(MomBin>=NumMomBins) continue;

            //skip momentum bins that we do not have in the CATS object
            if(fabs(fMomentum-Kitty.GetMomentum(MomBin))>0.1){
                continue;
            }
            switch(uFile){
            case 0://Spin 0, Isospin 0, s-wave
                if(TYPE==0 || TYPE==1) WaveFunctionU[0][MomBin][0][0][RadBin] = fCatsWf*fRadius;
                else WaveFunctionU[0][MomBin][0][0][RadBin] = (fCatsReWf+i*fCatsImWf)*fRadius;
                PhaseShifts[0][MomBin][0][0] = fPhaseShift*3.14159/180.;
                break;
            default:
                printf("\033[1;31mERROR:\033[0m SWITCH CHANNELS AND PWS\n");
                break;
            }
            RadBinLoaded[RadBin] = true;
        }
        delete [] cdummy;
        fclose(InFile);
    }//uFile

    for(unsigned uFile=0; uFile<NumFiles; uFile++){
        delete [] InputFileName[uFile];
    }
    delete [] InputFileName;

    delete [] RadBinLoaded;
//printf("hhhhhhhhhhhh\n");
}

//TYPE 0 = p k-; //TYPE 1 = n k0
//TYPE 2 = La pi0; //TYPE 3 = Si+ pi-
//TYPE 4 = Si0 pi0; //TYPE 5 = Si- pi+
//Be careful, as due to the weird binning of the wave functions, we actually initialize pretty much everything about the object here
void InitCatForHaidenbauerKaonProton1(const char* InputFolder, CATS& Kitty, complex<double>***** WaveFunctionU, double**** PhaseShifts, double** RadBins, unsigned& NumRadBins, const int& TYPE){
    const double BinCenters[] = {
        0.8145,18.8835,26.0830,
        36.4455,43.7270,47.0314,
        52.0749,54.3229,56.0890,
        57.2364,58.1940,58.4093,
        60.0099,62.6620,65.6609,
        73.5596,81.9185,90.6103,
        99.5477,108.6700,117.9349,
        127.3106,136.7746,146.3098,
        155.9030,165.5444,175.2258,
        184.9410,194.6850,204.4536
    };

    const unsigned NumMomBins = 30;
    double* MomBins = new double [NumMomBins+1];
    MomBins[0] = 0;
    for(unsigned uBin=1; uBin<NumMomBins; uBin++){
        MomBins[uBin] = 0.5*(BinCenters[uBin-1]+BinCenters[uBin]);
        //printf("MomBins[%u]=%.2f\n",uBin,MomBins[uBin]);
    }
    MomBins[NumMomBins] = 2*MomBins[NumMomBins-1]-MomBins[NumMomBins-2];
    Kitty.SetMomBins(NumMomBins, MomBins, BinCenters);

    //for(unsigned uBin=0; uBin<NumMomBins; uBin++){
    //   printf("B%u |%.2f|%.2f|%.2f|\n",uBin,Kitty.GetMomBinLowEdge(uBin),Kitty.GetMomentum(uBin),Kitty.GetMomBinUpEdge(uBin));
    //}

    const double Mass_p = 938.272;
    //const double Mass_n = 939.565;
    const double Mass_Km = 493.677;
    //const double Mass_K0 = 497.648;
    //const double Mass_L = 1115.683;
    //const double Mass_Pi0 = 134.9766;
    //const double Mass_PiCh = 139.57018;
    //const double Mass_Sig0 = 1192.642;
    //const double Mass_SigPos = 1189.37;
    //const double Mass_SigNeg = 1197.449;

    const unsigned NumChannels = 1;

    const unsigned NumChannelsMem = 4;
    const unsigned NumPwPerChMem = 2;

    if(TYPE==0){
        Kitty.SetNumChannels(NumChannels);
        Kitty.SetChannelWeight(0,1);
        Kitty.SetNumPW(0,1);
        Kitty.SetSpin(0,1);

        Kitty.SetQ1Q2(-1);
        Kitty.SetPdgId(2212, -321);//check the pdg id of the kaon
        Kitty.SetRedMass( (Mass_p*Mass_Km)/(Mass_p+Mass_Km) );
    }
    else if(TYPE==1){
        Kitty.SetNumChannels(NumChannels);
        Kitty.SetChannelWeight(0,1);
        Kitty.SetNumPW(0,1);
        Kitty.SetSpin(0,1);

        Kitty.SetQ1Q2(-1);
        Kitty.SetPdgId(2212, -321);//check the pdg id of the kaon
        Kitty.SetRedMass( (Mass_p*Mass_Km)/(Mass_p+Mass_Km) );
    }
    else if(TYPE==2){
        Kitty.SetNumChannels(NumChannels);
        Kitty.SetChannelWeight(0,1);
        Kitty.SetNumPW(0,1);
        Kitty.SetSpin(0,1);

        Kitty.SetQ1Q2(-1);
        Kitty.SetPdgId(2212, -321);
        Kitty.SetRedMass( (Mass_p*Mass_Km)/(Mass_p+Mass_Km) );
    }
    else if(TYPE==3){
        Kitty.SetNumChannels(NumChannels);
        Kitty.SetChannelWeight(0,1);
        Kitty.SetNumPW(0,1);
        Kitty.SetSpin(0,1);

        Kitty.SetQ1Q2(-1);
        Kitty.SetPdgId(2212, -321);
        Kitty.SetRedMass( (Mass_p*Mass_Km)/(Mass_p+Mass_Km) );
    }
    else if(TYPE==4){
        Kitty.SetNumChannels(NumChannels);
        Kitty.SetChannelWeight(0,1);
        Kitty.SetNumPW(0,1);
        Kitty.SetSpin(0,1);

        Kitty.SetQ1Q2(-1);
        Kitty.SetPdgId(2212, -321);
        Kitty.SetRedMass( (Mass_p*Mass_Km)/(Mass_p+Mass_Km) );
    }
    else if(TYPE==5){
        Kitty.SetNumChannels(NumChannels);
        Kitty.SetChannelWeight(0,1);
        Kitty.SetNumPW(0,1);
        Kitty.SetSpin(0,1);

        Kitty.SetQ1Q2(-1);
        Kitty.SetPdgId(2212, -321);
        Kitty.SetRedMass( (Mass_p*Mass_Km)/(Mass_p+Mass_Km) );
    }
    else{
        printf("Problem with the Haidenbauer functions!\n");
        return;
    }

    char* InputFileName = new char [128];
    strcpy(InputFileName, InputFolder);
    strcat(InputFileName, "kmpO.data");

    const unsigned NumBlankTransitionLines = 1;
    const double MinRad = 0.1;
    const double RadStep = 0.1;
    const double MaxRad = 10.0;
    NumRadBins = round((MaxRad-MinRad)/RadStep) + 1;
    bool* RadBinLoaded = new bool [NumRadBins+1];

    WaveFunctionU[0] = new complex<double>*** [NumMomBins];
    PhaseShifts[0] = new double** [NumMomBins];

    for(unsigned uMomBin=0; uMomBin<NumMomBins; uMomBin++){
        WaveFunctionU[0][uMomBin] = new complex<double>** [NumChannelsMem];
        PhaseShifts[0][uMomBin] = new double* [NumChannelsMem];
        for(unsigned usCh=0; usCh<NumChannelsMem; usCh++){
            WaveFunctionU[0][uMomBin][usCh] = new complex<double>* [NumPwPerChMem];
            PhaseShifts[0][uMomBin][usCh] = new double [NumPwPerChMem];
            for(unsigned usPw=0; usPw<NumPwPerChMem; usPw++){
                WaveFunctionU[0][uMomBin][usCh][usPw] = new complex<double> [NumRadBins];
                PhaseShifts[0][uMomBin][usCh][usPw] = 0;
            }
        }
    }

    RadBins[0] = new double [NumRadBins+1];
    for(unsigned uRad=0; uRad<=NumRadBins; uRad++){
        RadBins[0][uRad] = MinRad-RadStep*0.5 + RadStep*double(uRad);
        RadBinLoaded[uRad] = false;
    }

    FILE *InFile;
    InFile = fopen(InputFileName, "r");
    if(!InFile){
        printf("\033[1;31mERROR:\033[0m The file\033[0m %s cannot be opened!\n", InputFileName);
        return;
    }
    fseek ( InFile , 0 , SEEK_END );
    fseek ( InFile , 0 , SEEK_SET );
    char* cdummy = new char [256];
    //Read the header lines
    for(unsigned short us=0; us<NumBlankTransitionLines; us++){
        fgets(cdummy, 255, InFile);
    }
    if(feof(InFile)){
        printf("\033[1;31mERROR:\033[0m Trying to read past end of file %s\n", InputFileName);
        return;
    }
    float fRadius;
    float fMomentum;

    const unsigned NumSystems = 6;

    float* fCatsReWf = new float [NumSystems];
    float* fCatsImWf = new float [NumSystems];
    //float fPhaseShift=0;

    unsigned RadBin;
    unsigned LastRadBin;
    unsigned MomBin;

    //!---Iteration over all events---
    while(!feof(InFile)){
        fgets(cdummy, 255, InFile);
        if(strlen(cdummy)!=165) continue;
        sscanf(cdummy, " %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                &fRadius,&fMomentum,
                &fCatsReWf[0],&fCatsImWf[0],&fCatsReWf[1],&fCatsImWf[1],&fCatsReWf[2],&fCatsImWf[2],
                &fCatsReWf[3],&fCatsImWf[3],&fCatsReWf[4],&fCatsImWf[4],&fCatsReWf[5],&fCatsImWf[5]);

        LastRadBin = RadBin;
        RadBin = Kitty.GetBin(fRadius,RadBins[0],NumRadBins+1);
        //we filled up all rad bins
        if(RadBin<=LastRadBin){
            for(unsigned uRad=0; uRad<=NumRadBins; uRad++){
                RadBinLoaded[uRad]=false;
            }
        }
        if(RadBin>=NumRadBins) continue;
        if(RadBinLoaded[RadBin]){
            printf("\033[1;33mWARNING:\033[0m Trying to load RadBin #%u twice!\n", RadBin);
            continue;
        }
        MomBin = Kitty.GetMomBin(fMomentum);
        if(MomBin>=NumMomBins) continue;

        //skip momentum bins that we do not have in the CATS object
        if(fabs(fMomentum-Kitty.GetMomentum(MomBin))>0.1){
            continue;
        }
        WaveFunctionU[0][MomBin][0][0][RadBin] = (fCatsReWf[TYPE]+i*fCatsImWf[TYPE])*fRadius;
        //printf("WaveFunctionU[0][%u][0][0][%u] = %.2e, %.2e\n", MomBin, RadBin, fCatsReWf[TYPE]*fRadius, fCatsImWf[TYPE]*fRadius);
        RadBinLoaded[RadBin] = true;
    }
    delete [] cdummy;
    fclose(InFile);


    delete [] MomBins;
    delete [] InputFileName;
    delete [] RadBinLoaded;
    delete [] fCatsReWf;
    delete [] fCatsImWf;
}

void CleanHaidenbauer(CATS& Kitty, complex<double>***** WaveFunctionU, double**** PhaseShifts, double** RadBins){
    unsigned NumMomBins = Kitty.GetNumMomBins();
    const unsigned short NumChannels = 4;
    const unsigned short NumPwPerCh = 2;
    //const unsigned short NumFiles = 6;

    for(unsigned uMomBin=0; uMomBin<NumMomBins; uMomBin++){
        for(unsigned usCh=0; usCh<NumChannels; usCh++){
            for(unsigned usPw=0; usPw<NumPwPerCh; usPw++){
                delete [] WaveFunctionU[0][uMomBin][usCh][usPw];
            }
            delete [] WaveFunctionU[0][uMomBin][usCh];
            delete [] PhaseShifts[0][uMomBin][usCh];
        }
        delete [] WaveFunctionU[0][uMomBin];
        delete [] PhaseShifts[0][uMomBin];
    }

    delete [] WaveFunctionU[0];
    delete [] PhaseShifts[0];
    delete [] RadBins[0];
}












