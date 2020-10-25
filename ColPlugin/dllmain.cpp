// dllmain.cpp : Defines the entry point for the DLL application.

#include "stdafx.h"
#include "ColPlugin.h"

HMODULE hModuleInstance= 0;
::ColPlugin lePlugin;
/*
class ColClassDesc : public ::ClassDesc2
{
public:
	virtual int IsPublic() {	return 1;	}
	virtual void* Create(BOOL loading= false) {	return &lePlugin;	}
	virtual const MCHAR* ClassName()	{	return TEXT("Collision_Plugin");	}
	virtual ::SClass_ID SuperClassID()	{ return UTILITY_CLASS_ID;	}
	virtual Class_ID ClassID()	{	return Class_ID(0xa2e24d1, 0x7a946233);		}
	virtual const MCHAR* Category()		{	return TEXT("nouvelle categorie");	}
	virtual const MCHAR* GetInternalName()	{	return TEXT("Collision importer/exporter");	}
	virtual HINSTANCE HInstance()	{	return hModuleInstance;	}
	static ::ClassDesc2* GetClassDescInstance()	{	static ColClassDesc desc;	return &desc;	}
};
*/
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	hModuleInstance= hModule;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

__declspec( dllexport ) const TCHAR * LibDescription() 
{ 
	return TEXT("GTA COL Import/Export plug-in (c) November 2011 by Seggaeman"); 
}

__declspec( dllexport ) int LibNumberClasses() 
{ 
	return 1;  
}

__declspec( dllexport ) ClassDesc2* LibClassDesc(int i) 
{ 
	return ColClassDesc::GetClassDescInstance();
}

__declspec( dllexport ) ULONG LibVersion() 
{ 
	return PLUGIN_VERSION;
}

__declspec( dllexport ) ULONG CanAutoDefer() 
{ 
	return TRUE; 
}

__declspec( dllexport ) ULONG LibInitialize() 
{ 
    // Note: called after the DLL is loaded (i.e. DllMain is called with DLL_PROCESS_ATTACH)
    return 1;
}

__declspec( dllexport ) ULONG LibShutdown() 
{ 
    // Note: called before the DLL is unloaded (i.e. DllMain is called with DLL_PROCESS_DETACH)
    return 1;
}

//Descriptor
//============================

int ColClassDesc::IsPublic()
{
	return TRUE;
}

void* ColClassDesc::Create(BOOL loading)
{
	return &lePlugin;
}

const MCHAR* ColClassDesc::ClassName()
{
	return TEXT("Collision plugin");
}

SClass_ID ColClassDesc::SuperClassID()
{
	return UTILITY_CLASS_ID;
}

Class_ID ColClassDesc::ClassID()
{
	return Class_ID(0xa2e24d1, 0x7a946233);
}

const MCHAR* ColClassDesc::Category()
{
	return TEXT("newer");
}

const MCHAR* ColClassDesc::GetInternalName()
{
	return TEXT("Collision plugin");
}

HINSTANCE ColClassDesc::HInstance()
{
	return ::hModuleInstance;
}

ClassDesc2* ColClassDesc::GetClassDescInstance()
{
	static ColClassDesc description;
	return &description;
}