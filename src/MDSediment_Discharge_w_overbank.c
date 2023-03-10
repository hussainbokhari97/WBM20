/******************************************************************************
WBMsed V2.0
MDDischarge_w_overbank.c
Add an overbank water loss and storage.

sagy.cohen@colorado.edu
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <cm.h>
#include <MF.h>
#include <MD.h>

// Input
static int _MDInDischLevel1ID   = MFUnset;
static int _MDInBankfullQID 	= MFUnset;
static int _MDInBankfull_QnID  	= MFUnset;
static int _MDInFlowCoefficientID= MFUnset;

// Output
static int _MDOutDischargeID     = MFUnset;
static int _MDOutOverBankQID     = MFUnset;
static int _MDOutPCQdifferenceID = MFUnset;

static void _MDDischargeBF (int itemID) {
	float discharge,Qbf,FlowCoeff,QfromFP; 
	float ExcessQ = 0;
	float Qfp = 0;
	int Q_n;
	float percentDiff = 1;
	FlowCoeff = MFVarGetFloat (_MDInFlowCoefficientID, itemID, 0.0);; // coefficient for water flow from floodplain to river 
	
	discharge = MFVarGetFloat (_MDInDischLevel1ID,   itemID, 0.0);
 	if (discharge < 0) discharge = 0;
 	
	Q_n = MFVarGetFloat (_MDInBankfull_QnID, itemID, 0.0);
	
	Qbf  = MFVarGetFloat (_MDInBankfullQID, itemID, 0.0); 
	Qbf = Qbf * 0.4; //BANKFULL DISCHARGE ADJUSTMENT !!!!!!!!

	if (Q_n == 0) Qbf  = 1000000; 
	
	Qfp = MFVarGetFloat (_MDOutOverBankQID,   itemID, 0.0); //excess water stored in floodplain the day before
	
	if (discharge > Qbf){
		ExcessQ = Qfp + (discharge - Qbf);
		discharge = Qbf;
	}	
	else {
		QfromFP = (FlowCoeff * (Qbf - discharge));	
		if (QfromFP > Qfp) QfromFP = Qfp;	
		ExcessQ = Qfp - QfromFP;
		if (ExcessQ < 0) ExcessQ =0;
		if (QfromFP > 0) percentDiff = (discharge/(QfromFP+discharge)); // calculate the percent difference for sediment adjustment
		discharge = discharge + QfromFP;
		if (discharge > Qbf) discharge = Qbf;	
	}

	MFVarSetFloat (_MDOutPCQdifferenceID, itemID, percentDiff);
	MFVarSetFloat (_MDOutOverBankQID, itemID, ExcessQ);
	MFVarSetFloat (_MDOutDischargeID, itemID, discharge);
}

int MDSediment_DischargeBFDef () {
	int optID = MFinput;
	const char *optStr;

	if (_MDOutDischargeID != MFUnset) return (_MDOutDischargeID);

	MFDefEntering ("DischargeBF");
	if ((optStr = MFOptionGet (MDVarRouting_Discharge)) != (char *) NULL) optID = CMoptLookup (MFsourceOptions,optStr,true);
	switch (optID) {
		default:
		case MFhelp:  MFOptionMessage (MDVarRouting_Discharge, optStr, MFsourceOptions); return (CMfailed);
		case MFinput: _MDOutDischargeID = MFVarGetID (MDVarRouting_Discharge,         "m3/s",   MFInput,  MFState, MFInitial); break;
		case MFcalculate:
			if (((_MDInDischLevel1ID     = MDRouting_DischargeUptakeDef ())  == CMfailed) ||
				((_MDInBankfullQID       = MDRouting_BankfullQcalcDef ())    == CMfailed) ||
				((_MDInBankfull_QnID     = MFVarGetID (MDVarRouting_BankfullQ50,      "m3/s",   MFInput,  MFState, MFBoundary)) == CMfailed) ||
				((_MDInFlowCoefficientID = MFVarGetID (MDVarSediment_FlowCoefficient, "m3/s",   MFInput,  MFState, MFBoundary)) == CMfailed) ||
				((_MDOutOverBankQID      = MFVarGetID (MDVarSediment_OverBankQ,       "m3/s",   MFOutput, MFState, MFInitial))  == CMfailed) ||
				((_MDOutPCQdifferenceID  = MFVarGetID (MDVarSediment_PCQdifference,   MFNoUnit, MFOutput, MFState, MFInitial))  == CMfailed) ||
				((_MDOutDischargeID      = MFVarGetID (MDVarRouting_Discharge,        "m3/s",   MFRoute,  MFState, MFBoundary)) == CMfailed) ||
				(MFModelAddFunction (_MDDischargeBF) == CMfailed)) return (CMfailed);
			break;
	}
	MFDefLeaving  ("DischargeBF");
	return (_MDOutDischargeID);
}
