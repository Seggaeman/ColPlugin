#include "stdafx.h"
#include "ColPlugin.h"


//Base
ColWorkers::ColWorker::ColWorker(ColPlugin* input, DWORD BAddress) : ColPtr(input), BaseAddress(BAddress), colMeshNode(NULL)
{
	this->ColPtr->iColStream.seekg(this->BaseAddress, std::ios::beg);
}



//COLL
//=======
ColWorkers::ColWorker1::ColWorker1(ColPlugin* input, DWORD BAddress) : ColWorker(input, BAddress)
{
	this->ColPtr->iColStream >> this->theHeader1;
	this->ColPtr->ColIP->ProgressUpdate(this->ColPtr->PercentageProgress, FALSE, this->theHeader1.CollisionName);
}

void ColWorkers::ColWorker1::CreateRootDummy()
{
	//empty node collection.
	this->colNodes.clear();
	::DummyObject* theDummy= (DummyObject*)this->ColPtr->ColIP->CreateInstance(SClass_ID(HELPER_CLASS_ID), Class_ID(DUMMY_CLASS_ID,0));
	INode* theNode= this->ColPtr->ColIP->CreateObjectNode(theDummy);
	theNode->SetName(theHeader1.CollisionName);
	this->colNodes.push_back(theNode);
}

void ColWorkers::ColWorker1::DrawBounds()
{
	//verify check box
	if (SendMessage(GetDlgItem(this->ColPtr->hRollup, IDC_BOUNDING), BM_GETCHECK, NULL, NULL) == BST_UNCHECKED)
		return;

	::GeomObject* theSphere= (GeomObject*)this->ColPtr->ColIP->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(SPHERE_CLASS_ID, 0));
	theSphere->GetParamBlock()->SetValue(0, this->ColPtr->ColIP->GetTime(), this->theHeader1.boundsVer1.BSphereRad);	//radius
	::GeomObject* theBox= (GeomObject*)this->ColPtr->ColIP->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(BOXOBJ_CLASS_ID,0));
	theBox->GetParamBlock()->SetValue(0, this->ColPtr->ColIP->GetTime(), this->theHeader1.boundsVer1.BBoxMax[1]-this->theHeader1.boundsVer1.BBoxMin[1]);	//length
	theBox->GetParamBlock()->SetValue(1, this->ColPtr->ColIP->GetTime(), this->theHeader1.boundsVer1.BBoxMax[0]-this->theHeader1.boundsVer1.BBoxMin[0]);
	theBox->GetParamBlock()->SetValue(2, this->ColPtr->ColIP->GetTime(), this->theHeader1.boundsVer1.BBoxMax[2]-this->theHeader1.boundsVer1.BBoxMin[2]);
	INode* SphereNode= this->ColPtr->ColIP->CreateObjectNode(theSphere);
	INode* BoxNode= this->ColPtr->ColIP->CreateObjectNode(theBox);
	//make transparent
	SphereNode->XRayMtl(TRUE); BoxNode->XRayMtl(TRUE);
	//set names
	std::string boundsName= this->theHeader1.CollisionName; boundsName += "_BoundSphere";
	SphereNode->SetName(const_cast<char*>(boundsName.c_str()));
	boundsName= this->theHeader1.CollisionName; boundsName += "_BoundBox";
	BoxNode->SetName(const_cast<char*>(boundsName.c_str()));
	//set positions
	Matrix3 temp(::Point3(1,0,0), ::Point3(0,1,0), Point3(0,0,1), Point3(this->theHeader1.boundsVer1.BCenter[0], this->theHeader1.boundsVer1.BCenter[1], this->theHeader1.boundsVer1.BCenter[2]));
	SphereNode->SetNodeTM(this->ColPtr->ColIP->GetTime(), temp);
	//adjust z position because box pivot is at the bottom by default (not in the center)
	temp.SetRow(3, Point3(temp.GetRow(3).x, temp.GetRow(3).y, temp.GetRow(3).z-0.5f*(this->theHeader1.boundsVer1.BBoxMax[2]-this->theHeader1.boundsVer1.BBoxMin[2])));
	BoxNode->SetNodeTM(this->ColPtr->ColIP->GetTime(), temp);
	this->colNodes.push_back(SphereNode); this->colNodes.push_back(BoxNode);
}

