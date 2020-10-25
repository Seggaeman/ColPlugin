// ColPlugin.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ColPlugin.h"

extern ::ColPlugin lePlugin;
extern ::HMODULE hModuleInstance;
//extern is superfluous when prepended to global functions
extern INT_PTR CALLBACK ToolDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
//plugin class implementation
//===========================

ColPlugin::ColPlugin() : LboxSelection(NULL), theWorker(NULL), ::UtilityObj() 
{

}

ColPlugin::~ColPlugin()
{

}

void ColPlugin::BeginEditParams(Interface* ip, IUtil* iu)
{
	this->ColIP= ip; this->ColIU= iu;
	this->hRollup= CreateDialogParam(hModuleInstance, MAKEINTRESOURCE(IDD_DIALOG1), ip->GetMAXHWnd(), ToolDlgProc, (LPARAM)this);
	
	::IRollupWindow* leWindow= ::GetIRollup(GetDlgItem(this->hRollup, IDC_CUSTOM1));
	leWindow->SetBorderless(TRUE);
	HRSRC hRsrc= FindResource(hModuleInstance, MAKEINTRESOURCE(IDD_DIALOG2), MAKEINTRESOURCE(RT_DIALOG));
	HGLOBAL hTemplate= LoadResource(hModuleInstance, hRsrc);
	DLGTEMPLATE* pTemplate= (DLGTEMPLATE*)LockResource(hTemplate);
	int RollupID= leWindow->AppendRollup(hModuleInstance, pTemplate, ToolDlgProc, "Menue");
	leWindow->Show();
	UnlockResource(pTemplate);
	FreeResource(hTemplate);
	/*
	::GeomObject* theBall= (GeomObject*)ip->CreateInstance(SClass_ID(GEOMOBJECT_CLASS_ID), Class_ID(SPHERE_CLASS_ID, 0));
	theBall->GetParamBlock()->SetValue(0, this->ColIP->GetTime(), 1.34f);
	INode* theNode= this->ColIP->CreateObjectNode(theBall);
	theNode->SetName("laBalle");
	Mtl* theMaterial= (Mtl*)this->ColIP->CreateInstance(MATERIAL_CLASS_ID, GTA_MultiMat);
	theMaterial->SetName("testMaterial");
	::MultiMtl* theMulti= (MultiMtl*) theMaterial->GetReference(0);
	theMulti->SetNumSubMtls(5);
	theNode->SetMtl(theMaterial);
	this->ColIU->CloseUtility();
	*/
}

void ColPlugin::EndEditParams(Interface* ip, IUtil* iu)
{
	DestroyWindow(this->hRollup);
	this->ColIP->DeleteRollupPage(this->hRollup);
}

void ColPlugin::SelectionSetChanged(Interface* ip, IUtil* iu)
{

}

void ColPlugin::DeleteThis()
{
}

ClassDesc2* ColPlugin::GetClassDesc()
{
	return ColClassDesc::GetClassDescInstance();
}

//implementation
//==============
void ColPlugin::PopulateListbox()
{
	//save current address
	std::streampos CurrentAddress= this->iColStream.tellg();
	//delete saved addresses and clear the listbox
	this->collisionAddrs.clear();

	//LRESULT lastItemIdx= SendMessage(GetDlgItem(hRollup, IDC_COLLIST), LB_GETCOUNT, NULL, NULL);
	//while ((lastItemIdx= SendMessage(GetDlgItem(hRollup, IDC_COLLIST), LB_DELETESTRING, (WPARAM)--lastItemIdx, NULL)) != LB_ERR);

	//std::fstream logFile("c:\\users\\packard-bell\\desktop\\logFile.txt", std::ios::out | std::ios::beg | std::ios::trunc);
	while (true)
	{
		extras::ColHeader1_t theHeader;
		iColStream >> theHeader;
		if (this->iColStream.eof() || (! theHeader.CheckHeader()))
			break;

		auto itr= this->collisionAddrs.insert(std::make_pair(this->collisionAddrs.size(),std::make_tuple((DWORD)iColStream.tellg()-sizeof(extras::ColHeader1_t), (DWORD)iColStream.tellg()+theHeader.FileSizeMinus8-65, theHeader.FourCC)));
		iColStream.seekg(theHeader.FileSizeMinus8-64, std::ios::cur);

		//logFile << std::get<0>(itr.first->second) << ", " << std::get<1>(itr.first->second) << ", " << std::get<1>(itr.first->second)- std::get<0>(itr.first->second)+1 << std::endl;
		std::string LBoxString= theHeader.CollisionName;
		LBoxString += " ("; LBoxString += theHeader.FourCCAsString; LBoxString += ")";
		SendMessage(GetDlgItem(hRollup, IDC_COLLIST), LB_ADDSTRING, NULL, (LPARAM)LBoxString.c_str());
	}
	//go back to start
	iColStream.clear();		//reset error flags. This is important, reads will fail if the filestream is in an error state.
	iColStream.seekg(CurrentAddress, std::ios::beg);
}

void ColPlugin::ImportCollisions(int LboxIdx)
{
	switch(std::get<2>(this->collisionAddrs[LboxIdx]))
	{
		case extras::ColIDs::COLL:
			this->theWorker = new ColWorkers::ColWorker1(this, std::get<0>(this->collisionAddrs[LboxIdx]));
		break;

		case extras::ColIDs::COL2:
			this->theWorker= new ColWorkers::ColWorker2(this, std::get<0>(this->collisionAddrs[LboxIdx]));
		break;

		case extras::ColIDs::COL3:
			this->theWorker= new ColWorkers::ColWorker3(this, std::get<0>(this->collisionAddrs[LboxIdx]));
		break;

		default:
			//MessageBox(NULL, "Problem!", "Caption", MB_OK);
		return;
	}
	

	theWorker->CreateRootDummy();
	theWorker->DrawBounds();
	theWorker->ReadSpheres();
	theWorker->ReadBoxes();
	theWorker->AssignBoxNSphereMats();
	theWorker->ReadColMesh();
	theWorker->ReadShadowMesh();
	theWorker->AttachNodes();

	delete this->theWorker;
	this->theWorker= NULL;
}

