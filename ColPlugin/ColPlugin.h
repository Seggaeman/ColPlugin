#pragma once
#pragma unmanaged
#include "stdafx.h"
#include "extras.h"

#ifdef MAX_2012
	#define PLUGIN_VERSION	VERSION_3DSMAX
#else
	#define PLUGIN_VERSION ((MAX_RELEASE_R12<<16)+(MAX_API_NUM_R120<<8)+MAX_SDK_REV)
#endif
#define GTA_COLSurface Class_ID(0x48238272, 0x26507873)
#define GTA_COLShadow Class_ID(0x48238272, 0x26574236)
#define GTA_Material Class_ID(0x48238272, 0x48206285)
#define GTA_MultiMat Class_ID(0x744721ba, 0x31ec52e0)
extern class ColPlugin;

namespace ColWorkers
{
	class ColWorker
	{
	protected:
		ColPlugin* ColPtr;
		::INode* colMeshNode;
		DWORD BaseAddress;
		std::vector<BYTE> GTAsurfaceIDs;
		std::map<BYTE, std::unordered_set<INode*>> GTABoxNSpheres;
	public:
		virtual void CreateRootDummy()= 0;
		virtual void DrawBounds()= 0;
		virtual void AttachNodes()= 0;
		virtual void ReadSpheres()= 0;
		virtual void ReadBoxes()= 0;
		virtual void AssignBoxNSphereMats()= 0;
		virtual void ReadColMesh()= 0;
		virtual void ReadShadowMesh()= 0;
		ColWorker(ColPlugin* cPlugin, DWORD BAddress);
	};

	class ColWorker1 : public ColWorker
	{
	protected:
		extras::ColHeader1_t theHeader1;
		std::vector<INode*> colNodes;
		std::map<UINT, std::unordered_set<INode*>> BoxNSpheres;
		std::vector<UINT> ColMeshMatsV1;
	public:
		ColWorker1(ColPlugin* input, DWORD BAddress);
		virtual void CreateRootDummy();
		virtual void DrawBounds();
		virtual void AttachNodes();
		virtual void ReadSpheres();
		virtual void ReadBoxes();
		virtual void AssignBoxNSphereMats();
		virtual void ReadColMesh();
		virtual void ReadShadowMesh();
		inline virtual ::Mtl* GenGTAMaterial(BYTE surfaceID);
		inline virtual std::string GetTextureName(BYTE surfaceID);
	};

	class ColWorker2 : public ColWorker1
	{
	protected:
		extras::ColHeader23_t theHeader23;
		std::vector<USHORT> ColMeshMatsV23;
	public:
		ColWorker2(ColPlugin* input, DWORD BAddress);
		virtual void DrawBounds();
		virtual void ReadSpheres();
		virtual void ReadBoxes();
		virtual void ReadColMesh();
	};

	class ColWorker3 : public ColWorker2
	{
	protected:
		extras::ColHeader3_t theHeader3;
		std::vector<USHORT> ShadowMeshMats;
	public:
		ColWorker3(ColPlugin* input, DWORD BAddress);
		virtual void ReadShadowMesh();
	};
};

class ColClassDesc : public ::ClassDesc2
{
public:
	virtual int IsPublic();
	virtual void* Create(BOOL loading= false);
	virtual const MCHAR* ClassName();
	virtual ::SClass_ID SuperClassID();
	virtual Class_ID ClassID();
	virtual const MCHAR* Category();
	virtual const MCHAR* GetInternalName();
	virtual HINSTANCE HInstance();
	static ::ClassDesc2* GetClassDescInstance();
};

class ColPlugin : public ::UtilityObj
{
public:
//fields
	::Interface* ColIP;
	::IUtil* ColIU;
	HWND hRollup;
	extras::BinStream iColStream, oColStream;
	std::map<int, std::tuple<DWORD, DWORD ,UINT>> collisionAddrs;
	int* LboxSelection;
	char szFileName[MAX_PATH];
	ColWorkers::ColWorker* theWorker;
	float PercentageProgress;
//functions
//UtilityObj overrides
//======================
	ColPlugin();
	~ColPlugin();
	virtual void BeginEditParams(::Interface*, IUtil*);
	virtual void EndEditParams(Interface*, IUtil*);
	virtual void SelectionSetChanged(Interface*, IUtil*);
	virtual void DeleteThis();
	virtual ClassDesc2* GetClassDesc();

//ColPlugin specific
//==================
	bool SeekDFFCollision();
	void PopulateListbox();
	void ImportCollisions(int LboxIdx);
	void DeleteCollisions();
};

namespace CallbackSpace
{
class HitCallback : public ::HitByNameDlgCallback
{
	virtual MCHAR* dialogTitle();
	virtual MCHAR* buttonText();
	virtual BOOL singleSelect();
	virtual BOOL useFilter();
	virtual int filter(::INode* leNode);
	virtual BOOL useProc();
	virtual void proc(INodeTab& nodeTab);
	virtual BOOL doCustomHilite();
	virtual BOOL doHilite(::INode* node);
	virtual BOOL showHiddenAndFrozen();

public:
	INode* selectedNode;
	HitCallback(bool whichTitle= true);

private:
	std::string dlgTitle;
};
}