void ColWorkers::ColWorker1::AttachNodes()
{
	for (std::vector<INode*>::iterator itr= this->colNodes.begin() + 1; itr < this->colNodes.end(); ++ itr)
		(*this->colNodes.begin())->AttachChild(*itr);


	if (SendMessage(GetDlgItem(this->ColPtr->hRollup, IDC_OPTGTAMAT), BM_GETCHECK, NULL, NULL) == BST_CHECKED)
	{
		//make a mesh out of the first box and then delete the entry
		if (this->colMeshNode == NULL)
		{
			if (this->GTABoxNSpheres.size() > 0)
			{
				auto itr1= this->GTABoxNSpheres.begin();
				auto itr2= itr1->second.begin();
				auto itr3= std::find(this->GTAsurfaceIDs.begin(), this->GTAsurfaceIDs.end(), itr1->first);

				this->colMeshNode= *itr2;

				//move to origin
				::Matrix3 orgTransform= this->colMeshNode->GetNodeTM(this->ColPtr->ColIP->GetTime());
				//orgTransform.Invert();
				this->colMeshNode->SetNodeTM(this->ColPtr->ColIP->GetTime(), Matrix3(TRUE));
				this->colMeshNode->SetName(const_cast<char*>((this->theHeader1.CollisionName + std::string("_Collision")).c_str()));
				
				//convert the node
				this->ColPtr->ColIP->ConvertNode(this->colMeshNode, Class_ID(EDITTRIOBJ_CLASS_ID,0));
				
				//set material IDs and adjust vertex coordinates so that geometry keeps it's original position relative to pivot
				BOOL deleteIt;
				TriObject* colMesh= extras::GetObjectFromNode<TriObject*>(this->colMeshNode, this->ColPtr->ColIP->GetTime(), EDITTRIOBJ_CLASS_ID, deleteIt);

				for (int i= 0; i < colMesh->mesh.numFaces; ++i)
					colMesh->mesh.faces[i].setMatID(std::distance(this->GTAsurfaceIDs.begin(), itr3));
				
				for (int i= 0; i< colMesh->mesh.numVerts; ++i)
					colMesh->mesh.setVert(i, colMesh->mesh.verts[i]*orgTransform);

				if (deleteIt)
					colMesh->DeleteThis();

				//add this node's material to the list of surface IDs
				this->GTAsurfaceIDs.push_back(itr1->first);

				//delete the node from the list (so that it doesn't attach to itself)
				itr1->second.erase(itr2);

				//delete the entire entry if node set is empty
				if (itr1->second.size() == 0)
					this->GTABoxNSpheres.erase(itr1);
			}
			else
				return;
		}

		//retrieve the collision mesh
		BOOL deleteColMesh;
		TriObject* colMesh= extras::GetObjectFromNode<TriObject*>(this->colMeshNode, this->ColPtr->ColIP->GetTime(), EDITTRIOBJ_CLASS_ID, deleteColMesh);
		//build collision mesh inverted transform matrix
		Matrix3 leMatrix(TRUE);
		leMatrix.PreTranslate(this->colMeshNode->GetObjOffsetPos());
		::PreRotateMatrix(leMatrix, this->colMeshNode->GetObjOffsetRot());
		::ApplyScaling(leMatrix, this->colMeshNode->GetObjOffsetScale());
		leMatrix= this->colMeshNode->GetNodeTM(this->ColPtr->ColIP->GetTime())*leMatrix;
		leMatrix.Invert();

		//add additional surfaceIDs
		for (auto itr = this->GTABoxNSpheres.begin(); itr != this->GTABoxNSpheres.end(); ++ itr)
		{
			//MessageBox(NULL, "You are here", "Caption", MB_OK);
			auto itr3 = std::find(this->GTAsurfaceIDs.begin(), this->GTAsurfaceIDs.end(), itr->first);

			for (auto itr2= itr->second.begin(); itr2 != itr->second.end(); ++itr2)
			{
				//probably not necessary to convert the box/sphere
				//this->ColPtr->ColIP->ConvertNode((*itr2), Class_ID(EDITTRIOBJ_CLASS_ID,0));
				BOOL deleteIt= FALSE;
				TriObject* theTri= extras::GetObjectFromNode<TriObject*>((*itr2), this->ColPtr->ColIP->GetTime(), EDITTRIOBJ_CLASS_ID, deleteIt);

				//set material ID
				for (int i= 0; i < theTri->mesh.numFaces; ++i)
					theTri->mesh.faces[i].setMatID(std::distance(this->GTAsurfaceIDs.begin(), itr3));

				::MeshDelta leDelta;
				leDelta.AttachMesh(colMesh->mesh, theTri->mesh, leMatrix*(*itr2)->GetNodeTM(this->ColPtr->ColIP->GetTime()), 0);
				leDelta.Apply(colMesh->mesh);
				//MessageBox(NULL, "you are here", "Caption", MB_OK);
				if (deleteIt)
					theTri->DeleteThis();

				(*itr2)->Delete(this->ColPtr->ColIP->GetTime(), TRUE);
			}
			if (itr3 == this->GTAsurfaceIDs.end())
				this->GTAsurfaceIDs.push_back(itr->first);
		}
		//shouldn't be deleted in general
		if (deleteColMesh)
		{
			//MessageBox(NULL, "dELETING MESH", "Caption", MB_OK);
			colMesh->DeleteThis();
		}
		//create multimaterial
		
		::MultiMtl* multiMat= (MultiMtl*)this->ColPtr->ColIP->CreateInstance(SClass_ID(MATERIAL_CLASS_ID), Class_ID(MULTI_CLASS_ID, 0));

		multiMat->SetNumSubMtls(this->GTAsurfaceIDs.size());
		for (int i = 0; i < this->GTAsurfaceIDs.size(); ++i)
		{
			multiMat->SetSubMtl(i, this->GenGTAMaterial(this->GTAsurfaceIDs[i]));
		}
		//apply it to collision mesh
		this->colMeshNode->SetMtl(multiMat);
		
		//update
		//colMesh->mesh.InvalidateGeomCache();
		//colMesh->mesh.InvalidateTopologyCache();
		//colMesh->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		//colMesh->NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);
		//colMeshNode->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		//colMeshNode->NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);
	}
}