bool ColPlugin::SeekDFFCollision()
{
	union
	{
		extras::GTAHeader Clump;
		extras::GTAHeader Struct;
		extras::GTAHeader FrameList;
		extras::GTAHeader GeometryList;
		extras::GTAHeader AtomicHeader;
		extras::GTAHeader LightHeader;
		extras::GTAHeader CameraHeader;
		extras::GTAHeader Extension;
		extras::GTAHeader CollisionHeader;
	};

	this->iColStream >> Clump;
	this->iColStream >> Struct;
	if (Struct.fileVer != extras::FileVersions::GTA_SA)
		return false;
	//get geometry, light and camera count
	DWORD geometryCount, lightCount, cameraCount;
	this->iColStream >> geometryCount >> lightCount >> cameraCount;
	//skip frame list
	this->iColStream >> FrameList;
	this->iColStream.seekg(FrameList.secSize, std::ios::cur);
	//skip geometry list
	this->iColStream >> GeometryList;
	this->iColStream.seekg(GeometryList.secSize, std::ios::cur);
	//skip atomic section
	for (int i=0; i < geometryCount; ++i)
	{
		this->iColStream >> AtomicHeader;
		this->iColStream.seekg(AtomicHeader.secSize, std::ios::cur);
	}
	//skip lights
	for (int i= 0; i < lightCount; ++i)
	{
		//skip struct
		this->iColStream >> Struct;
		this->iColStream.seekg(Struct.secSize, std::ios::cur);
		this->iColStream >> LightHeader;
		this->iColStream.seekg(LightHeader.secSize, std::ios::cur);
	}
	//skip cameras
	for (int i= 0; i < cameraCount; ++i)
	{
		//skip struct
		this->iColStream >> Struct;
		this->iColStream.seekg(Struct.secSize, std::ios::cur);
		this->iColStream >> CameraHeader;
		this->iColStream.seekg(CameraHeader.secSize, std::ios::cur);
	}
	//skip extension
	this->iColStream >> Extension;
	this->iColStream >> CollisionHeader;
	return true;
}

void ColPlugin::DeleteCollisions()
{
	union {
		LRESULT itemCount;
		LRESULT selCount;
	};
	//itemCount= SendMessage(GetDlgItem(this->hRollup, IDC_COLLIST), LB_GETCOUNT, NULL, NULL);

	//if (! itemCount)	//return if listbox is empty
		//return;

	selCount= SendMessage(GetDlgItem(this->hRollup, IDC_COLLIST), LB_GETSELCOUNT, NULL, NULL);

	if (! selCount)	//return if nothing is selected
		return;

	this->LboxSelection= new int[selCount];
	SendMessage(GetDlgItem(this->hRollup, IDC_COLLIST), LB_GETSELITEMS, (WPARAM)selCount, (LPARAM)this->LboxSelection);
	
	//create a temporary file
	std::string tempFileName= ::IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_TEMP_DIR);
	//std::string tempFileName= "C:\\Users\\Packard-Bell\\Desktop\\";
	if (*tempFileName.rbegin() != '\\')
		tempFileName += '\\';

	tempFileName += "TempFile.txt";
	this->oColStream.close();
	this->oColStream.open(tempFileName.c_str(), std::ios::out | std::ios::binary | std::ios::trunc | std::ios::beg);
	
	//MessageBox(NULL, extras::ToString<std::size_t>(this->collisionAddrs.size()).c_str(), "number of addresses", MB_OK);
	for (int i= 0, k= 0; i < this->collisionAddrs.size(); ++i)
	{
		//MessageBox(NULL, extras::ToString<int>(this->LboxSelection[k]).c_str(), extras::ToString<int>(k).c_str(), MB_OK);
		if (k < selCount && (this->LboxSelection[k] == i))
		{
			++k;
			continue;
		}

		this->iColStream.seekg(std::get<0>(this->collisionAddrs[i]), std::ios::beg);
		DWORD colClumpSize= std::get<1>(this->collisionAddrs[i])- std::get<0>(this->collisionAddrs[i])+1;
		//MessageBox(NULL, extras::ToString<DWORD>(colClumpSize).c_str(), "Clump size", MB_OK);
		char* buffer= new char[colClumpSize];

		this->iColStream.read(buffer, colClumpSize);
		this->oColStream.write(buffer, colClumpSize);

		delete[] buffer;
	}
	delete[] this->LboxSelection;
	this->iColStream.close();
	this->oColStream.close();

	remove(this->szFileName);
	rename(tempFileName.c_str(), this->szFileName);
	this->iColStream.open(this->szFileName, std::ios::in | std::ios::binary | std::ios::beg);

	//delete strings
	selCount= SendMessage(GetDlgItem(this->hRollup, IDC_COLLIST), LB_GETCOUNT, NULL, NULL);
	while ((selCount= SendMessage(GetDlgItem(this->hRollup, IDC_COLLIST), LB_DELETESTRING, (WPARAM)--selCount, NULL)) != LB_ERR);

	this->PopulateListbox();
}