void ColWorkers::ColWorker1::ReadSpheres()
{
	//get sphere count
	UINT numSpheres;
	this->ColPtr->iColStream >> numSpheres;
	for (int i= 0; i < numSpheres; ++i)
	{
		extras::TSphereV1 laBalle;
		this->ColPtr->iColStream >> laBalle;
		laBalle.surface.material= (laBalle.surface.material > 178)? 178 : laBalle.surface.material;
		GeomObject* theBall= (::GeomObject*)this->ColPtr->ColIP->CreateInstance(SClass_ID(GEOMOBJECT_CLASS_ID), Class_ID(SPHERE_CLASS_ID,0));
		theBall->GetParamBlock()->SetValue(0, this->ColPtr->ColIP->GetTime(), (float)laBalle.radius);
		theBall->GetParamBlock()->SetValue(1, this->ColPtr->ColIP->GetTime(), 16);
		INode* ballNode= this->ColPtr->ColIP->CreateObjectNode(theBall);
		Matrix3 temp(Point3(1,0,0), Point3(0,1,0), Point3(0,0,1), Point3(laBalle.center));
		ballNode->SetNodeTM(this->ColPtr->ColIP->GetTime(), temp);
		this->BoxNSpheres[laBalle.surfaceAsUINT].insert(ballNode);
		this->GTABoxNSpheres[laBalle.surface.material].insert(ballNode);
		this->colNodes.push_back(ballNode);
	}
}

void ColWorkers::ColWorker1::ReadBoxes()
{
	//skip unknown
	this->ColPtr->iColStream.seekg(4, std::ios::cur);
	UINT numBoxes;
	this->ColPtr->iColStream >> numBoxes;
	for (int i= 0; i < numBoxes; ++i)
	{
		extras::TBox laBoite;
		this->ColPtr->iColStream >> laBoite;
		laBoite.surface.material = (laBoite.surface.material) > 178 ? 178 : laBoite.surface.material;
		GeomObject* theBox= (::GeomObject*)this->ColPtr->ColIP->CreateInstance(SClass_ID(GEOMOBJECT_CLASS_ID), Class_ID(BOXOBJ_CLASS_ID,0));

		//#defines are wrong
		theBox->GetParamBlock()->SetValue(0, this->ColPtr->ColIP->GetTime(), laBoite.max[1]-laBoite.min[1]);
		theBox->GetParamBlock()->SetValue(1, this->ColPtr->ColIP->GetTime(), laBoite.max[0]-laBoite.min[0]);
		theBox->GetParamBlock()->SetValue(2, this->ColPtr->ColIP->GetTime(), laBoite.max[2]-laBoite.min[2]);
		Matrix3 temp(TRUE);
		temp.SetRow(3, Point3(laBoite.min[0]+0.5f*(laBoite.max[0]-laBoite.min[0]), laBoite.min[1]+0.5f*(laBoite.max[1]-laBoite.min[1]), laBoite.min[2]));
		INode* boxNode= this->ColPtr->ColIP->CreateObjectNode(theBox);
		boxNode->SetNodeTM(this->ColPtr->ColIP->GetTime(), temp);
		this->BoxNSpheres[laBoite.surfaceAsUINT].insert(boxNode);
		this->GTABoxNSpheres[laBoite.surface.material].insert(boxNode);
		this->colNodes.push_back(boxNode);
	}
}

void ColWorkers::ColWorker1::AssignBoxNSphereMats()
{
	if (SendMessage(GetDlgItem(this->ColPtr->hRollup, IDC_OPTGTAMAT), BM_GETCHECK, NULL, NULL) == BST_UNCHECKED)
	{
		for (auto itr= this->BoxNSpheres.begin(); itr != this->BoxNSpheres.end(); ++itr)
		{
			extras::TSurfaceV1* leSurface= reinterpret_cast<extras::TSurfaceV1*>(const_cast<UINT*>(&(itr->first)));
			Mtl* leMat;
		
			{
				leMat= (Mtl*)this->ColPtr->ColIP->CreateInstance(SClass_ID(MATERIAL_CLASS_ID), GTA_COLSurface);
				//set material, flag, brightness and light
				leMat->GetParamBlock(0)->SetValue(0, this->ColPtr->ColIP->GetTime(), leSurface->material);
				if (leSurface->flag == 255)
					leMat->GetParamBlock(0)->SetValue(1, this->ColPtr->ColIP->GetTime(), -1);
				else
					leMat->GetParamBlock(0)->SetValue(1, this->ColPtr->ColIP->GetTime(), leSurface->flag);
				leMat->GetParamBlock(0)->SetValue(2, this->ColPtr->ColIP->GetTime(), leSurface->brightness);
				leMat->GetParamBlock(0)->SetValue(3, this->ColPtr->ColIP->GetTime(), leSurface->light);
			}

			//assign the material
			for (std::unordered_set<INode*>::iterator itr2= itr->second.begin(); itr2 != itr->second.end(); ++itr2)
			{
				(*itr2)->SetMtl(leMat);
				if (leSurface->flag < 18)
					(*itr2)->SetName(extras::partNames[leSurface->flag]);
				else if (leSurface->flag == 255)
					(*itr2)->SetName("Special");
				else
					(*itr2)->SetName("Unknown parts");
			}
		}
	}
}
void ColWorkers::ColWorker1::ReadColMesh()
{
	union {
		UINT faceCount;
		UINT vertexCount;
	};

	this->ColPtr->iColStream >> vertexCount;

	if (!vertexCount)
		return;

	//create triobject
	TriObject* colMesh= (TriObject*)this->ColPtr->ColIP->CreateInstance(SClass_ID(GEOMOBJECT_CLASS_ID), Class_ID(EDITTRIOBJ_CLASS_ID, 0));
	colMesh->mesh.setNumVerts(vertexCount);
	
	//read vertices
	for (int i= 0; i < colMesh->mesh.numVerts; ++i)
	{
		float vtx[3];
		this->ColPtr->iColStream >> vtx;
		colMesh->mesh.setVert(i, vtx[0], vtx[1], vtx[2]);
	}

	//read faces
	//get materials, set faces and assign IDs
	this->ColPtr->iColStream >> faceCount;
	colMesh->mesh.setNumFaces(faceCount);

	for (int i= 0; i < colMesh->mesh.numFaces; ++i)
	{
		extras::TFaceV1 theFace;
		this->ColPtr->iColStream >> theFace;
		theFace.surface.material = (theFace.surface.material > 178)? 178 : theFace.surface.material;

		for (int j= 0; j < 3; ++j)
			colMesh->mesh.faces[i].v[j]= theFace[j];

		colMesh->mesh.FlipNormal(i);
		colMesh->mesh.faces[i].setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_VIS);
		
		if (SendMessage(GetDlgItem(this->ColPtr->hRollup, IDC_OPTGTAMAT), BM_GETCHECK, NULL, NULL) == BST_UNCHECKED)
		{
			std::vector<UINT>::iterator itr= std::find(this->ColMeshMatsV1.begin(), this->ColMeshMatsV1.end(), theFace.surfaceCompressed);
			colMesh->mesh.faces[i].setMatID(std::distance(this->ColMeshMatsV1.begin(),itr));
			if (itr == this->ColMeshMatsV1.end())
				this->ColMeshMatsV1.push_back(theFace.surfaceCompressed);
		}
		else
		{
			auto itr= std::find(this->GTAsurfaceIDs.begin(), this->GTAsurfaceIDs.end(), theFace.surface.material);
			colMesh->mesh.faces[i].setMatID(std::distance(this->GTAsurfaceIDs.begin(), itr));
			if (itr == this->GTAsurfaceIDs.end())
				this->GTAsurfaceIDs.push_back(theFace.surface.material);
		}
	}
	
	//create node
	this->colMeshNode= this->ColPtr->ColIP->CreateObjectNode(colMesh);
	colMeshNode->SetNodeTM(this->ColPtr->ColIP->GetTime(), Matrix3(TRUE));	//set node transform
	std::string colNodeName= this->theHeader1.CollisionName; colNodeName += "_Collision";
	colMeshNode->SetName(const_cast<char*>(colNodeName.c_str()));
	this->colNodes.push_back(colMeshNode);

	//apply materials

	if (SendMessage(GetDlgItem(this->ColPtr->hRollup, IDC_OPTGTAMAT), BM_GETCHECK, NULL, NULL) == BST_UNCHECKED)
	{
		::MultiMtl* multiMat= (::MultiMtl*)this->ColPtr->ColIP->CreateInstance(SClass_ID(MATERIAL_CLASS_ID), Class_ID(MULTI_CLASS_ID, 0));
		::Mtl* theMat;
		multiMat->SetNumSubMtls(this->ColMeshMatsV1.size());
	
		for (std::vector<UINT>::iterator itr = this->ColMeshMatsV1.begin(); itr < this->ColMeshMatsV1.end(); ++itr)
		{
			//set surface and parts

			extras::TSurfaceV1* leSurface= (extras::TSurfaceV1*)&(*itr);
			//already changed this above
			//leSurface->material= (leSurface->material > 178)? 178 : leSurface->material;
			theMat= (Mtl*)this->ColPtr->ColIP->CreateInstance(SClass_ID(MATERIAL_CLASS_ID), GTA_COLSurface);
			theMat->GetParamBlock(0)->SetValue(0, this->ColPtr->ColIP->GetTime(), leSurface->material);
			if (leSurface->light == 255)
				theMat->GetParamBlock(0)->SetValue(1, this->ColPtr->ColIP->GetTime(), -1);
			else
				theMat->GetParamBlock(0)->SetValue(1, this->ColPtr->ColIP->GetTime(), (BYTE)leSurface->light);

			theMat->GetParamBlock(0)->SetValue(2, this->ColPtr->ColIP->GetTime(), leSurface->brightness);
			theMat->GetParamBlock(0)->SetValue(3, this->ColPtr->ColIP->GetTime(), leSurface->light);

			//add this material
			multiMat->SetSubMtl(std::distance(this->ColMeshMatsV1.begin(), itr), theMat);
		}

		colMeshNode->SetMtl(multiMat);
	}
}

Mtl* ColWorkers::ColWorker1::GenGTAMaterial(BYTE surfaceID)
{
	::Mtl* leGTAMat= (::Mtl*)this->ColPtr->ColIP->CreateInstance(SClass_ID(MATERIAL_CLASS_ID), GTA_Material);
	::BitmapTex* theTex= (BitmapTex*)this->ColPtr->ColIP->CreateInstance(SClass_ID(TEXMAP_CLASS_ID), Class_ID(BMTEX_CLASS_ID, 0));
	theTex->SetMapName(this->GetTextureName(surfaceID).c_str());
	leGTAMat->GetParamBlock(0)->SetValue(4, this->ColPtr->ColIP->GetTime(), theTex);
	return leGTAMat;
}

std::string ColWorkers::ColWorker1::GetTextureName(BYTE surfaceID)
{
	char* fileExt[]= {".TGA", ".DDS", ".PNG", ".GIF", ".BMP", ".JPG"};
	char longFileName[MAX_PATH];
	std::string shortFileName;
	for (int i= 0; i < 6; ++i)
	{
		shortFileName= extras::surfaceNames[surfaceID]; shortFileName += fileExt[i];

		if (::BMMGetFullFilename(shortFileName.c_str(), longFileName))
			return std::string(longFileName);
	}

	shortFileName.erase(shortFileName.length()-4);
	shortFileName += fileExt[0];
	return shortFileName;
}

void ColWorkers::ColWorker1::ReadShadowMesh()
{

}

//COL2
//==========
ColWorkers::ColWorker2::ColWorker2(ColPlugin* input, DWORD BAddress) : ColWorker1(input, BAddress)
{
	this->ColPtr->iColStream >> this->theHeader23;

}


void ColWorkers::ColWorker2::DrawBounds()
{
	//verify check box
	if (SendMessage(GetDlgItem(this->ColPtr->hRollup, IDC_BOUNDING), BM_GETCHECK, NULL, NULL) == BST_UNCHECKED)
		return;

	::GeomObject* theSphere= (GeomObject*)this->ColPtr->ColIP->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(SPHERE_CLASS_ID, 0));
	theSphere->GetParamBlock()->SetValue(0, this->ColPtr->ColIP->GetTime(), this->theHeader1.boundsVer23.BSphereRad);	//radius
	::GeomObject* theBox= (GeomObject*)this->ColPtr->ColIP->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(BOXOBJ_CLASS_ID,0));
	theBox->GetParamBlock()->SetValue(0, this->ColPtr->ColIP->GetTime(), this->theHeader1.boundsVer23.BBoxMax[1]-this->theHeader1.boundsVer23.BBoxMin[1]);	//length
	theBox->GetParamBlock()->SetValue(1, this->ColPtr->ColIP->GetTime(), this->theHeader1.boundsVer23.BBoxMax[0]-this->theHeader1.boundsVer23.BBoxMin[0]);
	theBox->GetParamBlock()->SetValue(2, this->ColPtr->ColIP->GetTime(), this->theHeader1.boundsVer23.BBoxMax[2]-this->theHeader1.boundsVer23.BBoxMin[2]);
	INode* SphereNode= this->ColPtr->ColIP->CreateObjectNode(theSphere);
	INode* BoxNode= this->ColPtr->ColIP->CreateObjectNode(theBox);
	//make transparent
	SphereNode->XRayMtl(TRUE); BoxNode->XRayMtl(TRUE);
	//set names
	std::string boundsName= this->theHeader1.CollisionName; boundsName += "_BoundSphere";
	SphereNode->SetName(const_cast<char*>(boundsName.c_str()));
	boundsName= this->theHeader1.CollisionName; boundsName += "_BoundBox";
	BoxNode->SetName(const_cast<char*>(boundsName.c_str()));
	//set positions
	Matrix3 temp(::Point3(1,0,0), ::Point3(0,1,0), Point3(0,0,1), Point3(this->theHeader1.boundsVer23.BCenter[0], this->theHeader1.boundsVer23.BCenter[1], this->theHeader1.boundsVer23.BCenter[2]));
	SphereNode->SetNodeTM(this->ColPtr->ColIP->GetTime(), temp);
	//adjust z position because box pivot is at the bottom by default (not in the center)
	temp.SetRow(3, Point3(temp.GetRow(3).x, temp.GetRow(3).y, temp.GetRow(3).z-0.5f*(this->theHeader1.boundsVer23.BBoxMax[2]-this->theHeader1.boundsVer23.BBoxMin[2])));
	BoxNode->SetNodeTM(this->ColPtr->ColIP->GetTime(), temp);
	this->colNodes.push_back(SphereNode); this->colNodes.push_back(BoxNode);
}

void ColWorkers::ColWorker2::ReadSpheres()
{
	//check if collision is empty
	if ((this->theHeader23.flags & 0x02)== 0x00)
		return;
	this->ColPtr->iColStream.seekg(this->BaseAddress+4+this->theHeader23.ColSphereOffset, std::ios::beg);
	for (int i= 0; i < this->theHeader23.numColSpheres; ++i)
	{
		extras::TSphereV23 laBalle;
		this->ColPtr->iColStream >> laBalle;
		GeomObject* theBall= (::GeomObject*)this->ColPtr->ColIP->CreateInstance(SClass_ID(GEOMOBJECT_CLASS_ID), Class_ID(SPHERE_CLASS_ID,0));
		theBall->GetParamBlock()->SetValue(0, this->ColPtr->ColIP->GetTime(), (float)laBalle.radius);
		theBall->GetParamBlock()->SetValue(1, this->ColPtr->ColIP->GetTime(), 16);
		INode* ballNode= this->ColPtr->ColIP->CreateObjectNode(theBall);
		Matrix3 temp(Point3(1,0,0), Point3(0,1,0), Point3(0,0,1), Point3(laBalle.center));
		ballNode->SetNodeTM(this->ColPtr->ColIP->GetTime(), temp);
		this->BoxNSpheres[laBalle.surfaceAsUINT].insert(ballNode);
		this->GTABoxNSpheres[laBalle.surface.material].insert(ballNode);
		this->colNodes.push_back(ballNode);
	}
}

void ColWorkers::ColWorker2::ReadBoxes()
{
	//check if collision is empty
	if ((this->theHeader23.flags & 0x02)== 0x00)
		return;
	this->ColPtr->iColStream.seekg(this->BaseAddress+4+this->theHeader23.ColBoxOffset, std::ios::beg);
	for (int i= 0; i < this->theHeader23.numColBoxes; ++i)
	{
		extras::TBox laBoite;
		this->ColPtr->iColStream >> laBoite;
		GeomObject* theBox= (::GeomObject*)this->ColPtr->ColIP->CreateInstance(SClass_ID(GEOMOBJECT_CLASS_ID), Class_ID(BOXOBJ_CLASS_ID,0));

		theBox->GetParamBlock()->SetValue(0, this->ColPtr->ColIP->GetTime(), laBoite.max[1]-laBoite.min[1]);
		theBox->GetParamBlock()->SetValue(1, this->ColPtr->ColIP->GetTime(), laBoite.max[0]-laBoite.min[0]);
		theBox->GetParamBlock()->SetValue(2, this->ColPtr->ColIP->GetTime(), laBoite.max[2]-laBoite.min[2]);
		Matrix3 temp(TRUE);
		temp.SetRow(3, Point3(laBoite.min[0]+0.5f*(laBoite.max[0]-laBoite.min[0]), laBoite.min[1]+0.5f*(laBoite.max[1]-laBoite.min[1]), laBoite.min[2]));
		INode* boxNode= this->ColPtr->ColIP->CreateObjectNode(theBox);
		boxNode->SetNodeTM(this->ColPtr->ColIP->GetTime(), temp);
		this->BoxNSpheres[laBoite.surfaceAsUINT].insert(boxNode);
		this->GTABoxNSpheres[laBoite.surface.material].insert(boxNode);
		this->colNodes.push_back(boxNode);
	}
}

void ColWorkers::ColWorker2::ReadColMesh()
{
	//check if collision is empty
	if ((this->theHeader23.flags & 0x02) == 0x00 || (this->theHeader23.ColmeshFaceOffset == 0x00) || (this->theHeader23.ColmeshVtxOffset == 0x00) ||
		(this->theHeader23.numColmeshFaces == 0x00))
		return;
	//create triobject
	TriObject* colMesh= (TriObject*)this->ColPtr->ColIP->CreateInstance(SClass_ID(GEOMOBJECT_CLASS_ID), Class_ID(EDITTRIOBJ_CLASS_ID, 0));
	
	union {
		UINT maxVertex;
		UINT numFaceGroups;
	};
	maxVertex = 0;	//highest vertex index

	//read faces
	this->ColPtr->iColStream.seekg(this->BaseAddress+4+this->theHeader23.ColmeshFaceOffset, std::ios::beg);
	colMesh->mesh.setNumFaces(this->theHeader23.numColmeshFaces);

	//get materials, set faces and assign IDs
	for (int i= 0; i < colMesh->mesh.numFaces; ++i)
	{
		extras::TFaceV23 theFace;
		this->ColPtr->iColStream >> theFace;
		theFace.surface.material= (theFace.surface.material > 178)? 178 : theFace.surface.material;

		for (int j= 0; j < 3; ++j)
		{
			colMesh->mesh.faces[i].v[j]= theFace[j];
			maxVertex= (maxVertex < theFace[j]) ? theFace[j] : maxVertex;
		}

		colMesh->mesh.FlipNormal(i);
		colMesh->mesh.faces[i].setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_VIS);
		

		if (SendMessage(GetDlgItem(this->ColPtr->hRollup, IDC_OPTGTAMAT), BM_GETCHECK, NULL, NULL) == BST_UNCHECKED)
		{
			std::vector<USHORT>::iterator itr= std::find(this->ColMeshMatsV23.begin(), this->ColMeshMatsV23.end(), theFace.surfaceCompressed);
			colMesh->mesh.faces[i].setMatID(std::distance(this->ColMeshMatsV23.begin(),itr));
			if (itr == this->ColMeshMatsV23.end())
				this->ColMeshMatsV23.push_back(theFace.surfaceCompressed);
		}
		else
		{
			std::vector<BYTE>::iterator itr= std::find(this->GTAsurfaceIDs.begin(), this->GTAsurfaceIDs.end(), theFace.surface.material);
			colMesh->mesh.faces[i].setMatID(std::distance(this->GTAsurfaceIDs.begin(), itr));

			if (itr == this->GTAsurfaceIDs.end())
				this->GTAsurfaceIDs.push_back(theFace.surface.material);
		}
	}

	//read vertices
	colMesh->mesh.setNumVerts(maxVertex+1);		//set vertex count (important !)
	//go to start of collision vertex array
	this->ColPtr->iColStream.seekg(this->BaseAddress+4+this->theHeader23.ColmeshVtxOffset, std::ios::beg);

	for (int i= 0; i <= maxVertex; ++i)
	{
		SHORT vtx[3];
		this->ColPtr->iColStream >> vtx;
		colMesh->mesh.setVert(i, vtx[0]/128.0, vtx[1]/128.0, vtx[2]/128.0);
	}
	
	if (this->theHeader23.flags & 0x08)
	{
		this->ColPtr->iColStream.seekg(this->BaseAddress+4+this->theHeader23.ColmeshFaceOffset, std::ios::beg);
		this->ColPtr->iColStream.seekg(-4, std::ios::cur);
		this->ColPtr->iColStream >> numFaceGroups;
		//MessageBox(NULL, extras::ToString<UINT>(numFaceGroups).c_str(), "Number of face groups", MB_OK);
		this->ColPtr->iColStream.seekg(-(std::streamoff)(4+numFaceGroups*sizeof(extras::TFaceGroup)), std::ios::cur);

		//store face groups using smoothing flags
		for (UINT i= 0; i < numFaceGroups; ++i)
		{
			extras::TFaceGroup theFcGroup;
			this->ColPtr->iColStream >> theFcGroup;
			for (int j= theFcGroup.StartFace; j <= theFcGroup.EndFace; ++j)
				colMesh->mesh.faces[j].setSmGroup(i);
		}
		
	}
	
	//create node
	this->colMeshNode= this->ColPtr->ColIP->CreateObjectNode(colMesh);
	colMeshNode->SetNodeTM(this->ColPtr->ColIP->GetTime(), Matrix3(TRUE));	//set node transform
	std::string colNodeName= this->theHeader1.CollisionName; colNodeName += "_Collision";
	colMeshNode->SetName(const_cast<char*>(colNodeName.c_str()));
	this->colNodes.push_back(colMeshNode);

	//apply collision materials if GTA material checkbox is unticked

	if (SendMessage(GetDlgItem(this->ColPtr->hRollup, IDC_OPTGTAMAT), BM_GETCHECK, NULL, NULL) == BST_UNCHECKED)
	{
		::MultiMtl* multiMat= (::MultiMtl*)this->ColPtr->ColIP->CreateInstance(SClass_ID(MATERIAL_CLASS_ID), Class_ID(MULTI_CLASS_ID, 0));
		multiMat->SetNumSubMtls(this->ColMeshMatsV23.size());
		Mtl* theMat;

		for (std::vector<USHORT>::iterator itr = this->ColMeshMatsV23.begin(); itr < this->ColMeshMatsV23.end(); ++itr)
		{
			//set surface and parts
			extras::TSurfaceV23* leSurface= (extras::TSurfaceV23*)&(*itr);
		
			//Done already
			//if (leSurface->material > 178)
				//leSurface->material= 178;
			
			theMat= (Mtl*)this->ColPtr->ColIP->CreateInstance(SClass_ID(MATERIAL_CLASS_ID), GTA_COLSurface);
			theMat->GetParamBlock(0)->SetValue(0, this->ColPtr->ColIP->GetTime(), (BYTE)leSurface->material);

			if (leSurface->light == 255)
				theMat->GetParamBlock(0)->SetValue(1, this->ColPtr->ColIP->GetTime(), -1);
			else
				theMat->GetParamBlock(0)->SetValue(1, this->ColPtr->ColIP->GetTime(), (BYTE)leSurface->light);


			//add this material
			multiMat->SetSubMtl(std::distance(this->ColMeshMatsV23.begin(), itr), theMat);
		}
		colMeshNode->SetMtl(multiMat);
	}
}

//COL3
//==========
ColWorkers::ColWorker3::ColWorker3(ColPlugin* input, DWORD BAddress) : ColWorker2(input, BAddress)
{
	this->ColPtr->iColStream >> this->theHeader3;
}

void ColWorkers::ColWorker3::ReadShadowMesh()
{
	if (!(this->theHeader23.flags & 0x10) || (SendMessage(GetDlgItem(this->ColPtr->hRollup, IDC_IGNORE_SHADOW), BM_GETCHECK, NULL, NULL) == BST_CHECKED) )
		return;
	//create triobject
	::TriObject* shadowMesh= (TriObject*)this->ColPtr->ColIP->CreateInstance(SClass_ID(GEOMOBJECT_CLASS_ID), Class_ID(EDITTRIOBJ_CLASS_ID, 0));

	UINT maxVert= 0;

	//read faces
	this->ColPtr->iColStream.seekg(this->BaseAddress+4+this->theHeader3.ShadowmeshFaceOffset, std::ios::beg);
	shadowMesh->mesh.setNumFaces(this->theHeader3.numShadowmeshFaces);
	for (int i= 0; i < shadowMesh->mesh.numFaces; ++i)
	{
		extras::TFaceV23 theFace;
		this->ColPtr->iColStream >> theFace;
		for (int j= 0; j < 3; ++j)
		{
			shadowMesh->mesh.faces[i].v[j]= theFace[j];
			maxVert= (maxVert < theFace[j])? theFace[j] : maxVert;
		}
		
		shadowMesh->mesh.faces[i].setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_VIS);
		std::vector<USHORT>::iterator itr= std::find(this->ShadowMeshMats.begin(), this->ShadowMeshMats.end(), theFace.surfaceCompressed);
		shadowMesh->mesh.faces[i].setMatID(std::distance(this->ShadowMeshMats.begin(), itr));
		if (itr == this->ShadowMeshMats.end())
			this->ShadowMeshMats.push_back(theFace.surfaceCompressed);
	}

	//read vertices
	shadowMesh->mesh.setNumVerts(maxVert+1);
	this->ColPtr->iColStream.seekg(this->BaseAddress+4+this->theHeader3.ShadowmeshVtxOffset, std::ios::beg);
	for (int i= 0; i <= maxVert; ++i)
	{
		SHORT vtx[3];
		this->ColPtr->iColStream >> vtx;
		shadowMesh->mesh.setVert(i, vtx[0]/128.0, vtx[1]/128.0, vtx[2]/128.0);
	}

	//create node
	INode* shadowMeshNode= this->ColPtr->ColIP->CreateObjectNode(shadowMesh);
	shadowMeshNode->SetNodeTM(this->ColPtr->ColIP->GetTime(), Matrix3(TRUE));
	std::string shadowNodeName= this->theHeader1.CollisionName; shadowNodeName += "_Shadow";
	shadowMeshNode->SetName(const_cast<char*>(shadowNodeName.c_str()));
	this->colNodes.push_back(shadowMeshNode);

	//apply materials
	::MultiMtl* multiMat= (::MultiMtl*)this->ColPtr->ColIP->CreateInstance(SClass_ID(MATERIAL_CLASS_ID), Class_ID(MULTI_CLASS_ID, 0));
	multiMat->SetNumSubMtls(this->ShadowMeshMats.size());
	for (std::vector<USHORT>::iterator itr= this->ShadowMeshMats.begin(); itr < this->ShadowMeshMats.end(); ++itr)
	{
		Mtl* shadowMat= (Mtl*)this->ColPtr->ColIP->CreateInstance(SClass_ID(MATERIAL_CLASS_ID), GTA_COLShadow);
		extras::TSurfaceV23* laSurface= (extras::TSurfaceV23*)&(*itr);
		shadowMat->GetParamBlock(0)->SetValue(0, this->ColPtr->ColIP->GetTime(), laSurface->material);
		shadowMat->GetParamBlock(0)->SetValue(1, this->ColPtr->ColIP->GetTime(), laSurface->light);
		multiMat->SetSubMtl(std::distance(this->ShadowMeshMats.begin(), itr), shadowMat);
	}
	shadowMeshNode->SetMtl(multiMat);
}